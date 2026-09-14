// Times the tree-based LimitOrderBook (std::map price ladder +
// unordered_map order lookup) against an alternate HeapLimitOrderBook
// (two std::priority_queue heaps for best-price tracking + unordered_map
// price levels + lazy deletion for stale entries).
//
// Both support the same operations, seeded identically, same RNG seed,
// so any timing difference is attributable to the data structure choice.
//
// Part A: raw insert/cancel cost at increasing book depth (both books
// should be O(log n)-ish here -- std::map insert is O(log P), heap push
// is O(log P) too, so this part is expected to look similar).
//
// Part B: the actual differentiator. We run a realistic cancel/reinsert
// churn phase (cancel a random resting order, place a new one at a
// random nearby price -- mirroring OrderFlowSimulator's churn model),
// then time getBestPrice() calls. The tree's std::map always returns
// its top in O(1) (map::rbegin()/begin() is cached by the tree
// structure itself); the heap has to pop and discard any stale price
// entries left over from cancelled-away levels before it can return a
// valid top, so its getBestPrice cost grows with churn history instead
// of staying flat.

#include "HeapLimitOrderBook.h"
#include "LimitOrderBook.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

using Clock = std::chrono::steady_clock;

static double nsPerOp(Clock::duration total, size_t ops) {
  if (ops == 0)
    return 0.0;
  return std::chrono::duration<double, std::nano>(total).count() /
         static_cast<double>(ops);
}

// ---------- Part A: raw insert/cancel at increasing depth ----------

template <typename Book>
static void seed(Book &book, size_t bookSize, std::mt19937 &rng,
                  std::vector<uint64_t> &ids, size_t &levelsPerSideOut) {
  std::uniform_int_distribution<int> qtyDist(10, 200);
  size_t half = bookSize / 2;
  size_t levelsPerSide = std::max<size_t>(bookSize / 40, size_t{1});
  size_t ordersPerLevel = std::max<size_t>(half / levelsPerSide, size_t{1});
  levelsPerSideOut = levelsPerSide;

  for (size_t lvl = 0; lvl < levelsPerSide; lvl++) {
    double bidPrice = 100.0 - static_cast<double>(lvl) * 0.01;
    double askPrice = 101.0 + static_cast<double>(lvl) * 0.01;
    for (size_t k = 0; k < ordersPerLevel; k++) {
      ids.push_back(book.placeLimitOrder(bidPrice, qtyDist(rng), Side::BUY));
      ids.push_back(book.placeLimitOrder(askPrice, qtyDist(rng), Side::SELL));
    }
  }
}

struct PartAResult {
  size_t bookSize, levelsPerSide;
  double treeInsertNs, heapInsertNs;
  double treeCancelNs, heapCancelNs;
};

static PartAResult runPartA(size_t bookSize, std::mt19937 &rng) {
  LimitOrderBook treeBook;
  HeapLimitOrderBook heapBook;
  std::vector<uint64_t> treeIds, heapIds;
  size_t levelsPerSide = 0;

  seed(treeBook, bookSize, rng, treeIds, levelsPerSide);
  std::mt19937 rngCopy = rng; // identical seed sequence for both books
  seed(heapBook, bookSize, rngCopy, heapIds, levelsPerSide);

  size_t opsToSample = std::min<size_t>(treeIds.size(), 5000);
  std::uniform_int_distribution<size_t> levelDist(0, levelsPerSide - 1);
  std::uniform_int_distribution<int> qtyDist(10, 200);

  Clock::duration treeInsertTotal{}, heapInsertTotal{};
  for (size_t i = 0; i < opsToSample; i++) {
    size_t lvl = levelDist(rng);
    double bidPrice = 100.0 - static_cast<double>(lvl) * 0.01;
    int qty = qtyDist(rng);

    auto t0 = Clock::now();
    treeBook.placeLimitOrder(bidPrice, qty, Side::BUY);
    treeInsertTotal += Clock::now() - t0;

    auto t1 = Clock::now();
    heapBook.placeLimitOrder(bidPrice, qty, Side::BUY);
    heapInsertTotal += Clock::now() - t1;
  }

  std::shuffle(treeIds.begin(), treeIds.end(), rng);
  std::shuffle(heapIds.begin(), heapIds.end(), rng);

  Clock::duration treeCancelTotal{}, heapCancelTotal{};
  for (size_t i = 0; i < opsToSample; i++) {
    auto t0 = Clock::now();
    treeBook.cancelOrder(treeIds[i]);
    treeCancelTotal += Clock::now() - t0;

    auto t1 = Clock::now();
    heapBook.cancelOrder(heapIds[i]);
    heapCancelTotal += Clock::now() - t1;
  }

  return {bookSize,
          levelsPerSide,
          nsPerOp(treeInsertTotal, opsToSample),
          nsPerOp(heapInsertTotal, opsToSample),
          nsPerOp(treeCancelTotal, opsToSample),
          nsPerOp(heapCancelTotal, opsToSample)};
}

// ---------- Part B: transient-order churn, then getBestPrice cost ----------
//
// Each churn "event" places a brand-new order priced ABOVE the resting
// core book (so it is instantly the new best bid) and cancels it again
// immediately. That single order never matters again -- but the tree's
// std::map erases its price key immediately (O(log P), gone for good),
// while the heap can only mark its level dead in the map and leaves the
// price sitting in the heap. Do this churnEvents times and the heap
// accumulates up to churnEvents dead entries, all priced above the real
// (core) best -- so the very next getBestPrice() call has to pop every
// single one of them before it can return a valid answer. That is the
// concrete cost of lazy deletion: not paid per-operation, but paid in a
// single lump sum whenever someone actually asks "what's the best price"
// after a long stretch of top-of-book churn.

struct PartBResult {
  size_t churnEvents;
  double treeFirstQueryNs;
  double heapFirstQueryNs;  // pays off the whole stale backlog
  double heapSecondQueryNs; // backlog just got drained -- should be cheap again
  size_t heapBacklogBeforeQuery;
};

static PartBResult runPartB(size_t churnEvents, std::mt19937 &rng) {
  LimitOrderBook treeBook;
  HeapLimitOrderBook heapBook;
  std::uniform_int_distribution<int> qtyDist(10, 200);

  // small resting "core" book -- this is the real best price both books
  // should end up returning once the transient noise above it is cleared.
  for (int lvl = 0; lvl < 5; lvl++) {
    double bidPrice = 100.0 - lvl * 0.01;
    for (int k = 0; k < 5; k++) {
      treeBook.placeLimitOrder(bidPrice, qtyDist(rng), Side::BUY);
      heapBook.placeLimitOrder(bidPrice, qtyDist(rng), Side::BUY);
    }
  }

  for (size_t i = 0; i < churnEvents; i++) {
    double transientPrice = 100.0 + 0.01 * static_cast<double>(1 + (i % 500));

    uint64_t tid =
        treeBook.placeLimitOrder(transientPrice, qtyDist(rng), Side::BUY);
    treeBook.cancelOrder(tid);

    uint64_t hid =
        heapBook.placeLimitOrder(transientPrice, qtyDist(rng), Side::BUY);
    heapBook.cancelOrder(hid);
  }

  size_t backlog = heapBook.staleBidEntries();

  auto t0 = Clock::now();
  volatile double treeBest = treeBook.getBestPrice(Side::BUY);
  auto treeDur = Clock::now() - t0;
  (void)treeBest;

  auto t1 = Clock::now();
  volatile double heapBest = heapBook.getBestPrice(Side::BUY);
  auto heapDur = Clock::now() - t1;
  (void)heapBest;

  auto t2 = Clock::now();
  volatile double heapBest2 = heapBook.getBestPrice(Side::BUY);
  auto heapDur2 = Clock::now() - t2;
  (void)heapBest2;

  return {churnEvents, nsPerOp(treeDur, 1), nsPerOp(heapDur, 1),
          nsPerOp(heapDur2, 1), backlog};
}

int main() {
  std::mt19937 rng(7);
  std::vector<size_t> sizes = {1000, 5000, 10000, 50000, 100000, 200000};

  std::cout << std::fixed << std::setprecision(1);

  std::cout << "Part A: tree-based LimitOrderBook vs HeapLimitOrderBook, "
               "insert/cancel at increasing book depth\n"
               "(both are O(log n)-ish here -- std::map insert vs heap "
               "push -- expected to track each other)\n\n";
  std::cout << std::left << std::setw(10) << "book_size" << std::setw(16)
            << "tree_insert_ns" << std::setw(16) << "heap_insert_ns"
            << std::setw(16) << "tree_cancel_ns" << std::setw(16)
            << "heap_cancel_ns" << "\n";
  for (size_t n : sizes) {
    PartAResult r = runPartA(n, rng);
    std::cout << std::left << std::setw(10) << r.bookSize << std::setw(16)
              << r.treeInsertNs << std::setw(16) << r.heapInsertNs
              << std::setw(16) << r.treeCancelNs << std::setw(16)
              << r.heapCancelNs << "\n";
  }

  std::cout << "\nPart B: cost of the FIRST getBestPrice() call after N "
               "transient top-of-book create+cancel events\n"
               "(the heap pays its lazy-deletion backlog off in one lump "
               "sum here; the tree never accumulates one)\n\n";
  std::cout << std::left << std::setw(14) << "churn_events" << std::setw(16)
            << "stale_backlog" << std::setw(16) << "tree_1st_ns"
            << std::setw(16) << "heap_1st_ns" << std::setw(16)
            << "heap_2nd_ns" << "\n";
  for (size_t churn : {0u, 1000u, 5000u, 20000u, 50000u, 100000u}) {
    PartBResult r = runPartB(churn, rng);
    std::cout << std::left << std::setw(14) << r.churnEvents << std::setw(16)
              << r.heapBacklogBeforeQuery << std::setw(16)
              << r.treeFirstQueryNs << std::setw(16) << r.heapFirstQueryNs
              << std::setw(16) << r.heapSecondQueryNs << "\n";
  }

  std::cout << "\nDone.\n";
  return 0;
}

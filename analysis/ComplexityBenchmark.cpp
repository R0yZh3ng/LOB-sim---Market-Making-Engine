// Empirically measures the O(1) vs O(log n) tradeoff behind the
// LimitOrderBook's design: an unordered_map (OrderHashMap) for O(1) order
// lookup/cancel, backed by std::map price ladders that cost O(log P) per
// touch (P = number of distinct price levels).
//
// Part A drives the real LimitOrderBook under increasing book depth and
// times cancelOrder (hash lookup + list unlink) vs placeLimitOrder
// (map lookup + list append) directly.
//
// Part B isolates the two container types on their own (unordered_map vs
// map, random-key lookup) to show the underlying complexity classes
// cleanly, decoupled from the linked-list/matching mechanics.

#include "LimitOrderBook.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <unordered_map>
#include <vector>

using Clock = std::chrono::steady_clock;

static double nsPerOp(Clock::duration total, size_t ops) {
  if (ops == 0)
    return 0.0;
  return std::chrono::duration<double, std::nano>(total).count() /
         static_cast<double>(ops);
}

// ---------- Part A: benchmark the real LimitOrderBook under load ----------
struct LobBenchResult {
  size_t bookSize;
  size_t priceLevelsPerSide;
  double avgInsertNs;
  double avgCancelNs;
};

static LobBenchResult benchmarkLob(size_t bookSize, std::mt19937 &rng) {
  LimitOrderBook lob;
  std::vector<uint64_t> ids;
  ids.reserve(bookSize);
  std::uniform_int_distribution<int> qtyDist(10, 200);

  // ~40 resting orders per distinct price level, split across both sides,
  // so the number of price levels (which drives the std::map cost) grows
  // linearly with book depth -- this is what makes the O(log P) term
  // actually show up as book depth increases.
  size_t half = bookSize / 2;
  size_t levelsPerSide = std::max<size_t>(bookSize / 40, size_t{1});
  size_t ordersPerLevel = std::max<size_t>(half / levelsPerSide, size_t{1});

  for (size_t lvl = 0; lvl < levelsPerSide; lvl++) {
    double bidPrice = 100.0 - static_cast<double>(lvl) * 0.01;
    double askPrice = 101.0 + static_cast<double>(lvl) * 0.01;
    for (size_t k = 0; k < ordersPerLevel; k++) {
      ids.push_back(lob.placeLimitOrder(bidPrice, qtyDist(rng), Side::BUY));
      ids.push_back(lob.placeLimitOrder(askPrice, qtyDist(rng), Side::SELL));
    }
  }

  size_t opsToSample = std::min<size_t>(ids.size(), 5000);

  // Insertion first, while the book sits at its full target depth: this
  // measures placeLimitOrder cost (map lookup + O(1) queue append) against
  // a book that already has `levelsPerSide` price levels on the bid side.
  std::uniform_int_distribution<size_t> levelDist(0, levelsPerSide - 1);
  Clock::duration insertTotal{};
  for (size_t i = 0; i < opsToSample; i++) {
    size_t lvl = levelDist(rng);
    double bidPrice = 100.0 - static_cast<double>(lvl) * 0.01;
    auto start = Clock::now();
    lob.placeLimitOrder(bidPrice, qtyDist(rng), Side::BUY);
    insertTotal += Clock::now() - start;
  }

  // Cancel benchmark: O(1) hash lookup + O(1) doubly-linked-list unlink,
  // independent of how many price levels exist.
  std::shuffle(ids.begin(), ids.end(), rng);
  Clock::duration cancelTotal{};
  for (size_t i = 0; i < opsToSample; i++) {
    auto start = Clock::now();
    lob.cancelOrder(ids[i]);
    cancelTotal += Clock::now() - start;
  }

  return {bookSize, levelsPerSide, nsPerOp(insertTotal, opsToSample),
          nsPerOp(cancelTotal, opsToSample)};
}

// ---------- Part B: isolated hash map vs ordered map lookup cost ----------
struct MapBenchResult {
  size_t n;
  double avgHashMapNs;
  double avgOrderedMapNs;
};

static MapBenchResult benchmarkMaps(size_t n, std::mt19937 &rng) {
  std::vector<uint64_t> keys(n);
  std::iota(keys.begin(), keys.end(), 1);
  std::shuffle(keys.begin(), keys.end(), rng);

  std::unordered_map<uint64_t, uint64_t> hashMap;
  std::map<uint64_t, uint64_t> orderedMap;
  hashMap.reserve(n);
  for (uint64_t k : keys) {
    hashMap[k] = k;
    orderedMap[k] = k;
  }

  size_t opsToSample = std::min<size_t>(n, 20000);
  std::vector<uint64_t> lookupKeys(keys.begin(), keys.begin() + opsToSample);
  std::shuffle(lookupKeys.begin(), lookupKeys.end(), rng);

  // sink forces the compiler to actually keep the lookups (their results
  // feed a value that gets used below), so -O2 can't optimize them away.
  volatile uint64_t sink = 0;

  auto startH = Clock::now();
  for (uint64_t k : lookupKeys)
    sink += hashMap.find(k)->second;
  auto hashDur = Clock::now() - startH;

  auto startO = Clock::now();
  for (uint64_t k : lookupKeys)
    sink += orderedMap.find(k)->second;
  auto orderedDur = Clock::now() - startO;

  return {n, nsPerOp(hashDur, opsToSample), nsPerOp(orderedDur, opsToSample)};
}

int main() {
  std::mt19937 rng(7);
  std::vector<size_t> sizes = {1000, 5000, 10000, 50000, 100000, 200000};

  std::cout << std::fixed << std::setprecision(1);

  std::cout << "Part A: LimitOrderBook under increasing book depth\n"
               "(cancelOrder = O(1) hash lookup; placeLimitOrder = O(log P) "
               "map lookup, P = distinct price levels)\n\n";
  std::cout << std::left << std::setw(12) << "book_size" << std::setw(16)
            << "price_levels" << std::setw(18) << "avg_insert_ns"
            << std::setw(18) << "avg_cancel_ns" << "\n";
  std::vector<LobBenchResult> lobResults;
  for (size_t n : sizes) {
    LobBenchResult r = benchmarkLob(n, rng);
    lobResults.push_back(r);
    std::cout << std::left << std::setw(12) << r.bookSize << std::setw(16)
              << r.priceLevelsPerSide << std::setw(18) << r.avgInsertNs
              << std::setw(18) << r.avgCancelNs << "\n";
  }

  std::cout << "\nPart B: isolated unordered_map (O(1)) vs map (O(log n)) "
               "random-key lookup, decoupled from the LOB's linked-list "
               "mechanics\n\n";
  std::cout << std::left << std::setw(12) << "n" << std::setw(18)
            << "avg_hashmap_ns" << std::setw(18) << "avg_orderedmap_ns"
            << std::setw(10) << "ratio" << "\n";
  for (size_t n : sizes) {
    MapBenchResult r = benchmarkMaps(n, rng);
    double ratio = r.avgHashMapNs > 0 ? r.avgOrderedMapNs / r.avgHashMapNs
                                       : 0.0;
    std::cout << std::left << std::setw(12) << r.n << std::setw(18)
              << r.avgHashMapNs << std::setw(18) << r.avgOrderedMapNs
              << std::setw(10) << ratio << "\n";
  }

  return 0;
}

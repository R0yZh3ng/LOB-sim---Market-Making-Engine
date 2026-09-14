// Alternate LimitOrderBook backed by two binary heaps instead of two
// std::map price ladders -- built purely to benchmark against the
// tree-based LimitOrderBook (see analysis/HeapVsTreeBenchmark.cpp).
//
// Why a heap needs "lazy deletion":
// std::priority_queue only ever exposes its top element and cannot erase
// an arbitrary interior element in better than O(n). So when a price
// level empties out (last resting order there cancelled/filled), we
// remove it from the *level* map (O(1) average) but leave its price
// sitting in the heap. getBestPrice() then pops any stale price off the
// top -- one whose level no longer exists -- before returning the real
// top. This makes push/erase-from-map O(log n)/O(1), same big-O as the
// tree version's O(log P) map insert/erase, but getBestPrice is no
// longer O(1): under heavy churn, stale entries pile up and have to be
// skipped, which is the actual tradeoff this benchmark is measuring.

#ifndef HEAPLIMITORDERBOOK_H
#define HEAPLIMITORDERBOOK_H

#include "OrderNode.h"
#include "PriceLevel.h"
#include <cstdint>
#include <queue>
#include <unordered_map>
#include <vector>

struct HeapLimitOrderBook {
private:
  std::priority_queue<double> bidHeap; // max-heap: top() is best bid
  std::priority_queue<double, std::vector<double>, std::greater<double>>
      askHeap; // min-heap: top() is best ask

  std::unordered_map<double, PriceLevel> bidLevels;
  std::unordered_map<double, PriceLevel> askLevels;
  std::unordered_map<uint64_t, OrderNode *> OrderHashMap;

  static uint64_t nextOrderId;
  uint64_t totalVolume;

  uint64_t placeOrder(double price, uint64_t quantity, Side side);
  void removePriceLevelIfEmpty(double price);
  void fillOrder(OrderNode *incomingOrder);

public:
  HeapLimitOrderBook();

  uint64_t placeLimitOrder(double price, uint64_t quantity, Side side);
  void cancelOrder(uint64_t orderId);
  double getBestPrice(Side side); // not const: lazily pops stale heap entries
  uint64_t getVolumeAtPrice(double p) const;
  bool hasOrders(Side side) const;

  // How many stale (already-removed) price entries are still sitting in
  // the heap, waiting to be popped -- exposes the lazy-deletion backlog
  // that the tree-based book never accumulates.
  size_t staleBidEntries() const;
  size_t staleAskEntries() const;
};
#endif

#include "HeapLimitOrderBook.h"
#include "OrderNode.h"
#include "PriceLevel.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

HeapLimitOrderBook::HeapLimitOrderBook() : totalVolume(0) {}
uint64_t HeapLimitOrderBook::nextOrderId = 0;

uint64_t HeapLimitOrderBook::placeLimitOrder(double price, uint64_t quantity,
                                              Side side) {
  return placeOrder(price, quantity, side);
}

void HeapLimitOrderBook::cancelOrder(uint64_t orderId) {
  auto it = OrderHashMap.find(orderId);
  if (it == OrderHashMap.end()) {
    throw std::invalid_argument("Order ID not found");
  }

  OrderNode *nodeToCancel = it->second;
  double price = nodeToCancel->price;

  nodeToCancel->parentLevel->removeOrder(nodeToCancel);
  OrderHashMap.erase(it);

  // Note: the heap entry for `price` is deliberately left in place here --
  // that's the lazy-deletion tradeoff. It gets skipped later in
  // getBestPrice() if/when it surfaces at the top.
  removePriceLevelIfEmpty(price);
}

double HeapLimitOrderBook::getBestPrice(Side side) {
  if (side == Side::BUY) {
    while (!bidHeap.empty() && bidLevels.find(bidHeap.top()) == bidLevels.end())
      bidHeap.pop(); // skip stale (already-removed) price entries
    if (bidHeap.empty())
      throw std::invalid_argument("no orders on book");
    return bidHeap.top();
  } else if (side == Side::SELL) {
    while (!askHeap.empty() && askLevels.find(askHeap.top()) == askLevels.end())
      askHeap.pop();
    if (askHeap.empty())
      throw std::invalid_argument("no orders on book");
    return askHeap.top();
  }
  throw std::invalid_argument("side must be either buy or sell");
}

bool HeapLimitOrderBook::hasOrders(Side side) const {
  return side == Side::BUY ? !bidLevels.empty() : !askLevels.empty();
}

uint64_t HeapLimitOrderBook::getVolumeAtPrice(double price) const {
  uint64_t volume = 0;
  auto askIt = askLevels.find(price);
  if (askIt != askLevels.end())
    volume += askIt->second.totalVolume;
  auto bidIt = bidLevels.find(price);
  if (bidIt != bidLevels.end())
    volume += bidIt->second.totalVolume;
  return volume;
}

size_t HeapLimitOrderBook::staleBidEntries() const {
  return bidHeap.size() - bidLevels.size();
}

size_t HeapLimitOrderBook::staleAskEntries() const {
  return askHeap.size() - askLevels.size();
}

// ---- private ----

uint64_t HeapLimitOrderBook::placeOrder(double price, uint64_t quantity,
                                         Side side) {
  if (side != Side::BUY && side != Side::SELL) {
    throw std::invalid_argument("orderType must be of either buy or sell");
  }
  nextOrderId++;

  OrderNode *ordn = new OrderNode(quantity, nextOrderId, side);

  auto &levels = (side == Side::BUY) ? bidLevels : askLevels;

  if (levels.find(price) == levels.end()) {
    levels.emplace(price, PriceLevel(price));
    // Only push when the level is newly (re)created -- pushing on every
    // order would make the heap grow without bound even faster than the
    // natural stale-entry backlog already does.
    if (side == Side::BUY)
      bidHeap.push(price);
    else
      askHeap.push(price);
  }

  ordn->price = price;
  ordn->parentLevel = &levels.at(price);

  levels.at(price).addOrder(ordn);
  OrderHashMap.emplace(nextOrderId, ordn);

  fillOrder(ordn);

  if (ordn->quantity == 0) {
    double filledPrice = ordn->price;
    uint64_t filledOrderId = ordn->orderId;

    ordn->parentLevel->removeOrder(ordn);
    removePriceLevelIfEmpty(filledPrice);
    OrderHashMap.erase(filledOrderId);
  }
  return nextOrderId;
}

void HeapLimitOrderBook::removePriceLevelIfEmpty(double price) {
  auto bidIt = bidLevels.find(price);
  if (bidIt != bidLevels.end() && bidIt->second.isEmpty())
    bidLevels.erase(bidIt);
  auto askIt = askLevels.find(price);
  if (askIt != askLevels.end() && askIt->second.isEmpty())
    askLevels.erase(askIt);
}

void HeapLimitOrderBook::fillOrder(OrderNode *incomingOrder) {
  Side oppositeSide =
      incomingOrder->side == Side::BUY ? Side::SELL : Side::BUY;
  auto &oppositeLevels =
      incomingOrder->side == Side::BUY ? askLevels : bidLevels;

  while (incomingOrder->quantity > 0) {
    double bestPrice;
    try {
      bestPrice = getBestPrice(oppositeSide);
    } catch (const std::invalid_argument &) {
      return; // opposite side empty
    }

    if (incomingOrder->side == Side::BUY) {
      if (incomingOrder->price < bestPrice)
        return;
    } else {
      if (incomingOrder->price > bestPrice)
        return;
    }

    PriceLevel &level = oppositeLevels.at(bestPrice);
    OrderNode *bookOrder = level.peekHead();

    uint64_t quantityFilled =
        std::min<uint64_t>(incomingOrder->quantity, bookOrder->quantity);

    incomingOrder->quantity -= quantityFilled;
    bookOrder->quantity -= quantityFilled;
    totalVolume -= quantityFilled * 2;

    if (bookOrder->quantity == 0) {
      uint64_t id = bookOrder->orderId;
      level.popHead();
      OrderHashMap.erase(id);
    }

    if (level.isEmpty()) {
      oppositeLevels.erase(bestPrice);
      // heap entry for bestPrice is left as a stale entry, cleaned up
      // lazily next time getBestPrice() is called on this side.
    }
  }
}

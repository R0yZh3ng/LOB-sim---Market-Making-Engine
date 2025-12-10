#include "LimitOrderBook.h"
#include "OrderNode.h"
#include "PriceLevel.h"

#include <cstdint>
#include <iterator>
#include <map>
#include <stdexcept>

LimitOrderBook::LimitOrderBook()
    : totalVolume(0) {} // maps automatically initiate empty
uint64_t LimitOrderBook::nextOrderId =
    0; // initialize global id counter to ensure quick unique orders, should not
       // be cleared and the uint_64 makes sure the orders will never exceed the
       // limit

// note that to the maps will store the price levels in ascending order, but
// using, std::prev(bidLevels.end()) gives u the lowest ask

uint64_t LimitOrderBook::placeLimitOrder(double price, uint64_t quantity,
                                     Side side) {
  // note that a limit order fills for everything under

  // OrderNode* ordn = new OrderNode(quantity, id++, side); //create the node on
  // the heap instead of the stack because the node needs to outlive the
  // function that created it
  //
  // if (side == side::BUY) {
  //   if (bidLevels.find(price) == bidLevels.end()) {
  //     bidLevels.emplace(price, PriceLevel(price));//can use emplace, which is
  //     preferred for objects as they are constructed within it, can use
  //     insert, common with key value pairs, or just reference it by the
  //     index/key, if it doesnt exist, it will initiate it and then add it
  //   }
  //   bidLevels[price].addOrder(ordn);
  //
  // }
  // else if (side == Side::SELL) {
  //   if (askLevels.find(price) == askLevels.end()) {
  //     askLevels.emplace(price, PriceLevel(price));
  //
  //   askLevels[price].addOrder(ordn);
  // }
  // else {
  //    throw std::invalid_argument("the side should match either buy or sell");
  // }

  // above is a non efficient way of implementation, can abstract certain steps,
  // also dont allocate the node on the heap;
  uint64_t orderId = placeOrder(price, quantity, side);
  return orderId;
}

uint64_t LimitOrderBook::placeMarketOrder(uint64_t quantity, Side side) {
  // this function needs to fil the amount to buy or sell at either the highest
  // bid or lowest ask, partial filling is needed
  double price = getBestPrice(side);
  uint64_t orderId = placeOrder(price, quantity, side);

  return orderId;
}

void LimitOrderBook::cancelOrder(uint64_t orderId) {
  OrderNode *nodeToCancel = OrderHashMap[orderId];

  nodeToCancel->parentLevel->removeOrder(nodeToCancel);

  removePriceLevelIfEmpty(nodeToCancel->price);

  // note to self need to implement the remove price level logic later
}

void LimitOrderBook::modifyOrder(double price, uint64_t orderId,
                                 uint64_t quantity, Side side) {
  // side should not be able to be changed, since it breaks market integrity
  // with possible fast change liquidity

  if (OrderHashMap.find(orderId) == OrderHashMap.end()) {
    throw std::invalid_argument("Order ID not found");
  }

  OrderNode *order = OrderHashMap[orderId];

  double originalPrice = order->price;
  uint64_t originalQuantity = order->quantity;

  if (originalPrice != price || originalQuantity != quantity) {
    // Remove from old price level
    order->parentLevel->removeOrder(order);
    removePriceLevelIfEmpty(originalPrice);

    // Update order details
    order->price = price;
    order->quantity = quantity;

    // Add to new price level
    auto &levels = (side == Side::BUY) ? bidLevels : askLevels;
    if (levels.find(price) == levels.end()) {
      levels.emplace(price, PriceLevel(price));
    }
    order->parentLevel = &levels.at(price);
    levels.at(price).addOrder(order);

    // Try to fill if it's a marketable order now (simplified, normally modify
    // might lose priority) For now, just re-adding to book.
  }
}

double LimitOrderBook::getBestPrice(Side side) const {
  if (side == Side::BUY) {
    if (bidLevels.empty()) {
      throw std::invalid_argument("no orders on book");
    }
    return bidLevels.rbegin()->first;
  } else if (side == Side::SELL) {
    if (askLevels.empty()) {
      throw std::invalid_argument("no orders on book");
    }
    return askLevels.begin()->first;
  }

  throw std::invalid_argument("side must be either buy or sell");
}

uint64_t LimitOrderBook::getVolumeAtPrice(double price) const {
  uint64_t volume = 0;
  if (askLevels.count(price)) {
    volume += askLevels.at(price).totalVolume;
  }
  if (bidLevels.count(price)) {
    volume += bidLevels.at(price).totalVolume;
  }
  return volume;
}

// private helpers below
// here;////////////////////////////////////////////////////////////////////

void LimitOrderBook::placeOrder(double price, uint64_t quantity, Side side) {
  if (side != Side::BUY && side != Side::SELL) {
    throw std::invalid_argument("orderType must be of either buy or sell");
  }
  nextOrderId++; // increment the global id counter

  OrderNode *ordn = new OrderNode(quantity, nextOrderId, side);

  auto &levels = (side == Side::BUY)
                     ? bidLevels
                     : askLevels; // the ampersand here does not mean the
                                  // address but rather that it is a reference
                                  // to the level that was assigned to it.

  if (levels.find(price) == levels.end()) {
    levels.emplace(price, PriceLevel(price));
  }

  ordn->price = price;
  ordn->parentLevel = &levels.at(price);

  levels.at(price).addOrder(ordn);
  OrderHashMap.emplace(nextOrderId,
                       ordn); // put into an unordered map for O(1) seek
  
  fillOrder(ordn);

  if (ordn->quantity == 0) {
    ordn->parentLevel->removeOrder(ordn);
    removePriceLevelIfEmpty(ordn->price);
    OrderHashMap.erase(ordn->orderId);
    // delete ordn;
  }
  return nextOrderId;
  
}

void LimitOrderBook::removePriceLevelIfEmpty(double price) {
  // think about some way to simplify this further, there is some repeated code,
  // do i want to pass in a side or should both the orderLevels be checked each
  // time this is called considering that the fillOrder logic will also call
  // this at the of a orderFill

  if (bidLevels.count(price) && bidLevels.at(price).isEmpty()) {
    bidLevels.erase(price);
  }
  if (askLevels.count(price) && askLevels.at(price).isEmpty()) {
    askLevels.erase(price);
  }
}

void LimitOrderBook::fillOrder(OrderNode *incomingOrder) {
  std::map<double, PriceLevel> *opposite;

  if (incomingOrder->side == Side::BUY) {
    opposite = &askLevels;
  } else {
    opposite = &bidLevels;
  }

  while (incomingOrder->quantity > 0 && !opposite->empty()) {
    double bestPrice;
    if (incomingOrder->side == Side::BUY) {
      bestPrice = opposite->begin()->first;
      if (incomingOrder->price < bestPrice) {
        return; // Buy order price is lower than lowest ask
      }
    } else {
      bestPrice = opposite->rbegin()->first;
      if (incomingOrder->price > bestPrice) {
        return; // Sell order price is higher than highest bid
      }
    }

    PriceLevel &level = opposite->at(bestPrice);
    OrderNode *bookOrder = level.peekHead();

    uint64_t quantityFilled =
        std::min(incomingOrder->quantity, bookOrder->quantity);

    incomingOrder->quantity -= quantityFilled;
    bookOrder->quantity -= quantityFilled;
    totalVolume -=
        quantityFilled *
        2; // Deduct volume from both sides? Or just track trade volume?
    // The design doc says "incremented/decremented the volume counter when
    // order were added and cancelled". So if we fill, we are effectively
    // removing from book.

    // Note: The original code didn't handle totalVolume updates during fill
    // correctly in all places. PriceLevel handles its own volume.
    // LimitOrderBook totalVolume might be aggregate. For now, let's stick to
    // the logic of matching.

    if (bookOrder->quantity == 0) {
      uint64_t id = bookOrder->orderId;
      level.popHead();
      OrderHashMap.erase(id);
    }

    if (level.isEmpty()) {
      opposite->erase(bestPrice);
    }
  }
}

#include "LimitOrderBook.h"
#include "PriceLevel.h"
#include "OrderNode.h"

#include <cstdint>
#include <stdexcept>

LimitOrderBook::LimitOrderBook() : totalVolume(0) {} //maps automatically initiate empty
uint64_t LimitOrderBook::nextOrderId = 0; //initialize global id counter to ensure quick unique orders, should not be cleared and the uint_64 makes sure the orders will never exceed the limit 

//note that to the maps will store the price levels in ascending order, but using, std::prev(bidLevels.end()) gives u the lowest ask 

void LimitOrderBook::placeLimitOrder(double price, uint64_t quantity, Side side) {
 //note that a limit order fills for everything under

 // OrderNode* ordn = new OrderNode(quantity, id++, side); //create the node on the heap instead of the stack because the node needs to outlive the function that created it 
 //   
 // if (side == side::BUY) {
 //   if (bidLevels.find(price) == bidLevels.end()) {
 //     bidLevels.emplace(price, PriceLevel(price));//can use emplace, which is preferred for objects as they are constructed within it, can use insert, common with key value pairs, or just reference it by the index/key, if it doesnt exist, it will initiate it and then add it 
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
 
 // above is a non efficient way of implementation, can abstract certain steps, also dont allocate the node on the heap;
  placeOrder(price, quantity, side);
}

void LimitOrderBook::placeMarketOrder(uint64_t quantity, Side side) {
 // this function needs to fil the amount to buy or sell at either the highest bid or lowest ask, partial filling is needed
 double price = getBestPrice(side);
 placeOrder(price, quantity, side);
 
}

void LimitOrderBook::cancelOrder(uint_64_t orderId) {
 OrderNode* nodeToCancel = OrderHashMap[orderId];
 
 nodeToCancel->parentLevel.removeOrder(nodeToCancel);
 
 removePriceLevelIfEmpty(nodeToCancel->price);
 
 //note to self need to implement the remove price level logic later
}

double LimitOrderBook::getBestPrice(Side side) const {
  if (side == side::BUY) {
    if (askLevels.empty()) {
      throw std::invalid_argument("no orders on book");
    }
    return askLevels.begin()->first;
  } 
  else if(side == side::SELL) {
    if (bidLevels.empty()) {
      throw std::invalid_argument("no orders on book");
    }
    return prev(bidLevels.end())->first;
  }

  throw std::invalid_argument("side must be either buy or sell");
}


uint64_t LimtiOrderBook::getVolumeAtPrice(double price) const {
  return askLevels.at(price).totalVolume + bidLevels.at(price).totalVolume;
}



//private helpers below here;////////////////////////////////////////////////////////////////////

void LimitOrderBook::placeOrder(double price, uint64_t quantity, Side side) {
 if (side != Side::BUY && side != Side::SELL) {
  throw std::invalid_argument("orderType must be of either buy or sell");
 } 
 nextOrderId++; //increment the global id counter
       
 OrderNode* ordn = new OrderNode(quantity, nextOrderId, side);

 auto& levels = (side == Side::BUY) ? bidLevels : askLevels; // the ampersand here does not mean the address but rather that it is a reference to the level that was assigned to it.
 
 if (levels.find(price) == levels.end()) {
   levels.emplace(price, PriceLevel(price));
 }

 ordn->price = price;
 ordn->parentLevel = &levels[price];  

 levels[price].addOrder(ordn);
 OrderHashMap.emplace(nextOrderId, ordn); //put into an unordered map for O(1) seek
 
 fillOrder(ordn);
 
}

void LimtiOrderBook::removePriceLevelIfEmpty(double price) {
  //think about some way to simplify this further, there is some repeated code, do i want to pass in a side or should both the orderLevels be checked each time this is called
  //considering that the fillOrder logic will also call this at the of a orderFill

  if (bidLevels[price].isEmpty()) {
    bidLevels.erase(price); 
  }
  if (askLevels[price].isEmpty()) {
    askLevels.erase(price);
  }
}

void LimitOrderBook::fillOrder(OrderNode* incomingOrder) {
  // lets think about what this funciton should do, on call, we need to go through the best asking price and the best bid price and see if there is a match available, if so, remove corresponding quantity
  // from both sides and remove the order if completed
  // is it neccesary to implement partial fills? should I be considering how the fill logic works when users are implemented with actual 
  // this function should only be callled by one order, the one that is just added, and it just fills against the other Side, if theres no fill or left overs, just keep the order in the price leevl
  // auto& opposite =  (incomingOrder->side == Side::BUY) ? askLevels : bidLevels;
 
  // double price = incomingOrder->price;
  // double quantity = incomingOrder->quantity;
 
  // while (quantity > 0) {
  //   if (opposite.find(price) == opposite.end()) {
  //     return;
  //   }
  //   
  //   OrderNode* topOfBook = opposite[price]peakHead();
 
  //   if (topOfBook->quantity > quantity) {
  //     topOfBook->quantity -= quantity;
  //     incomingOrder->quantity -= quantity;
  //     quantity -= topOfBook->quantity;
  //     break;
  //   }
  //   
  //   opposite[price].popHead();
  //   incomingOrder->quantity -= topOfBook->quantity;
  //   quantity -= topOfBook->quantity;
  // }
  //
  // this implementation did not take into account limit orders that could not be met and goes into a infinite loop
  //
  auto& opposite;
  double bestPrice;

  if (incomingOrder->side == Side::BUY) {
    opposite = askLevels;
    bestPrice = askLevels.begin()->first;
  } else {
    opposite = bidLevels;
    bestPrice = std::prev(bidLevels.begin())->first;
  }
 
  
  while (incomingOrder->quantity > 0) {
    if (opposite.empty()) {
      return; //no liquidity on the opposite level
    }

   
    if (incomingOrder->side == Side::BUY && incomingOrder->price < bestPrice) {
      return; // limit buy order that is higher than all current asks
    }
    
    if (incomingOrder->side == Side::SELL && incomingOrder->price > bestPrice) {
      return; // limit sell order that is lower than all current asks
    }

    uint64_t quantityFilled = std::min(incomingOrder->quantity, opposite[bestPrice].peekHead()->quantity);

    incomingOrder->quantity -= quantityFilled;
    opposite[price].peekHead()->quantity -= quantityFilled;

    if(opposite[price].peekHead()->quantity == 0) {
      opposite[price].popHead();
      removePriceLevelIfEmpty(bestPrice);
    }
    
    
  }

}

#include "LimitOrderBook.h"
#include "PriceLevel.h"
#include "OrderNode.h"

#include <cstdint>
#include <stdexcept>

LimtiOrderBook::LimitOrderBook() : totalVolume(0) {} //maps automatically initiate empty
uint64_t nextOrderId = 0; //initialize global id counter to ensure quick unique orders, should not be cleared and the uint_64 makes sure the orders will never exceed the limit 

//note that to the maps will store the price levels in ascending order, but using, std::prev(bidLevels.end()) gives u the lowest ask 

void LimitOrderBook::placeLimitOrder(double price, uint64_t quantity, Side side) {

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

void LimitOrderBook::placeMarketOrder(uint_64_t quantity, Side side) {
 // this function needs to fil the amount to buy or sell at either the highest bid or lowest ask, partial filling is needed
 double price = getBestPrice(side);
 placeOrder(price, quantity, side);
 
}

void LimitOrderBook::cancelOrder(uint_64_t orderId) {
 OrderNode* nodeToCancel = OrderHashMap[orderId]->second;
 
 nodeToCancel->parentLevel.removeOrder(nodeToCancel);
 
 //note to self need to implement the remove price level logic later
}

double LimitOrderBook::getBestPrice(Side side) const {
  return (side == Side::BUY) askLevels.begin()->first : prev(bidLevels.end())->first;
}


uint64_t LimtiOrderBook::getVolumeAtPrice(double price) const {
  return askLevels[price]->second->totalVolume + bidLevels[price]->second->totalVolume;
}



//private helpers below here;////////////////////////////////////////////////////////////////////

void placeOrder(double price, uint64_t quantity, Side side) {
 if (side != Side::BUY && side != Side::SELL) {
  throw std::invalid_argument("orderType must be of either buy or sell");
 } 
 nextOrderId++; //increment the global id counter
       
 OrderNode* ordn = new OrderNode(quantity, id, side);

 auto& levels = (side == Side::BUY) ? bidLevels : askLevels; // the ampersand here does not mean the address but rather that it is a reference to the level that was assigned to it.
 
 if (levels.find(price) == levels.end()) {
   levels.emplace(price, PriceLevel(price));
 }
 levels[price].addOrder(ordn);
 OrderHashMap.emplace(nextOrderId, ordn); //put into an unordered map for O(1) seek
 
 ordn->price = price;
 ordn->parentLevel = &levels[price];  
}

void removePriceLevelIfEmpty(double price) {
  return;
}

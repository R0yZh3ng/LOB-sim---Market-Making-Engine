#include "LimitOrderBook.h"
#include "PriceLevel.h"
#include "OrderNode.h"

#include <cstdint>
#include <stdexcept>

LimtiOrderBook::LimitOrderBook() : totalVolume(0) {} //maps automatically initiate empty
uint64_t id = 0;

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
 //   }
 //   askLevels[price].addOrder(ordn);
 // } 
 // else {
 //    throw std::invalid_argument("the side should match either buy or sell");
 // }
 
 // above is a non efficient way of implementation, can abstract certain steps, also dont allocate the node on the heap;
 if (side != Side::BUY && side != Side::SELL) {
  throw std::invalid_argument("orderType must be of either buy or sell");
 } 
 id++; //increment the global id counter
       
 OrderNode* ordn = new OrderNode(quantity, id, side);

 auto& levels = (side == Side::BUY) ? bidLevels : askLevels; // the ampersand here does not mean the address but rather that it is a reference to the level that was assigned to it.
 
 if (levels.find(price) == levels.end()) {
   levels.emplace(price, PriceLevel(price));
 }
 levels[price].addOrder(ordn);
 OrderHashMap.emplace(id, ordn); //put into an unordered map for O(1) seek
 
 ordn->price = price;
 ordn->parentLevel = &levels[price];
}

void LimitOrderBook::matchMarketOrder(uint_64_t quantity) {
  
}

void LimitOrderBook::cancelOrder(uint_64_t orderId) {

}

int LimitOrderBook::getBestPrice() const {
  return 0; //stub
}

uint64_t LimtiOrderBook::getVolumeAtPrice(double p) const {
  return 0; //stub
}

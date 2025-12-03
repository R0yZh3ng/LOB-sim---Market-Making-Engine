#include "LimitOrderBook.h"

#include <cstdint>

LimtiOrderBook::LimitOrderBook() : totalVolume(0) {} //maps automatically initiate empty

void LimitOrderBook::placeLimitOrder(double price, uint64_t quantity) {
  
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

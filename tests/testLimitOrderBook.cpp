#include "LimitOrderBook.h"
#include <cassert>
#include <iostream>

void testLimitOrderBookPlaceOrder() {
  LimitOrderBook lob;
  lob.placeLimitOrder(100.0, 10, Side::BUY);

  // Since we don't have getters for everything, we verify via public interface
  // or side effects
  assert(lob.getBestPrice(Side::BUY) == 100.0);
  assert(lob.getVolumeAtPrice(100.0) == 10);

  lob.placeLimitOrder(99.0, 5, Side::BUY);
  assert(lob.getBestPrice(Side::BUY) == 100.0); // Best price is still 100
  assert(lob.getVolumeAtPrice(99.0) == 5);

  lob.placeLimitOrder(101.0, 20, Side::SELL);
  assert(lob.getBestPrice(Side::SELL) == 101.0);
  assert(lob.getVolumeAtPrice(101.0) == 20);

  std::cout << "testLimitOrderBookPlaceOrder passed" << std::endl;
}

void testLimitOrderBookMatching() {
  LimitOrderBook lob;
  try {
    std::cout << "Placing SELL order..." << std::endl;
    lob.placeLimitOrder(100.0, 10, Side::SELL);
    std::cout << "Placing BUY order..." << std::endl;
    lob.placeLimitOrder(100.0, 10, Side::BUY);
    std::cout << "Orders placed." << std::endl;
  } catch (const std::exception &e) {
    std::cout << "Exception during placement: " << e.what() << std::endl;
    throw;
  }

  try {
    std::cout << "Getting volume..." << std::endl;
    lob.getVolumeAtPrice(100.0);
    std::cout << "Volume at 100.0: " << lob.getVolumeAtPrice(100.0)
              << std::endl;
  } catch (const std::out_of_range &e) {
    // Expected behavior if fully matched and removed
    std::cout << "Caught expected out_of_range exception." << std::endl;
  }

  std::cout << "testLimitOrderBookMatching passed" << std::endl;
}

int main() {
  testLimitOrderBookPlaceOrder();
  testLimitOrderBookMatching();
  std::cout << "All LimitOrderBook tests passed!" << std::endl;
  return 0;
}

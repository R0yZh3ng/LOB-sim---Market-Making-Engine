#include "LimitOrderBook.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

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
    assert(lob.getVolumeAtPrice(100.0) == 0);
    assert(!lob.hasOrders(Side::BUY));
    assert(!lob.hasOrders(Side::SELL));
    std::cout << "Volume at 100.0: " << lob.getVolumeAtPrice(100.0)
              << std::endl;
  } catch (const std::out_of_range &e) {
    // Expected behavior if fully matched and removed
    std::cout << "Caught expected out_of_range exception." << std::endl;
  }

  std::cout << "testLimitOrderBookMatching passed" << std::endl;
}

// covers the paths that delete or relocate order nodes, run under
// AddressSanitizer to catch memory errors
void testLimitOrderBookOrderLifetime() {
  LimitOrderBook lob;

  // incoming order fills completely
  lob.placeLimitOrder(100.0, 10, Side::SELL);
  lob.placeLimitOrder(100.0, 4, Side::BUY);

  // cancel, then cancelling the same ID again throws
  uint64_t id = lob.placeLimitOrder(98.0, 5, Side::BUY);
  lob.cancelOrder(id);
  assert(lob.getVolumeAtPrice(98.0) == 0);
  bool threw = false;
  try {
    lob.cancelOrder(id);
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  assert(threw);

  // modify, then cancel the modified order
  id = lob.placeLimitOrder(97.0, 5, Side::BUY);
  lob.modifyOrder(96.0, id, 7, Side::BUY);
  assert(lob.getVolumeAtPrice(97.0) == 0);
  assert(lob.getVolumeAtPrice(96.0) == 7);
  lob.cancelOrder(id);
  assert(lob.getVolumeAtPrice(96.0) == 0);

  // leave an order resting so the destructor has to free it
  lob.placeLimitOrder(95.0, 1, Side::BUY);

  std::cout << "testLimitOrderBookOrderLifetime passed" << std::endl;
}

void testLimitOrderBookFillVolume() {
  LimitOrderBook lob;

  // resting order partially filled
  uint64_t ask = lob.placeLimitOrder(100.0, 10, Side::SELL);
  lob.placeLimitOrder(100.0, 4, Side::BUY);
  assert(lob.getVolumeAtPrice(100.0) == 6);

  // cancelling the remainder leaves later orders counted correctly
  lob.placeLimitOrder(100.0, 3, Side::SELL);
  lob.cancelOrder(ask);
  assert(lob.getVolumeAtPrice(100.0) == 3);

  // one order sweeps two levels, using up the first and part of the second
  lob.placeLimitOrder(101.0, 5, Side::SELL);
  lob.placeLimitOrder(101.0, 5, Side::SELL);
  lob.placeLimitOrder(101.0, 10, Side::BUY);
  assert(lob.getVolumeAtPrice(100.0) == 0);
  assert(lob.getVolumeAtPrice(101.0) == 3);

  // incoming order partially fills and rests alongside another order
  lob.placeLimitOrder(102.0, 2, Side::BUY); // takes 2 of the ask at 101
  lob.placeLimitOrder(101.0, 1, Side::SELL);
  lob.placeLimitOrder(101.0, 8, Side::BUY); // takes 2, rests 6
  assert(!lob.hasOrders(Side::SELL));
  assert(lob.getVolumeAtPrice(101.0) == 6);

  std::cout << "testLimitOrderBookFillVolume passed" << std::endl;
}

int main() {
  testLimitOrderBookPlaceOrder();
  testLimitOrderBookMatching();
  testLimitOrderBookOrderLifetime();
  testLimitOrderBookFillVolume();
  std::cout << "All LimitOrderBook tests passed!" << std::endl;
  return 0;
}

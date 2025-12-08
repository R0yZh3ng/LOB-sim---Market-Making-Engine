#include "OrderNode.h"
#include <cassert>
#include <iostream>

void testOrderNodeConstruction() {
  OrderNode order(100, 1, Side::BUY);
  assert(order.quantity == 100);
  assert(order.orderId == 1);
  assert(order.side == Side::BUY);
  assert(order.next == nullptr);
  assert(order.previous == nullptr);
  std::cout << "testOrderNodeConstruction passed" << std::endl;
}

void testOrderNodeModification() {
  OrderNode order(100, 1, Side::SELL);
  order.quantity = 50;
  assert(order.quantity == 50);

  order.price = 105.5;
  assert(order.price == 105.5); // Warning: float comparison

  std::cout << "testOrderNodeModification passed" << std::endl;
}

int main() {
  testOrderNodeConstruction();
  testOrderNodeModification();
  std::cout << "All OrderNode tests passed!" << std::endl;
  return 0;
}

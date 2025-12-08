#include "OrderNode.h"
#include "PriceLevel.h"
#include <cassert>
#include <iostream>

void testPriceLevelAddOrder() {
  PriceLevel level(100);
  OrderNode *order1 = new OrderNode(10, 1, Side::BUY);
  OrderNode *order2 = new OrderNode(20, 2, Side::BUY);

  level.addOrder(order1);
  assert(level.head == order1);
  assert(level.tail == order1);
  assert(level.totalVolume == 10);
  assert(level.orderCount == 1);

  level.addOrder(order2);
  assert(level.head == order1);
  assert(level.tail == order2);
  assert(level.totalVolume == 30);
  assert(level.orderCount == 2);
  assert(order1->next == order2);
  assert(order2->previous == order1);

  std::cout << "testPriceLevelAddOrder passed" << std::endl;
  // Cleanup not strictly necessary for simple test but good practice
  // Note: PriceLevel doesn't own nodes for deletion in this test context unless
  // we pop
}

void testPriceLevelRemoveOrder() {
  PriceLevel level(100);
  OrderNode *order1 = new OrderNode(10, 1, Side::BUY);
  OrderNode *order2 = new OrderNode(20, 2, Side::BUY);
  OrderNode *order3 = new OrderNode(30, 3, Side::BUY);

  level.addOrder(order1);
  level.addOrder(order2);
  level.addOrder(order3);

  // Remove middle
  level.removeOrder(order2);
  assert(level.totalVolume == 40);
  assert(level.orderCount == 2);
  assert(order1->next == order3);
  assert(order3->previous == order1);

  // Remove head
  level.removeOrder(order1);
  assert(level.head == order3);
  assert(level.totalVolume == 30);

  // Remove tail
  level.removeOrder(order3);
  assert(level.head == nullptr);
  assert(level.tail == nullptr);
  assert(level.isEmpty());

  std::cout << "testPriceLevelRemoveOrder passed" << std::endl;
}

int main() {
  testPriceLevelAddOrder();
  testPriceLevelRemoveOrder();
  std::cout << "All PriceLevel tests passed!" << std::endl;
  return 0;
}

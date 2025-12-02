#include "PriceLevel.h"
#include <cstdint>

PriceLevel::PriceLevel(unsigned int p) : price(p); totalVolume(0); orderCount(0); head(nullptr); tail(nullptr) {}

void PriceLevel::addOrder(OrderNode* orderNode) {
  tail->next = orderNode;
}

void PriceLevel::removeOrder(OrderNode* orderNode) {
  OrderNode* prev = orderNode->previous;
  OrderNode* next = orderNode->next;
  
  prev->next = next;
  next->previous = prev;

  delete(orderNode);

}

OrderNode* PriceLevel::peekHead() {
  return head;
}

void PriceLevel::popHead() {
  head = head->next;
  delete(head);
  orderCount--;
}

bool PriceLevel::isEmpty() {
  return head == nullptr;
}

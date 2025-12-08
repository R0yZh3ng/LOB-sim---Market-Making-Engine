#include "PriceLevel.h"
#include "OrderNode.h"
#include <cstdint>
#include <stdexcept>

PriceLevel::PriceLevel(double p)
    : price(p), totalVolume(0), orderCount(0), head(nullptr), tail(nullptr) {}

void PriceLevel::addOrder(OrderNode *orderNode) {
  orderNode->next =
      nullptr; // this is incase that for some reason the orderNode was used
               // somewhere else with a linked list that may carry over, called
               // sanitizeing the pointer values
  orderNode->previous = tail;

  if (isEmpty()) {
    head = orderNode;
  } else {
    tail->next = orderNode;
  }
  tail = orderNode;
  totalVolume += orderNode->quantity;
  orderCount++;
}
void PriceLevel::removeOrder(OrderNode *orderNode) {
  if (orderNode == nullptr) {
    throw std::invalid_argument(
        "the orderNode is not suppose to be non-existent on the price level");
  }

  if (isEmpty()) {
    throw std::invalid_argument("the PriceLevel is not suppose to be empty");
  }

  totalVolume -= orderNode->quantity;
  orderCount--;

  // case where there is only one element
  if (head == tail) {
    // in this case that there is only one element in the price level, so it is
    // implied that removing this order would just clear the queue
    head = nullptr;
    tail = nullptr;
    delete orderNode;
    return;
  }
  // case where node to remove is the head
  if (head == orderNode) {
    head = head->next;
    head->previous = nullptr; // dont forget to make the previous head nullptr
                              // after changing to the next in line
    delete orderNode;
    return;
  }
  // case where the node to remove is the tail
  if (tail == orderNode) {
    tail = tail->previous;
    tail->next = nullptr; // dont forget to make the previous tail pointer
                          // nullptr after changing to previous element
    delete orderNode;
    return;
  }

  // case for where the node to remove is in the middle of the queue
  OrderNode *prev = orderNode->previous;
  OrderNode *next = orderNode->next;
  prev->next = next;
  next->previous = prev;

  delete orderNode;
}

OrderNode *PriceLevel::peekHead() {
  if (head != nullptr) {
    return head;
  }
  return nullptr;
}

void PriceLevel::popHead() {
  if (head == nullptr) {
    throw std::invalid_argument("cannot pop a already empty price level");
  }

  OrderNode *temp = head;

  if (head == tail) {
    head = nullptr;
    tail = nullptr;
  } else {
    head = head->next;
    head->previous = nullptr;
  }
  totalVolume -= temp->quantity;
  orderCount--;
  delete temp;
}

bool PriceLevel::isEmpty() { return head == nullptr; }

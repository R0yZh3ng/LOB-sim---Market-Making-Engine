#include "OrderNode.h"

#include <cstdint>


OrderNode::OrderNode(unsigned int q) : orderId(next_id++), quantity(q), next(nullptr), previous(nullptr) {}
// this is the optimal way to write a constructor as it assigns at construction which makes it faster and is required for const, references, or memebers without default constructors
// every production c++ codebase uses this format


void OrderNode::setQuantity(unsigned int q) {
  quantity = q;
}

void OrderNode::reduceQuantity(unsigned int amt) {
  quantity -= amt;
}

bool OrderNode::isFilled() const {
  return quantity == 0;
}

void OrderNode::setNext(OrderNode* n) {
  next = n;
}

void OrderNode::setPrevious(OrderNode* n) {
  previous = n;
}



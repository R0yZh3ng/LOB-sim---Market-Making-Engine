// Order class design
//
// REQUIRES:
// - orderID
// - quantity
// - node pointer to previous
// - node pointer to next
//
//

#ifndef ORDERNODE_H
#define ORDERNODE_H
// header guards to prevent duplicate definitionns if the file is included
// multiple times

#include <cstdint> // for uint64_t

enum struct Side {
  BUY,
  SELL
}; // enum struct is just a way of naming a variable type so youre not stuck
   // with conventional types that are unclear, like using bool or 0 and 1 for
   // this type of situation here

struct PriceLevel;

struct OrderNode {

public:
  double price;
  uint64_t orderId;
  unsigned int quantity;
  Side side;

  OrderNode *next;
  OrderNode *previous;
  PriceLevel *parentLevel;

  OrderNode(unsigned int q, uint64_t id, Side s);

  void setQuantity(unsigned int q);
  void reduceQuantity(unsigned int amt);
  bool isFilled() const;
  void setNext(OrderNode *n);
  void setPrevious(OrderNode *n);
};

#endif

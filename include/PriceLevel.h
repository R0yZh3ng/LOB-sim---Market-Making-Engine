#ifndef PRICELEVEL_H
#define PRICELEVEL_H

// #include "OrderNode.h" this is not needed if only using pointers of the
// struct, also avoid circular dependencies
#include <cstdint>

struct OrderNode;

struct PriceLevel {
public:
  double price;
  uint64_t totalVolume;
  unsigned int orderCount;

  OrderNode *head;
  OrderNode *tail;

  // constructor
  PriceLevel(double price);

  // methods

  void addOrder(OrderNode *orderNode);
  void removeOrder(OrderNode *orderNode);
  void unlinkOrder(OrderNode *orderNode); // same bookkeeping as removeOrder but
                                          // does not delete the node, for
                                          // relocating an order to another
                                          // price level (modify)

  OrderNode *peekHead();
  void popHead();

  bool isEmpty(); // bool is avaiable but the return value is 0 for false and 1
                  // for true, can assign the true and false to it directly
};

#endif

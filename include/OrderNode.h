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
// header guards to prevent duplicate definitionns if the file is included multiple times

#include <cstdint> // for uint64_t

struct OrderNode {

public:
  uint64_t orderId; 
  unsigned int quantity;
  OrderNode* next;
  OrderNode* previous;

  OrderNode(unsigned int q);


};



#endif

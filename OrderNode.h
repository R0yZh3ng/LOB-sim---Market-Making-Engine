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

#include <csrdint> // for uint64_t

struct OrderNode {

public:
  uint64_t orderId; 
  unsigned int quantity;
  OrderNode* next;
  OrderNode* previous;

  OrderNode(unsigned int q);

private:
  static uint64_t next_id;
  //globally shared counter for all OrderNode instances to make sure that all orderIds are unique, the 64 bit makes sure that it will not overflow within any reasonable time frame of orders
  //initialize the next_id counter in the .cpp file this header file corresponds to

};



#endif

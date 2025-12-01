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


struct OrderNode {

public:
  unsigned long orderId; 
  unsigned int quantity;
  OrderNode* next;
  OrderNode* previous;

  OrderNode(unsigned int q);
  
  void setQuantity(unsigned int q);
  void reduceQuantity(unsigned int amt);
  bool isFilled() const; //reminder that the const label is not that the return is constant, but rather that the function will not change the object

  void setNext(OrderNode* n);
  void setPrevious(OrderNode* n);

private:
  static uint64_t next_id;
  //globally shared counter for all OrderNode instances to make sure that all orderIds are unique, the 64 bit makes sure that it will not overflow within any reasonable time frame of orders
  //initialize the next_id counter in the .cpp file this header file corresponds to

};



#endif

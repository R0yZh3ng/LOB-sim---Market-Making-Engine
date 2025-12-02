#ifndef PRICELEVEL_H
#define PRIRCELEVEL_H 

// #include "OrderNode.h" this is not needed if only using pointers of the struct, also avoid circular dependencies
#include <cstdint>

struct PriceLevel {
  public:
    unsigned int price;
    uint64_t totalVolume;
    unsigned int orderCount;

    OrderNode* head;
    OrderNode* tail;
    
    //constructor
    PriceLevel(unsigned int price);
    
    //methods
    
    void addOrder(OrderNode* orderNode);
    void removeOrder(OrderNode* orderNode);

    OrderNode* peekHead();
    void popHead();

    bool isEmpty();
}

#endif

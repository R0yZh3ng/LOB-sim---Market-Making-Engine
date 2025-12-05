//some functions that need to be implemented
//
// PlaceOrder(order) - takes an order object and either fills it or places it in the limit book, prints trades that have taken place as a result
// CancelOrder(order) - takes an orderId and cancels it if it has nto yet been filled, otherwise its a no-op
// GetVolumeAtPrice(price, buyingOrSelling) - Get volume of open orders for either buying or selling side of order book 
// 
  
#ifndef LIMITORDERBOOK_H
#define LIMITORDERBOOK_H

#include "OrderNode.h"

#include <cstdint>
  

struct LimitOrderBook {
private:
    std::map<double, PriceLevel> bidLevels; //descending because this is the buy side
    std::map<double, PriceLevel> askLevels; //ascending because thit si the sell sied, no pointers to price level because it should live inside the map
    std::unordered_map<uint64_t, OrderNode*> OrderHashMap; //does not need two of these because this is only mean to do fast lookup, so add both ask and bid orders in here too. pointer to node so it gets to the reference
    //maintaining two maps to ensure fast lookup in case of deletions and order changes
    //O(1) worst case compared to O(n) worst case with a ordered map, memory overhead is negligible
    
    static uint64_t nextOrderId;

    uint64_t totalVolume;

    void placeOrder(double price, uint64_t quantity, Side side);
    void removePriceLevelIfEmpty(double price); 
    void fillOrder(OrderNode* incomingOrder); // this funciton when called match the ask and bid side if possible
public:
    LimitOrderBook();
  
    void placeLimitOrder(double price, uint64_t quantity, Side side);
    void placeMarketOrder(uint64_t quantity, Side side);
    void cancelOrder(uint64_t orderId);
    void modifyOrder(double price, uint64_t orderId, uint64_t quantity, Side side);
    double getBestPrice(Side side) const;
    uint64_t getVolumeAtPrice(double p) const;

}
#endif

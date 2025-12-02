//some functions that need to be implemented
//
// PlaceOrder(order) - takes an order object and either fills it or places it in the limit book, prints trades that have taken place as a result
// CancelOrder(order) - takes an orderId and cancels it if it has nto yet been filled, otherwise its a no-op
// GetVolumeAtPrice(price, buyingOrSelling) - Get volume of open orders for either buying or selling side of order book 
// 
  
#ifndef LIMITORDERBOOK_H
#define LIMITORDERBOOK_H

#include <cstdint>
  

struct LimitOrderBook {
private:
    std::map<double, PriceLevel*> levels;
    std::unordered_map<uint64_t, OrderNode* n> orderHashMap;
    //maintaining two maps to ensure fast lookup in case of deletions and order changes
    //O(1) worst case compared to O(n) worst case with a ordered map, memory overhead is negligible

    uint64_t totalVolume;
  
public:
    LimitOrderBook();
  
    void placeLimitOrder(double price, uint64_t quantity);
    void matchMarketOrder(uint64_t quantity);
    void cancelOrder(uint64_t orderId);
    int getBestPrice() const;
    uint64_t getVolumeAtPrice(double p) const;

}
#endif

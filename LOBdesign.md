## Definition

Limit order book contains prices and corresponding volumes (number of shares) with which people want to buy a given stock:

- bid side represents open offers to buy
- ask side represents open offers to sell
- trades are made when the highest bid >= lowest ask (spread is crossed)
- price at which trade is executed is that of the trade already in the order book
- if client submits a buy or sell order that cannot be filled, it gets stored in the order book
- orders are executed at the best possible price first, and if many order have the same price, the one that was submitted first is filled (FIFO)

## Data structures for limit order

# thinking about the process of submitting a buy order

- first check the lowest price of the sell side of the limit book
- if the lowest price of the sell side is less than or equal to the buy side, execute a trade
- if the buyer still has more volume left to fill, look at the next lowest price on the sell side and keep going
- if there is unfilled volume for the buyers trade, add it to the buyer heap

- using a min heap for the sell side and a max heap for the buy side
# the issue with this is that max and min heaps only maintain the order of a single element
# the best practice for real amtching engines are
- price ladder + hashmaps
  a map from a price to a doubly linked fifo queue
  with 1. a bid map sorted descending
       2. a ask map sorted ascending

       the maps would be mapped to price as keys and price levels as values, where price levels are a struct that contains the pointer the head, tail or both nodes of a priority queue made up by a doubly linked list as well as the volume at this price level, with the queues consisting of orderNodes. orderNodes contain the order id, quantity, and related node pointers in a doubly linekd list

       since hashmaps are dynamic in c++ new price level orderes are jsut added to the map so you dont need ot worry about a maintaining all possible price raneg orderes

because priority is given to the earlier time stamp, need to take into account flfo, so cannot just add a bunch of order nodes in the heap, but rather queues of order nodes in the heap

## Getting volume

this call needs to happen as a fast as possible, looping through every element of a queue would be inefficient and slow, instead consider keeping a hashmap that kept track of the volume at each price, and incremented/decremented the volume counter when order were added and cancelled from the limit book this would result in o(1) complexity

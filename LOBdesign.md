## Definition

Limit order book contains prices and corresponding volumes (number of shares) with which people want to buy a given stock:

- bid side represents open offers to buy
- ask side represents open offers to sell
- trades are made when the highest bid >= lowest ask (spread is crossed)
- price at which trade is executed is that of the trade already in the order book
- if client submits a buy or sell order that cannot be filled, it gets stored in the order book
- orders are executed at the best possible price first, and if many order have the same price, the one that was submitted first is filled (FIFO)

#include "simpleMM.h"
#include <cmath>

simpleMM::simpleMM(LimitOrderBook& lob, double spread, double size, double invRisk)
  : lob(lob), spread(spread), size(size), invRisk(invRisk) {}

void simpleMM::update(double fairPrice) {
  if (bidOrderId != static_cast<uint64_t>(-1)) lob.cancelOrder(bidOrderId);
  if (askOrderId != static_cast<uint64_t>(-1)) lob.cancelOrder(askOrderId);

  double invAdj = inventory * invRisk;

  double bidPrice = fairPrice - spread - invAdj;
  double askPrice = fairPrice + spread - invAdj;

  bidOrderId = lob.placeLimitOrder(bidPrice, size, Side::BUY);
  askOrderId = lob.placeLimitOrder(askPrice, size, Side::SELL);

  inventory = 0;//placeholder for when the risk and clearing system get implemented to keep track of orders
}

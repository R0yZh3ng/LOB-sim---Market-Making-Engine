#include "simpleMM.h"
#include <cmath>

simpelMM::simpleMM(LimitOrderBook& lob, double spread, double size, double invRisk)
  : lob(lob), spread(spread), size(size), invRisk(invRisk) {}

void update(double fairPrice) {
  if (bidOrderId != -1) lob.cancelOrder(bidOrderId);
  if (askOrderId != -1) lob.cancelOrder(askOrderId);

  int invAdj = inventory * invRisk;

  double bidPrice = fairPrice - spread - invAdj;
  double askPrice = fairPrice + spread - invAdj;

  bidOrderId = lob.placeLimitOrder(bidPrice, size, BUY);
  askOrderId = lob.placeLimitOrder(askPrice, size, SELL);

  inventory = 0;//placeholder for when the risk and clearing system get implemented to keep track of orders
}

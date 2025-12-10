#pragma once

#include "LimitOrderBook.h"
#include <cstdint>

struct simpleMM {
public:
  simpleMM(LimitOrderBook& lob, double spread, double size, double invRisk);
  void update(double fairPrice);

private:
  LimitOrderBook& lob;

  double spread;
  double size;
  double invRisk;
  
  uint64_t bidOrderId = -1;
  uint64_t askOrderId = -1;
  //these placeOrder ids are here to make sure that the mm has a previous order;

  double inventory = 0;

}

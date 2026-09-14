#pragma once
// Drives synthetic, parameter-controlled order flow through a
// LimitOrderBook and records per-event book state, so downstream analysis
// can study mid-price evolution, spread dynamics, and the relationship
// between top-of-book queue imbalance and short-term price impact.

#include "LimitOrderBook.h"
#include "OrnsteinUhlenbeck.h"
#include <cstdint>
#include <random>
#include <vector>

struct SimulationConfig {
  int numEvents = 20000;

  // "Controlled order flow": the caller fixes these knobs per scenario.
  double buyProbability = 0.5;         // P(incoming order is a BUY)
  double aggressiveProbability = 0.15; // P(order is marketable / crosses spread)

  double initialFairPrice = 100.0;
  double ouTheta = 100.0; // long-run mean fair price
  double ouKappa = 0.05;  // mean-reversion strength
  double ouSigma = 0.05;  // volatility
  double ouDt = 1.0;

  double tickSize = 0.01;
  double minQty = 10;
  double maxQty = 200;
  double passiveOffsetTicks = 25; // how far from fair price resting orders sit

  double cancelChurnProbability = 0.35; // per-event chance to cancel a resting order
  int seedLevels = 25;   // resting price levels seeded on each side at start
  int seedOrdersPerLevel = 4;

  unsigned int seed = 42;
};

struct EventRecord {
  int eventIndex;
  double fairPrice;
  double midPriceBefore;
  double spreadBefore;
  double bidVolumeBefore;
  double askVolumeBefore;
  double imbalanceBefore; // (bidVol - askVol) / (bidVol + askVol), in [-1, 1]
  double midPriceAfter;
  double priceChange; // midPriceAfter - midPriceBefore, one event ahead
  bool aggressive;
};

class OrderFlowSimulator {
public:
  explicit OrderFlowSimulator(const SimulationConfig &cfg);

  // Runs the full simulation and returns one record per event where the
  // book had two-sided liquidity (needed to define mid/spread/imbalance).
  std::vector<EventRecord> run();

private:
  SimulationConfig cfg;
  LimitOrderBook lob;
  OrnsteinUhlenbeck ou;
  std::mt19937 rng;
  std::uniform_real_distribution<double> uniform01;
  std::vector<uint64_t> restingOrderIds;

  void seedBook();
  double roundToTick(double price) const;
  double currentMid() const;
  double currentSpread() const;
};

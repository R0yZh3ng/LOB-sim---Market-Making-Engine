#include "OrderFlowSimulator.h"
#include <cmath>
#include <stdexcept>

OrderFlowSimulator::OrderFlowSimulator(const SimulationConfig &c)
    : cfg(c), lob(), ou(c.initialFairPrice, c.ouTheta, c.ouKappa, c.ouSigma, c.ouDt),
      rng(c.seed), uniform01(0.0, 1.0) {}

double OrderFlowSimulator::roundToTick(double price) const {
  return std::round(price / cfg.tickSize) * cfg.tickSize;
}

double OrderFlowSimulator::currentMid() const {
  return (lob.getBestPrice(Side::BUY) + lob.getBestPrice(Side::SELL)) / 2.0;
}

double OrderFlowSimulator::currentSpread() const {
  return lob.getBestPrice(Side::SELL) - lob.getBestPrice(Side::BUY);
}

void OrderFlowSimulator::seedBook() {
  double fair = cfg.initialFairPrice;
  std::uniform_real_distribution<double> qtyDist(cfg.minQty, cfg.maxQty);

  for (int lvl = 1; lvl <= cfg.seedLevels; lvl++) {
    double bidPrice = roundToTick(fair - lvl * cfg.tickSize);
    double askPrice = roundToTick(fair + lvl * cfg.tickSize);
    for (int k = 0; k < cfg.seedOrdersPerLevel; k++) {
      restingOrderIds.push_back(lob.placeLimitOrder(
          bidPrice, static_cast<uint64_t>(qtyDist(rng)), Side::BUY));
      restingOrderIds.push_back(lob.placeLimitOrder(
          askPrice, static_cast<uint64_t>(qtyDist(rng)), Side::SELL));
    }
  }
}

std::vector<EventRecord> OrderFlowSimulator::run() {
  seedBook();

  std::vector<EventRecord> records;
  records.reserve(cfg.numEvents);

  std::uniform_real_distribution<double> qtyDist(cfg.minQty, cfg.maxQty);
  std::uniform_int_distribution<int> offsetDist(
      1, static_cast<int>(cfg.passiveOffsetTicks));

  for (int i = 0; i < cfg.numEvents; i++) {
    double fair = ou.update();

    // Cancellation churn: keeps the book from growing without bound and
    // mimics real order flow, which is dominated by cancels/replaces.
    if (!restingOrderIds.empty() &&
        uniform01(rng) < cfg.cancelChurnProbability) {
      std::uniform_int_distribution<size_t> pick(0, restingOrderIds.size() - 1);
      size_t idx = pick(rng);
      uint64_t id = restingOrderIds[idx];
      restingOrderIds[idx] = restingOrderIds.back();
      restingOrderIds.pop_back();
      try {
        lob.cancelOrder(id);
      } catch (const std::exception &) {
        // already filled earlier by matching -- not an error, just skip
      }
    }

    if (!lob.hasOrders(Side::BUY) || !lob.hasOrders(Side::SELL)) {
      // Churn emptied one side; top both sides up and skip this event's
      // stats since mid/spread/imbalance are undefined without two sides.
      double bidPrice = roundToTick(fair - cfg.tickSize);
      double askPrice = roundToTick(fair + cfg.tickSize);
      restingOrderIds.push_back(lob.placeLimitOrder(
          bidPrice, static_cast<uint64_t>(qtyDist(rng)), Side::BUY));
      restingOrderIds.push_back(lob.placeLimitOrder(
          askPrice, static_cast<uint64_t>(qtyDist(rng)), Side::SELL));
      continue;
    }

    double bestBid = lob.getBestPrice(Side::BUY);
    double bestAsk = lob.getBestPrice(Side::SELL);
    double midBefore = (bestBid + bestAsk) / 2.0;
    double spreadBefore = bestAsk - bestBid;
    double bidVol = static_cast<double>(lob.getVolumeAtPrice(bestBid));
    double askVol = static_cast<double>(lob.getVolumeAtPrice(bestAsk));
    double imbalance =
        (bidVol + askVol) > 0 ? (bidVol - askVol) / (bidVol + askVol) : 0.0;

    bool buy = uniform01(rng) < cfg.buyProbability;
    bool aggressive = uniform01(rng) < cfg.aggressiveProbability;
    Side side = buy ? Side::BUY : Side::SELL;
    uint64_t qty = static_cast<uint64_t>(qtyDist(rng));

    double price;
    if (aggressive) {
      // Marketable order: priced to cross and sweep the opposite side.
      price = buy ? bestAsk : bestBid;
    } else {
      // Passive order resting near (not at) the fair price.
      int offsetTicks = offsetDist(rng);
      price = buy ? roundToTick(fair - offsetTicks * cfg.tickSize)
                  : roundToTick(fair + offsetTicks * cfg.tickSize);
    }

    uint64_t orderId = lob.placeLimitOrder(price, qty, side);
    restingOrderIds.push_back(orderId);

    double midAfter = (lob.hasOrders(Side::BUY) && lob.hasOrders(Side::SELL))
                           ? currentMid()
                           : midBefore;

    EventRecord rec;
    rec.eventIndex = i;
    rec.fairPrice = fair;
    rec.midPriceBefore = midBefore;
    rec.spreadBefore = spreadBefore;
    rec.bidVolumeBefore = bidVol;
    rec.askVolumeBefore = askVol;
    rec.imbalanceBefore = imbalance;
    rec.midPriceAfter = midAfter;
    rec.priceChange = midAfter - midBefore;
    rec.aggressive = aggressive;
    records.push_back(rec);
  }

  return records;
}

// Mid-price evolution, spread dynamics, and queue-imbalance -> short-term
// price-impact study, run under several controlled order-flow scenarios.
//
// For each scenario we fix the order-flow parameters (buy/sell skew,
// aggressive-order rate) and run the LimitOrderBook forward for N events,
// recording top-of-book state before each event. We then measure:
//   1. mid-price / spread dynamics (volatility, mean spread)
//   2. correlation + OLS slope between top-of-book queue imbalance and the
//      forward mid-price change over 1, 5, 10, and 20 events
//
// Output: a console report plus one CSV per scenario under analysis/output/
// for external verification/plotting.

#include "OrderFlowSimulator.h"
#include "Stats.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct Scenario {
  std::string name;
  double buyProbability;
};

static void writeCsv(const std::string &path,
                      const std::vector<EventRecord> &records) {
  std::ofstream out(path);
  out << "event,fair_price,mid_before,spread_before,bid_vol,ask_vol,"
         "imbalance,mid_after,price_change,aggressive\n";
  out << std::fixed << std::setprecision(6);
  for (const auto &r : records) {
    out << r.eventIndex << ',' << r.fairPrice << ',' << r.midPriceBefore
        << ',' << r.spreadBefore << ',' << r.bidVolumeBefore << ','
        << r.askVolumeBefore << ',' << r.imbalanceBefore << ','
        << r.midPriceAfter << ',' << r.priceChange << ','
        << (r.aggressive ? 1 : 0) << '\n';
  }
}

// Builds (imbalance_t, mid_{t+k} - mid_t) pairs for a forward horizon of k
// events, using each record's pre-event mid price as the price series.
static void buildForwardPair(const std::vector<EventRecord> &records, int k,
                              std::vector<double> &imbalanceOut,
                              std::vector<double> &forwardChangeOut) {
  imbalanceOut.clear();
  forwardChangeOut.clear();
  if (static_cast<int>(records.size()) <= k)
    return;
  imbalanceOut.reserve(records.size() - k);
  forwardChangeOut.reserve(records.size() - k);
  for (size_t i = 0; i + k < records.size(); i++) {
    imbalanceOut.push_back(records[i].imbalanceBefore);
    forwardChangeOut.push_back(records[i + k].midPriceBefore -
                                records[i].midPriceBefore);
  }
}

static void runScenario(const Scenario &scenario) {
  SimulationConfig cfg;
  cfg.numEvents = 20000;
  cfg.buyProbability = scenario.buyProbability;
  cfg.seed = 42; // fixed seed: differences across scenarios are attributable
                 // to the order-flow parameter, not RNG noise

  OrderFlowSimulator sim(cfg);
  std::vector<EventRecord> records = sim.run();

  std::vector<double> midSeries, spreadSeries, returns;
  midSeries.reserve(records.size());
  spreadSeries.reserve(records.size());
  for (const auto &r : records) {
    midSeries.push_back(r.midPriceBefore);
    spreadSeries.push_back(r.spreadBefore);
  }
  for (size_t i = 1; i < midSeries.size(); i++)
    returns.push_back(midSeries[i] - midSeries[i - 1]);

  std::cout << "\n=== Scenario: " << scenario.name
            << " (buyProbability=" << scenario.buyProbability << ") ===\n";
  std::cout << "  events recorded:        " << records.size() << "\n";
  std::cout << "  mean mid price:          " << mean(midSeries) << "\n";
  std::cout << "  mid-price step std dev:  " << stddev(returns)
            << "  (per-event volatility)\n";
  std::cout << "  mean spread:             " << mean(spreadSeries) << "\n";
  std::cout << "  spread std dev:          " << stddev(spreadSeries) << "\n";

  std::cout << "  queue imbalance -> forward price impact:\n";
  for (int k : {1, 5, 10, 20}) {
    std::vector<double> imb, fwd;
    buildForwardPair(records, k, imb, fwd);
    double corr = pearsonCorrelation(imb, fwd);
    OlsResult ols = olsRegression(imb, fwd);
    std::cout << "    k=" << std::setw(2) << k
              << "  corr(imbalance, fwd_dP)=" << std::setw(8)
              << std::fixed << std::setprecision(4) << corr
              << "   slope=" << std::setw(9) << ols.slope
              << "   R^2=" << std::setw(7) << ols.r2 << "\n";
  }

  std::filesystem::create_directories("analysis/output");
  std::string path = "analysis/output/price_impact_" + scenario.name + ".csv";
  writeCsv(path, records);
  std::cout << "  wrote " << records.size() << " rows to " << path << "\n";
}

int main() {
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "Price-impact / mid-price-dynamics study\n";
  std::cout << "----------------------------------------\n";

  std::vector<Scenario> scenarios = {
      {"neutral_flow", 0.50},
      {"buy_heavy_flow", 0.65},
      {"sell_heavy_flow", 0.35},
  };

  for (const auto &s : scenarios)
    runScenario(s);

  std::cout << "\nDone.\n";
  return 0;
}

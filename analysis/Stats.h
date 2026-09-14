#pragma once
// Small dependency-free statistics helpers used by the analysis programs.
// Kept out of the core engine on purpose -- this is analysis tooling, not
// matching-engine logic.

#include <cmath>
#include <cstddef>
#include <vector>

struct OlsResult {
  double slope;
  double intercept;
  double r2;
};

inline double mean(const std::vector<double> &x) {
  if (x.empty())
    return 0.0;
  double s = 0.0;
  for (double v : x)
    s += v;
  return s / static_cast<double>(x.size());
}

inline double stddev(const std::vector<double> &x) {
  if (x.size() < 2)
    return 0.0;
  double m = mean(x);
  double s = 0.0;
  for (double v : x)
    s += (v - m) * (v - m);
  return std::sqrt(s / static_cast<double>(x.size() - 1));
}

// Pearson correlation coefficient between x and y (same length).
inline double pearsonCorrelation(const std::vector<double> &x,
                                  const std::vector<double> &y) {
  size_t n = x.size();
  if (n != y.size() || n < 2)
    return 0.0;

  double mx = mean(x), my = mean(y);
  double num = 0.0, dx2 = 0.0, dy2 = 0.0;
  for (size_t i = 0; i < n; i++) {
    double dx = x[i] - mx;
    double dy = y[i] - my;
    num += dx * dy;
    dx2 += dx * dx;
    dy2 += dy * dy;
  }
  double denom = std::sqrt(dx2 * dy2);
  if (denom == 0.0)
    return 0.0;
  return num / denom;
}

// Ordinary least squares fit y = slope * x + intercept, plus R^2.
inline OlsResult olsRegression(const std::vector<double> &x,
                                const std::vector<double> &y) {
  size_t n = x.size();
  if (n != y.size() || n < 2)
    return {0.0, 0.0, 0.0};

  double mx = mean(x), my = mean(y);
  double num = 0.0, den = 0.0;
  for (size_t i = 0; i < n; i++) {
    num += (x[i] - mx) * (y[i] - my);
    den += (x[i] - mx) * (x[i] - mx);
  }
  double slope = den == 0.0 ? 0.0 : num / den;
  double intercept = my - slope * mx;

  double ssRes = 0.0, ssTot = 0.0;
  for (size_t i = 0; i < n; i++) {
    double pred = slope * x[i] + intercept;
    ssRes += (y[i] - pred) * (y[i] - pred);
    ssTot += (y[i] - my) * (y[i] - my);
  }
  double r2 = ssTot == 0.0 ? 0.0 : 1.0 - (ssRes / ssTot);
  return {slope, intercept, r2};
}

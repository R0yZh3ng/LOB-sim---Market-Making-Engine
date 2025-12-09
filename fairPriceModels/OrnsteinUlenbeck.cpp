#include "OrnsteinUhlenbeck.h"

OrnsteinUhlenbeck::OrnsteinUhlenbeck(double initial, double theta, double kappa, double sigma, double dt)
  : price(initial), theta(theta), kappa(kappa), sigma(sigma), dt(dt), rng(std::random_device{}()), normal(0.0, 1.0) {}

double OrnsteinUhlenbeck::update() {
  double dW = normal(rng) * std::sqrt(dt); //dW is the Brownian increment, multiplies a random number N(0,1) with sqrt(dt), the scale factor for brownian motion, thus dW is the increment of brownian motion with variance dt

  price += kappa * (theta - price) * dt + sigma * dW;
  return price;
}

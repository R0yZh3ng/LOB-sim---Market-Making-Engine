#pragma once // this is the same as the header guards but cleaner
             //
#include <random>

struct OrnsteinUhlenbeck {
  
private:
  double price;
  double theta; //long run mean - what the price tries to return to in the long run
  double kappa; //mean reversion strength, the strength of pull back to the mean
  double sigma; //volitility
  double dt;    // time step between updates
  
  std::mt19937 rng; // this is the mersenne twister 19937, its a pseudo random number generator,
  std::normal_distribution<double> normal;

public: 
  OrnsteinUhlenbeck(double initial, double theta, double kappa, double sigma, double dt);

  double update();
  double getPrice() const {return price};

}

#include <cstddef> // size_t
#include <utility> //pair
#include <random> // mt19937
#pragma once

struct HestonParameters {
    double S0;
    double K;
    double r;
    double T;
    double v0;
    double theta;
    double kappa;
    double xi;
    double rho;
};

struct PricingResult {
    double price;
    double std_error;
    double ci_lower;
    double ci_upper;
    double elapsed_seconds;
};

class HestonSimulator {
public:
    explicit HestonSimulator(const HestonParameters& params, unsigned int seed);

    PricingResult price_european_call(size_t num_paths, size_t num_steps);
    
private:
    std::pair<double, double> generateCorrelatedNormal(double rho);
    HestonParameters params_;
    std::mt19937 rng_;
    std::normal_distribution<double> normal_;
};
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
    double sigma;
    double rho;
};

struct PricingResult {
    double price;
    double std_error;
    double ci_lower;
    double ci_upper;
    double elapsed_seconds;
};

// IMPLEMENT THIS LATER ON
enum class VarianceScheme {
    FullTruncation,
    PartialTruncation,
    Reflection,
    Absortion
};

struct CorrelatedNormals {
    double z1;
    double z2;
};

struct QECoefficients {
    double K0;
    double K1;
    double K2;
    double K3;
    double K4;
};

class HestonSimulator {
public:
    explicit HestonSimulator(const HestonParameters& params, unsigned int seed);

    PricingResult price_european_call(size_t num_paths, size_t num_steps);
    
private:
    HestonParameters params_;

    // Euler-Maruyama with Full Truncation
    void eulerStep_(double &X, double &v, double dt, double sqrt_dt);
    void qeStep_(double &X, double &v, double gamma1, double gamma2, double dt);

    std::mt19937 rng_;
    CorrelatedNormals generateCorrelatedNormal(double rho);
    std::normal_distribution<double> normal_;
    std::uniform_real_distribution<double> uniform_;
};
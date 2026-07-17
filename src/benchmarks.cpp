#include "heston.hpp"
#include <format>
#include <iostream>

constexpr unsigned int RNG_SEED = 123;

int main() {
    HestonParameters params = {.S0 = 100.0,
                               .K = 100.0,
                               .r = 0.05,
                               .T = 1.0,
                               .v0 = 0.04,
                               .theta = 0.04,
                               .kappa = 2.0,
                               .sigma = 0.1,
                               .rho = -0.5};

    // std::vector<int> num_steps = {1'000, 5'000, 10'000, 50'000, 100'000, 500'000, 1'000'000};

    // struct BenchmarkResult {
    //     double
    // };

    HestonSimulator pricingEngine(params, RNG_SEED);
    PricingResult results =
        pricingEngine.price_european_call(1e4, 1e3, DiscretizationScheme::QuadraticExponential);

    return 0;
}
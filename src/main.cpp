#include <iostream>
#include "heston.hpp"
#include <iostream>
#include <format>

constexpr unsigned int RNG_SEED = 42;

void printResults(const PricingResult& results){
    std::cout << std::format("Call Price    :{:.4f}\n", results.price);
    std::cout << std::format("Std error     :{:.4f}\n", results.std_error);
    std::cout << std::format("95% CI        :[{:.4f}, {:.4f}]\n", results.ci_lower, results.ci_upper);
    std::cout << std::format("Exec Time     :{:.4f}\n", results.elapsed_seconds);
}

int main() {    
    HestonParameters params = {
        .S0 = 100.0,
        .K = 100.0,
        .r = 0.05,
        .T = 1.0,
        .v0 = 0.04,
        .theta = 0.04,
        .kappa = 2.0,
        .xi = 0.1,
        .rho = -0.5
    };

    HestonSimulator pricingEngine(params, RNG_SEED);
    PricingResult results = pricingEngine.price_european_call(1e4,1e3);

    printResults(results);

    return 0;
}
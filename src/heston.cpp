#include "heston.hpp"
#include <cmath>
#include <random>
#include <algorithm>
#include <chrono>

HestonSimulator::HestonSimulator(const HestonParameters& params, unsigned int seed): params_(params), rng_(seed), normal_(0.0, 1.0) {}

// Generate two normal correlated distributions
CorrelatedNormals HestonSimulator::generateCorrelatedNormal(double rho){
    const double rho_comp = std::sqrt(1.0 - params_.rho * params_.rho);
    double Z1 = normal_(rng_);
    double Z2 = normal_(rng_);

    double Z2_corr = rho * Z1 + rho_comp * Z2;
    
    return {Z1, Z2_corr};
}

PricingResult HestonSimulator::price_european_call(size_t num_paths, size_t num_steps){
    const double dt = params_.T / static_cast<double>(num_steps);
    const double sqrt_dt = std::sqrt(dt);

    double sum_payoff = 0;
    double sum_squared_payoff = 0;

    auto start = std::chrono::steady_clock::now();

    for(size_t path = 0; path < num_paths; path++){
        double X = std::log(params_.S0); // Taking log is more precise
        double v = params_.v0;

        for(size_t step = 0; step < num_steps; step++){
            CorrelatedNormals normals = generateCorrelatedNormal(params_.rho);
            eulerStep_(X, v, normals.z1, normals.z2, dt, sqrt_dt);
        }
        double payoff = std::max(std::exp(X) - params_.K, 0.0);

        sum_payoff += payoff;
        sum_squared_payoff += payoff * payoff;
    }

    auto end = std::chrono::steady_clock::now();
    
    PricingResult result{};
    const double n = static_cast<double>(num_paths);

    const double discount_factor = std::exp(-params_.r * params_.T);

    // PRICE
    const double mean_payoff = sum_payoff / n;
    result.price = mean_payoff * discount_factor;

    // STD ERROR
    double price_variance = (sum_squared_payoff - (sum_payoff * sum_payoff) / n) / (n - 1.0);
    if (price_variance < 0.0) price_variance = 0.0; 

    const double std_dev = std::sqrt(price_variance);
    result.std_error = discount_factor * (std_dev / std::sqrt(n));

    // CONFIDENCE INTERVAL
    constexpr double z95 = 1.96;
    result.ci_lower = result.price - z95 * result.std_error;
    result.ci_upper = result.price + z95 * result.std_error;

    // TIMING BENCHMARK
    result.elapsed_seconds = std::chrono::duration<double>(end - start).count();

    return result;
}

// Euler-Maruyama with Full Truncation
void HestonSimulator::eulerStep_(double &X, double &v, double z1, double z2, double dt, double sqrt_dt){
    const double v_truncated = std::max(v, 0.0);
    const double sqrt_v = std::sqrt(v_truncated);
    
    X += (params_.r - 0.5 * v_truncated) * dt + sqrt_v * sqrt_dt * z1;
    v += params_.kappa * (params_.theta - v_truncated) * dt + sqrt_v * params_.xi *sqrt_dt * z2;
}
#include "cuda/cuda_heston_pricer.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <random>

/* KERNEL  TEMPLATE FOR CUDA

__global__ void simulate_paths(...)
{
        int path_id = blockIdx.x * blockDim.x + threadIdx.x;

        if(path_id >= num_paths)
                return;

        double X = ...;
        double v = ...;

        for(step ...){

        }

        payoffs[path_id] = ...;
}
*/
// THIS SHOULD BECOME A CUDA KERNEL
inline void stepEuler(double &X, double &v, double dt, double sqrt_dt, double rho, double rho_comp,
                      double r, double kappa, double theta, double xi, std::mt19937 &rng,
                      std::normal_distribution<double> &norm) {
    const double z1 = norm(rng);
    const double z2_uncorr = norm(rng);
    const double z2 = rho * z1 + rho_comp * z2_uncorr;

    const double v_trunc = std::max(v, 0.0);
    const double sqrt_v = std::sqrt(v_trunc);

    X += (r - 0.5 * v_trunc) * dt + sqrt_v * sqrt_dt * z1;
    v += kappa * (theta - v_trunc) * dt + xi * sqrt_v * sqrt_dt * z2;
}

PricingResult CUDAHestonPricer::price(const HestonParameters &params, size_t num_paths,
                                      size_t num_steps, DiscretizationScheme scheme,
                                      unsigned int seed) {

    // IMPLEMENT THE FUNCTION HERE
    const double dt = params.T / static_cast<double>(num_steps);
    const double discount_factor = std::exp(-params.r * params.T);

    std::mt19937 rng(seed);
    std::normal_distribution<double> norm_dist(0.0, 1.0);
    std::uniform_real_distribution<double> unif_dist(0.0, 1.0);

    double sum_payoff = 0.0;
    double sum_squared_payoff = 0.0;

    auto start = std::chrono::steady_clock::now();

    const double sqrt_dt = std::sqrt(dt);
    const double rho_comp = std::sqrt(1.0 - params.rho * params.rho);

    // THIS IS GOING TO BE REPLACED
    for (size_t path = 0; path < num_paths; ++path) {
        double X = std::log(params.S0);
        double v = params.v0;

        for (size_t step = 0; step < num_steps; ++step) {
            stepEuler(X, v, dt, sqrt_dt, params.rho, rho_comp, params.r, params.kappa, params.theta,
                      params.xi, rng, norm_dist);
        }

        const double payoff = std::max(std::exp(X) - params.K, 0.0);
        sum_payoff += payoff;
        sum_squared_payoff += payoff * payoff;
    }

    auto end = std::chrono::steady_clock::now();

    // Statiscal Measures and results
    PricingResult result{};
    const double n = static_cast<double>(num_paths);

    const double mean_payoff = sum_payoff / n;
    result.price = mean_payoff * discount_factor;

    double price_variance = (sum_squared_payoff - (sum_payoff * sum_payoff) / n) / (n - 1.0);
    if (price_variance < 0.0)
        price_variance = 0.0;

    const double std_dev = std::sqrt(price_variance);
    result.std_error = discount_factor * (std_dev / std::sqrt(n));

    constexpr double z95 = 1.96;
    result.ci_lower = result.price - z95 * result.std_error;
    result.ci_upper = result.price + z95 * result.std_error;

    result.elapsed_seconds = std::chrono::duration<double>(end - start).count();

    return result;
}

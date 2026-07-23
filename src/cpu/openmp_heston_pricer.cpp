#include "cpu/openmp_heston_pricer.hpp"
#include "cpu/heston_steps.hpp" // Update functions (EM & QE)
#include <algorithm>
#include <chrono>
#include <cmath>
#include <omp.h> // OPEN MP
#include <random>
#include <stdexcept>

PricingResult OpenMPHestonPricer::price(const HestonParameters &params, size_t num_paths,
                                        size_t num_steps, DiscretizationScheme scheme,
                                        unsigned int seed) {
    // Common precalculations
    const double dt = params.T / static_cast<double>(num_steps);
    const double discount_factor = std::exp(-params.r * params.T);
    const double sqrt_dt = std::sqrt(dt);
    const double rho_comp = std::sqrt(1.0 - params.rho * params.rho);

    double sum_payoff = 0.0;
    double sum_squared_payoff = 0.0;

    auto start = std::chrono::steady_clock::now();

    /* It is better to put an if outside the loop rather than inside so it only runs once
    even if the code looks messier */
    if (scheme == DiscretizationScheme::EulerMaruyama) {

#pragma omp parallel reduction(+ : sum_payoff, sum_squared_payoff)
        {

            // Corresponding precalculations
            const int thread_id = omp_get_thread_num();
            std::mt19937_64 rng(seed + static_cast<unsigned int>(thread_id) * 100073U);
            std::normal_distribution<double> norm_dist(0.0, 1.0);
            // std::uniform_real_distribution<double> unif_dist(0.0, 1.0);

#pragma omp for schedule(static)
            for (size_t path = 0; path < num_paths; ++path) {
                double X = std::log(params.S0);
                double v = params.v0;

                for (size_t step = 0; step < num_steps; ++step) {

                    heston::internal::stepEuler(X, v, dt, sqrt_dt, params.rho, rho_comp, params.r,
                                                params.kappa, params.theta, params.xi, rng,
                                                norm_dist);
                }

                const double payoff = std::max(std::exp(X) - params.K, 0.0);
                sum_payoff += payoff;
                sum_squared_payoff += payoff * payoff;
            }
        }
    } else if (scheme == DiscretizationScheme::QuadraticExponential) {

        // Corresponding precalculations (QE Scheme, check references)

        QEPrecomputed qe{};
        qe.exp_kdt = std::exp(-params.kappa * dt);
        qe.one_minus_exp = 1.0 - qe.exp_kdt;

        const double xi2 = params.xi * params.xi;
        qe.A_const = (xi2 * qe.exp_kdt * qe.one_minus_exp) / params.kappa;
        qe.B_const =
            (params.theta * xi2 * qe.one_minus_exp * qe.one_minus_exp) / (2.0 * params.kappa);

        constexpr double gamma1 = 0.5;
        constexpr double gamma2 = 0.5;

        qe.K0 = -params.kappa * params.rho * params.theta * dt / params.xi;
        qe.K1 =
            (params.kappa * params.rho / params.xi - 0.5) * gamma1 * dt - params.rho / params.xi;
        qe.K2 =
            (params.kappa * params.rho / params.xi - 0.5) * gamma2 * dt + params.rho / params.xi;
        qe.K3 = (1.0 - params.rho * params.rho) * gamma1 * dt;
        qe.K4 = (1.0 - params.rho * params.rho) * gamma2 * dt;

#pragma omp parallel reduction(+ : sum_payoff, sum_squared_payoff)
        {
            const int thread_id = omp_get_thread_num();
            std::mt19937_64 rng(seed + static_cast<unsigned int>(thread_id) * 100073U);
            std::normal_distribution<double> norm_dist(0.0, 1.0);
            std::uniform_real_distribution<double> unif_dist(0.0, 1.0);

#pragma omp for schedule(static)
            for (size_t path = 0; path < num_paths; ++path) {
                double X = std::log(params.S0);
                double v = params.v0;

                for (size_t step = 0; step < num_steps; ++step) {
                    heston::internal::stepQE(X, v, dt, params, qe, rng, norm_dist, unif_dist);
                }

                const double payoff = std::max(std::exp(X) - params.K, 0.0);
                sum_payoff += payoff;
                sum_squared_payoff += payoff * payoff;
            }
        }
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
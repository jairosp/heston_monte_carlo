#include "cpu/cpu_heston_pricer.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <random>
#include <stdexcept>

// Precomputed invariants for QE Method (Andersen 2007)
struct QEPrecomputed {
    double exp_kdt;
    double one_minus_exp;
    double K0, K1, K2, K3, K4;
    double A_const, B_const;
};

namespace {
// Euler Maruyama Step (Optimized and inline)
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

// Quadratic Exponential Step (Andersen 2007, Inlined)
inline void stepQE(double &X, double &v, double dt, const HestonParameters &p,
                   const QEPrecomputed &qe, std::mt19937 &rng,
                   std::normal_distribution<double> &norm,
                   std::uniform_real_distribution<double> &unif) {
    const double v_prev = v;
    const double PSI_C = 1.5;

    // QE Andersen Algorithm (Check References for more details)
    double m = p.theta + (v_prev - p.theta) * qe.exp_kdt;
    double s2 = v_prev * qe.A_const + qe.B_const;

    // Prevents zero division errors
    if (p.kappa < 1e-8) {
        m = v_prev;
        s2 = v_prev * p.xi * p.xi * dt;
    }

    const double psi = s2 / (m * m);

    if (psi <= PSI_C) {
        // Quadratic Branch (High variance)
        const double inv_psi = 1.0 / psi;
        const double b2 =
            2.0 * inv_psi - 1.0 + std::sqrt(2.0 * inv_psi) * std::sqrt(2.0 * inv_psi - 1.0);
        const double b = std::sqrt(b2);
        const double a = m / (1.0 + b2);
        const double Z = norm(rng);

        v = a * (b + Z) * (b + Z);
    } else {
        // Exponential Branch (Low variance)
        const double p_exp = (psi - 1.0) / (psi + 1.0);
        const double beta = (1.0 - p_exp) / m;
        const double U = unif(rng);

        v = (U > p_exp) ? (1.0 / beta) * std::log((1.0 - p_exp) / (1.0 - U)) : 0.0;
    }

    // Log(X_t) update
    const double Z_x = norm(rng);
    const double variance_term = std::max(0.0, qe.K3 * v_prev + qe.K4 * v);
    X += p.r * dt + qe.K0 + qe.K1 * v_prev + qe.K2 * v + std::sqrt(variance_term) * Z_x;
}
} // namespace

PricingResult CPUHestonPricer::price(const HestonParameters &params, size_t num_paths,
                                     size_t num_steps, DiscretizationScheme scheme,
                                     unsigned int seed) {
    // Common precalculations
    const double dt = params.T / static_cast<double>(num_steps);
    const double discount_factor = std::exp(-params.r * params.T);

    std::mt19937 rng(seed);
    std::normal_distribution<double> norm_dist(0.0, 1.0);
    std::uniform_real_distribution<double> unif_dist(0.0, 1.0);

    double sum_payoff = 0.0;
    double sum_squared_payoff = 0.0;

    auto start = std::chrono::steady_clock::now();

    /* It is better to put an if outside the loop rather than inside so it only runs once
    even if the code looks messier */
    if (scheme == DiscretizationScheme::EulerMaruyama) {
        // Corresponding precalculations
        const double sqrt_dt = std::sqrt(dt);
        const double rho_comp = std::sqrt(1.0 - params.rho * params.rho);

        for (size_t path = 0; path < num_paths; ++path) {
            double X = std::log(params.S0);
            double v = params.v0;

            for (size_t step = 0; step < num_steps; ++step) {
                stepEuler(X, v, dt, sqrt_dt, params.rho, rho_comp, params.r, params.kappa,
                          params.theta, params.xi, rng, norm_dist);
            }

            const double payoff = std::max(std::exp(X) - params.K, 0.0);
            sum_payoff += payoff;
            sum_squared_payoff += payoff * payoff;
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

        for (size_t path = 0; path < num_paths; ++path) {
            double X = std::log(params.S0);
            double v = params.v0;

            for (size_t step = 0; step < num_steps; ++step) {
                stepQE(X, v, dt, params, qe, rng, norm_dist, unif_dist);
            }

            const double payoff = std::max(std::exp(X) - params.K, 0.0);
            sum_payoff += payoff;
            sum_squared_payoff += payoff * payoff;
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
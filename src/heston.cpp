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

PricingResult HestonSimulator::price_european_call(size_t num_paths, size_t num_steps, DiscretizationScheme disc_scheme){
    const double dt = params_.T / static_cast<double>(num_steps);
    
    double sum_payoff = 0;
    double sum_squared_payoff = 0;
    double sqrt_dt = std::sqrt(dt);
    
    auto start = std::chrono::steady_clock::now();
    
    // FIX ME!
    /* When Implementing DiscretizationScheme, put an if here
    where you calculate sqrt_dt if EM, and K_i for QE (USE THE STRUCT QE QEcoefs) */
    
    QECoefficients QEcoefs;
    if(disc_scheme == DiscretizationScheme::QuadraticExponential){
        const double gamma1 = 0.5, gamma2 = 0.5; 

        QEcoefs.K0 = - params_.kappa * params_.rho * params_.theta * dt / params_.sigma;
        QEcoefs.K1 = (params_.kappa * params_.rho / params_.sigma - 0.5) * gamma1 * dt - params_.rho / params_.sigma;
        QEcoefs.K2 = (params_.kappa * params_.rho / params_.sigma - 0.5) * gamma2 * dt + params_.rho / params_.sigma;
        QEcoefs.K3 = (1 - params_.rho * params_.rho) * gamma1 * dt;
        QEcoefs.K4 = (1 - params_.rho * params_.rho) * gamma2 * dt;

    }

    for(size_t path = 0; path < num_paths; path++){
        double X = std::log(params_.S0); // Taking log is more precise
        double v = params_.v0;

        for(size_t step = 0; step < num_steps; step++){
            if(disc_scheme == DiscretizationScheme::EulerMaruyama) eulerStep_(X, v, dt, sqrt_dt);
            else if (disc_scheme == DiscretizationScheme::QuadraticExponential) qeStep_(X, v, QEcoefs, dt);
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
void HestonSimulator::eulerStep_(double &X, double &v, double dt, double sqrt_dt){
    CorrelatedNormals normals = generateCorrelatedNormal(params_.rho);
    const double v_truncated = std::max(v, 0.0);
    const double sqrt_v = std::sqrt(v_truncated);
    
    X += (params_.r - 0.5 * v_truncated) * dt + sqrt_v * sqrt_dt * normals.z1;
    v += params_.kappa * (params_.theta - v_truncated) * dt + sqrt_v * params_.sigma *sqrt_dt * normals.z2;
}

// Quadratic Exponential with Martingale Approach
void HestonSimulator::qeStep_(double &X, double &v, QECoefficients QEcoefs, double dt){
    double v_prev = v;
    
    // THRESHOLD
    const double PSI_C = 1.5; 

    // DEFINE m and s squared
    double exp_kdt = std::exp(-params_.kappa * dt);
    double m = params_.theta + (v_prev - params_.theta) * exp_kdt;
    double sigma2 = params_.sigma * params_.sigma;
    double one_minus_exp = 1.0 - exp_kdt;

    double s2 =
        v_prev * sigma2 * exp_kdt * one_minus_exp / params_.kappa
        +
        params_.theta * sigma2 * one_minus_exp * one_minus_exp
            / (2.0 * params_.kappa);

    if (params_.kappa < 1e-8) {
        // K tends to zero
        m = v; 
        s2 = v * params_.sigma * params_.sigma * dt;
    } else {
        // Standard formulas
        double exp_k = std::exp(-params_.kappa * dt);
        m = params_.theta + (v - params_.theta) * exp_k;
        
        double term1 = (v * params_.sigma * params_.sigma * exp_k / params_.kappa) * (1.0 - exp_k);
        double term2 = (params_.theta * params_.sigma * params_.sigma / (2.0 * params_.kappa)) * std::pow(1.0 - exp_k, 2);
        s2 = term1 + term2;
    }

    
    // DEFINE PSI
    double psi = s2 / (m * m);
    
    if(psi < 1e-9){
        v = m;
    } else if(psi <= PSI_C){
        // QUADRATIC
        double b2 =
            2.0 / psi
            - 1.0
            + std::sqrt(2.0 / psi)
                * std::sqrt(2.0 / psi - 1.0);

        double b = std::sqrt(b2);
        double a = m / (1 + b * b);
        double Z = normal_(rng_); 

        v = a * (b + Z) * (b + Z);
    }else{
        // EXPONENTIAL
        double p = (psi - 1.0) / (psi + 1.0);
        double beta = (1.0 - p) / m;
        double U = uniform_(rng_);

        v = (U > p) ? (1.0 / beta) * std::log((1.0 - p) / (1.0 - U)) : 0;
    }

    // UPDATE X_t
    // Calculate K_i's
    // double K0 = - params_.kappa * params_.rho * params_.theta * dt / params_.sigma;
    // double K1 = (params_.kappa * params_.rho / params_.sigma - 0.5) * gamma1 * dt - params_.rho / params_.sigma;
    // double K2 = (params_.kappa * params_.rho / params_.sigma - 0.5) * gamma2 * dt + params_.rho / params_.sigma;
    // double K3 = (1 - params_.rho * params_.rho) * gamma1 * dt;
    // double K4 = (1 - params_.rho * params_.rho) * gamma2 * dt;

    // INDEPENDENT N(0, 1)
    double Z_x = normal_(rng_);

    double variance_term = std::max(0.0, QEcoefs.K3 * v_prev + QEcoefs.K4 * v);
    X += params_.r * dt + QEcoefs.K0 + QEcoefs.K1 * v_prev + QEcoefs.K2 * v + std::sqrt(variance_term) * Z_x;
}
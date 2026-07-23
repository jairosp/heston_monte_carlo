#ifndef HESTON_STEPS_HPP
#define HESTON_STEPS_HPP

#include "core/types.hpp"
#include <algorithm>
#include <cmath>
#include <random>

struct QEPrecomputed {
    double exp_kdt;
    double one_minus_exp;
    double K0, K1, K2, K3, K4;
    double A_const, B_const;
};

namespace heston::internal {

template <typename RNG>
inline void stepEuler(double &X, double &v, double dt, double sqrt_dt, double rho, double rho_comp,
                      double r, double kappa, double theta, double xi, RNG &rng,
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
template <typename RNG>
inline void stepQE(double &X, double &v, double dt, const HestonParameters &p,
                   const QEPrecomputed &qe, RNG &rng, std::normal_distribution<double> &norm,
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
} // namespace heston::internal

#endif // Heston Steps
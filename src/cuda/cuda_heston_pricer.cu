#include "cuda/cuda_heston_pricer.hpp"

#include <cuda_runtime.h>
#include <curand_kernel.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <vector>

#define CUDA_CHECK(call)                                                       \
    do {                                                                       \
        cudaError_t err = (call);                                              \
        if (err != cudaSuccess) {                                              \
            throw std::runtime_error(                                          \
                std::string("CUDA error: ") + cudaGetErrorString(err));        \
        }                                                                      \
    } while (0)

__global__ void initRNG(curandStatePhilox4_32_10_t* states,
                        unsigned long long seed,
                        size_t num_paths)
{
    const size_t path_id =
        static_cast<size_t>(blockIdx.x) * blockDim.x + threadIdx.x;

    if (path_id >= num_paths)
        return;

    curand_init(seed, static_cast<unsigned long long>(path_id), 0, &states[path_id]);
}

__global__ void simulate_paths(double* payoffs,
                               curandStatePhilox4_32_10_t* rng_states,
                               size_t num_paths,
                               size_t num_steps,
                               double S0,
                               double K,
                               double T,
                               double r,
                               double kappa,
                               double theta,
                               double xi,
                               double v0,
                               double rho)
{
    const size_t path_id =
        static_cast<size_t>(blockIdx.x) * blockDim.x + threadIdx.x;

    if (path_id >= num_paths)
        return;

    curandStatePhilox4_32_10_t rng = rng_states[path_id];

    const double dt = T / static_cast<double>(num_steps);
    const double sqrt_dt = sqrt(dt);
    const double rho_comp = sqrt(1.0 - rho * rho);

    double X = log(S0);
    double v = v0;

    for (size_t step = 0; step < num_steps; ++step) {
        const double z1 = curand_normal_double(&rng);
        const double z2_uncorr = curand_normal_double(&rng);
        const double z2 = rho * z1 + rho_comp * z2_uncorr;

        const double v_trunc = fmax(v, 0.0);
        const double sqrt_v = sqrt(v_trunc);

        X += (r - 0.5 * v_trunc) * dt + sqrt_v * sqrt_dt * z1;
        v += kappa * (theta - v_trunc) * dt + xi * sqrt_v * sqrt_dt * z2;
    }

    const double ST = exp(X);
    const double payoff = fmax(ST - K, 0.0);

    payoffs[path_id] = payoff;
    rng_states[path_id] = rng;
}

__global__ void simulate_paths_qe(double* payoffs,
                                  curandStatePhilox4_32_10_t* rng_states,
                                  size_t num_paths,
                                  size_t num_steps,
                                  double S0,
                                  double K,
                                  double v0,
                                  double exp_kdt,
                                  double A_const,
                                  double B_const,
                                  double theta,
                                  double K0_,
                                  double K1_,
                                  double K2_,
                                  double K3_,
                                  double K4_)
{
    const size_t path_id =
        static_cast<size_t>(blockIdx.x) * blockDim.x + threadIdx.x;

    if (path_id >= num_paths)
        return;

    curandStatePhilox4_32_10_t rng = rng_states[path_id];

    constexpr double psi_c = 1.5;

    double X = log(S0);
    double v = v0;

    for (size_t step = 0; step < num_steps; ++step) {
        const double m  = theta + (v - theta) * exp_kdt;
        const double s2 = v * A_const + B_const;
        const double psi = s2 / (m * m);

        double v_next;

        if (psi <= psi_c) {
            const double inv_psi = 1.0 / psi;
            const double b2 = 2.0 * inv_psi - 1.0 +
                              sqrt(2.0 * inv_psi) * sqrt(2.0 * inv_psi - 1.0);
            const double a = m / (1.0 + b2);
            const double b = sqrt(b2);

            const double Zv = curand_normal_double(&rng);
            const double term = b + Zv;

            v_next = a * term * term;
        } else {
            const double p = (psi - 1.0) / (psi + 1.0);
            const double beta = (1.0 - p) / m;

            const double Uv = curand_uniform_double(&rng);

            v_next = (Uv <= p) ? 0.0
                                : (1.0 / beta) * log((1.0 - p) / (1.0 - Uv));
        }

        const double Zx = curand_normal_double(&rng);
        const double variance_term = fmax(K3_ * v + K4_ * v_next, 0.0);

        X += K0_ + K1_ * v + K2_ * v_next + sqrt(variance_term) * Zx;

        v = v_next;
    }

    const double ST = exp(X);
    const double payoff = fmax(ST - K, 0.0);

    payoffs[path_id] = payoff;
    rng_states[path_id] = rng;
}

PricingResult CUDAHestonPricer::price(const HestonParameters& params,
                                      size_t num_paths,
                                      size_t num_steps,
                                      DiscretizationScheme scheme,
                                      unsigned int seed)
{
    if (num_paths == 0)
        throw std::invalid_argument("num_paths must be > 0");

    if (num_steps == 0)
        throw std::invalid_argument("num_steps must be > 0");

    double* d_payoffs = nullptr;
    curandStatePhilox4_32_10_t* d_rng_states = nullptr;

    CUDA_CHECK(cudaMalloc(&d_payoffs, num_paths * sizeof(double)));
    CUDA_CHECK(
        cudaMalloc(&d_rng_states, num_paths * sizeof(curandStatePhilox4_32_10_t)));

    constexpr int BLOCK_SIZE = 256;
    const int GRID_SIZE =
        static_cast<int>((num_paths + BLOCK_SIZE - 1) / BLOCK_SIZE);

    cudaEvent_t start;
    cudaEvent_t stop;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));
    CUDA_CHECK(cudaEventRecord(start));

    initRNG<<<GRID_SIZE, BLOCK_SIZE>>>(
        d_rng_states, static_cast<unsigned long long>(seed), num_paths);
    CUDA_CHECK(cudaGetLastError());

    const double dt = params.T / static_cast<double>(num_steps);

    if (scheme == DiscretizationScheme::EulerMaruyama) {
        simulate_paths<<<GRID_SIZE, BLOCK_SIZE>>>(
            d_payoffs, d_rng_states, num_paths, num_steps,
            params.S0, params.K, params.T, params.r,
            params.kappa, params.theta, params.xi, params.v0, params.rho);
    } else if (scheme == DiscretizationScheme::QuadraticExponential) {
        const double exp_kdt = std::exp(-params.kappa * dt);
        const double one_minus_exp = 1.0 - exp_kdt;
        const double xi2 = params.xi * params.xi;

        const double A_const = (xi2 * exp_kdt * one_minus_exp) / params.kappa;
        const double B_const =
            (params.theta * xi2 * one_minus_exp * one_minus_exp) / (2.0 * params.kappa);

        constexpr double gamma1 = 0.5;
        constexpr double gamma2 = 0.5;

        const double K0 = -params.kappa * params.rho * params.theta * dt / params.xi;
        const double K1 =
            (params.kappa * params.rho / params.xi - 0.5) * gamma1 * dt - params.rho / params.xi;
        const double K2 =
            (params.kappa * params.rho / params.xi - 0.5) * gamma2 * dt + params.rho / params.xi;
        const double K3 = (1.0 - params.rho * params.rho) * gamma1 * dt;
        const double K4 = (1.0 - params.rho * params.rho) * gamma2 * dt;

        simulate_paths_qe<<<GRID_SIZE, BLOCK_SIZE>>>(
            d_payoffs, d_rng_states, num_paths, num_steps,
            params.S0, params.K, params.v0,
            exp_kdt, A_const, B_const, params.theta,
            K0, K1, K2, K3, K4);
    } else {
        CUDA_CHECK(cudaFree(d_payoffs));
        CUDA_CHECK(cudaFree(d_rng_states));
        throw std::invalid_argument("Unsupported discretization scheme.");
    }

    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaEventRecord(stop));
    CUDA_CHECK(cudaEventSynchronize(stop));

    float elapsed_ms = 0.0f;
    CUDA_CHECK(cudaEventElapsedTime(&elapsed_ms, start, stop));

    std::vector<double> payoffs(num_paths);

    CUDA_CHECK(cudaMemcpy(payoffs.data(),
                          d_payoffs,
                          num_paths * sizeof(double),
                          cudaMemcpyDeviceToHost));

    double sum_payoff = 0.0;
    double sum_squared_payoff = 0.0;

    for (size_t i = 0; i < num_paths; ++i) {
        const double payoff = payoffs[i];

        sum_payoff += payoff;
        sum_squared_payoff += payoff * payoff;
    }

    const double n = static_cast<double>(num_paths);
    const double discount_factor = std::exp(-params.r * params.T);
    const double mean_payoff = sum_payoff / n;

    PricingResult result{};

    result.price = mean_payoff * discount_factor;

    double price_variance =
        (sum_squared_payoff - (sum_payoff * sum_payoff) / n) / (n - 1.0);

    if (price_variance < 0.0)
        price_variance = 0.0;

    const double std_dev = std::sqrt(price_variance);

    result.std_error = discount_factor * (std_dev / std::sqrt(n));

    constexpr double z95 = 1.96;

    result.ci_lower = result.price - z95 * result.std_error;
    result.ci_upper = result.price + z95 * result.std_error;

    result.elapsed_seconds = static_cast<double>(elapsed_ms) / 1000.0;

    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));

    CUDA_CHECK(cudaFree(d_payoffs));
    CUDA_CHECK(cudaFree(d_rng_states));

    return result;
}
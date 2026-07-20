#include "cuda/cuda_heston_pricer.hpp"
#include <stdexcept>

PricingResult CUDAHestonPricer::price(const HestonParameters &params, size_t num_paths,
                                      size_t num_steps, DiscretizationScheme scheme,
                                      unsigned int seed) {
    throw std::runtime_error("CUDA Engine has not been implemented yet.");
}
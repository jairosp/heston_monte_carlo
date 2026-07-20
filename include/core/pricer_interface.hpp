#pragma once
#include "types.hpp"
#include <cstddef>

class IHestonPricer {
  public:
    virtual ~IHestonPricer() = default;

    virtual PricingResult price(const HestonParameters &params, size_t num_paths, size_t num_steps,
                                DiscretizationScheme scheme, unsigned int seed) = 0;
};
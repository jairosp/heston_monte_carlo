#pragma once
#include "core/pricer_interface.hpp"

class CPUHestonPricer : public IHestonPricer {
  public:
    PricingResult price(const HestonParameters &params, size_t num_paths, size_t num_steps,
                        DiscretizationScheme scheme, unsigned int seed) override;
};
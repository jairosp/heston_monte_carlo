#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/pricer_interface.hpp"
#include "core/types.hpp"
#include "cpu/cpu_heston_pricer.hpp"
#include "cpu/openmp_heston_pricer.hpp"

struct PricerConfig {
    std::string name;
    std::function<std::unique_ptr<IHestonPricer>()> create_pricer;
    DiscretizationScheme scheme;
};

inline const std::vector<PricerConfig> configs = {
    {"CPUEuler", [] { return std::make_unique<CPUHestonPricer>(); },
     DiscretizationScheme::EulerMaruyama},

    {"CPUQE", [] { return std::make_unique<CPUHestonPricer>(); },
     DiscretizationScheme::QuadraticExponential},

    {"OpenMPEuler", [] { return std::make_unique<OpenMPHestonPricer>(); },
     DiscretizationScheme::EulerMaruyama},

    {"OpenMPQE", [] { return std::make_unique<OpenMPHestonPricer>(); },
     DiscretizationScheme::QuadraticExponential}};
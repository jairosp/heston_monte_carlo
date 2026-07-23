#include "core/pricer_interface.hpp"
#include "core/types.hpp"
#include "cpu/cpu_heston_pricer.hpp"
#include "cpu/openmp_heston_pricer.hpp"
#include <iomanip>
#include <iostream>
#include <memory>
#include <string_view>

#ifdef BUILD_CUDA
#include "cuda/cuda_heston_pricer.hpp"
#endif

int main(int argc, char *argv[]) {
    EngineType engine_type = EngineType::CPU;
    DiscretizationScheme scheme = DiscretizationScheme::EulerMaruyama;

    // Get console flags
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--gpu")
            engine_type = EngineType::GPU;
        else if (arg == "--parallel")
            engine_type = EngineType::CPU_Parallel;
        if (arg == "--qe")
            scheme = DiscretizationScheme::QuadraticExponential;
    }

    std::unique_ptr<IHestonPricer> pricer;

    if (engine_type == EngineType::CPU) {
        std::cout << "[INFO] Running CPU engine...\n";
        pricer = std::make_unique<CPUHestonPricer>();
    } else if (engine_type == EngineType::CPU_Parallel) {
        std::cout << "[INFO] Running parallel CPU engine...\n";
        pricer = std::make_unique<OpenMPHestonPricer>();
    }

    else {
#ifdef BUILD_CUDA
        std::cout << "[INFO] Running GPU engine (CUDA)...\n";
        pricer = std::make_unique<CUDAHestonPricer>();
#else
        std::cerr << "[ERROR] CUDA is not supported.\n";
        return 1;
#endif
    }

    // TWEAK PARAMETERS HERE!
    HestonParameters params{.S0 = 100.0,
                            .K = 100.0,
                            .r = 0.03,
                            .T = 1.0,
                            .v0 = 0.04,
                            .kappa = 2.0,
                            .theta = 0.04,
                            .xi = 0.3,
                            .rho = -0.7};

    const size_t num_paths = 100000;
    const size_t num_steps = 365;
    const unsigned int seed = 1234;

    try {
        PricingResult res = pricer->price(params, num_paths, num_steps, scheme, seed);

        std::cout << std::fixed << std::setprecision(5);
        std::cout << "\n================ Pricing Result ================\n";
        std::cout << "Option Call Price:  " << res.price << "\n";
        std::cout << "Standard Error:     " << res.std_error << "\n";
        std::cout << "95% CI :            [ " << res.ci_lower << ", " << res.ci_upper << " ]\n";
        std::cout << "Elapsed time:       " << res.elapsed_seconds << "s\n";
        std::cout << "Throughput CPU:     " << static_cast<double>(num_paths) / res.elapsed_seconds
                  << "paths/sec\n";
        std::cout << "================================================\n";

    } catch (const std::exception &e) {
        std::cerr << "[EXCEPTION] " << e.what() << "\n";
        return 1;
    }

    return 0;
}
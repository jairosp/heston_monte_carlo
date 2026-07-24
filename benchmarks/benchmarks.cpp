#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>
// New architecture
#include "core/pricer_interface.hpp"
#include "core/types.hpp"
#include "cpu/cpu_heston_pricer.hpp"
#include "cpu/openmp_heston_pricer.hpp"

#ifdef BUILD_CUDA
#include "cuda/cuda_heston_pricer.hpp"
#endif

constexpr unsigned int RNG_SEED = 123;

struct BenchmarkResult {
    EngineType engine;
    DiscretizationScheme scheme;
    size_t steps;
    size_t paths;
    PricingResult metrics;
};

int main() {
    HestonParameters params = {.S0 = 100.0,
                               .K = 100.0,
                               .r = 0.05,
                               .T = 1.0,
                               .v0 = 0.04,
                               .kappa = 1.0,
                               .theta = 0.04,
                               .xi = 0.8,
                               .rho = -0.9};

    const size_t NUM_STEPS = 12; // One step = One month (Takes advantage of QE)
    std::vector<size_t> num_paths = {1'000, 10'000, 100'000, 1'000'000, 10'000'000};
    std::vector<BenchmarkResult> results;

    std::unique_ptr<IHestonPricer> cpu_pricer = std::make_unique<CPUHestonPricer>();
    std::unique_ptr<IHestonPricer> cpu_parallel_pricer = std::make_unique<OpenMPHestonPricer>();

    /* POTENTIAL FIX: CREATE A SIMILAR FOR LOOP LIKE THE ONE IN
     TESTING AND JUST ITERATE INSTEAD OF 4 FOR LOOPS*/

    // Euler-Maruyama Samples
    for (size_t paths : num_paths) {
        PricingResult metrics = cpu_pricer->price(params, paths, NUM_STEPS,
                                                  DiscretizationScheme::EulerMaruyama, RNG_SEED);

        results.push_back(
            {EngineType::CPU, DiscretizationScheme::EulerMaruyama, NUM_STEPS, paths, metrics});
    }

    // Euler-Maruyama Samples Parallel
    for (size_t paths : num_paths) {
        PricingResult metrics = cpu_parallel_pricer->price(
            params, paths, NUM_STEPS, DiscretizationScheme::EulerMaruyama, RNG_SEED);

        results.push_back({EngineType::CPU_Parallel, DiscretizationScheme::EulerMaruyama, NUM_STEPS,
                           paths, metrics});
    }

    // QE-Scheme Samples
    for (size_t paths : num_paths) {
        PricingResult metrics = cpu_pricer->price(
            params, paths, NUM_STEPS, DiscretizationScheme::QuadraticExponential, RNG_SEED);

        results.push_back({EngineType::CPU, DiscretizationScheme::QuadraticExponential, NUM_STEPS,
                           paths, metrics});
    }

    // QE-Scheme Samples Parallel
    for (size_t paths : num_paths) {
        PricingResult metrics = cpu_parallel_pricer->price(
            params, paths, NUM_STEPS, DiscretizationScheme::QuadraticExponential, RNG_SEED);

        results.push_back({EngineType::CPU_Parallel, DiscretizationScheme::QuadraticExponential,
                           NUM_STEPS, paths, metrics});
    }

    // GPU Benchmarking (To be built)
#ifdef BUILD_CUDA
    std::unique_ptr<IHestonPricer> gpu_pricer = std::make_unique<CUDAHestonPricer>();

    // Euler Maruyama
    for (size_t paths : num_paths) {
        try {
            PricingResult metrics = gpu_pricer->price(
                params, paths, NUM_STEPS, DiscretizationScheme::EulerMaruyama, RNG_SEED);

            results.push_back(
                {EngineType::GPU, DiscretizationScheme::EulerMaruyama, NUM_STEPS, paths, metrics});
        } catch (const std::exception &e) {
            std::cerr << "[GPU Benchmark inactive]: " << e.what() << "\n";
            break;
        }
    }

    // Quadratic Exponential
    for (size_t paths : num_paths) {
        try {
            PricingResult metrics = gpu_pricer->price(
                params, paths, NUM_STEPS, DiscretizationScheme::QuadraticExponential, RNG_SEED);

            results.push_back({EngineType::GPU, DiscretizationScheme::QuadraticExponential,
                               NUM_STEPS, paths, metrics});
        } catch (const std::exception &e) {
            std::cerr << "[GPU Benchmark inactive]: " << e.what() << "\n";
            break;
        }
    }
#endif

    // CSV Results
    std::filesystem::path root_path = PROJECT_ROOT_DIR;
    std::filesystem::path output_dir = root_path / "benchmarks";
    std::filesystem::path csv_path = output_dir / "benchmarks.csv";

    std::filesystem::create_directories(output_dir);
    std::ofstream csv_file(csv_path);

    if (!csv_file.is_open()) {
        std::cerr << "Error: could not open " << csv_path << " for writing\n";
        return 1;
    }

    // CSV Header
    csv_file << "Engine,Scheme,Paths,Timesteps,Price,StdError,TimeSeconds\n";

    for (const auto &res : results) {
        std::string engine_str;
        if (res.engine == EngineType::CPU)
            engine_str = "CPU";
        else if (res.engine == EngineType::CPU_Parallel)
            engine_str = "CPU_Parallel";
        else
            engine_str = "GPU";
        std::string scheme_str =
            (res.scheme == DiscretizationScheme::EulerMaruyama) ? "EulerMaruyama" : "QE";

        csv_file << engine_str << ',' << scheme_str << ',' << res.paths << ',' << res.steps << ','
                 << std::fixed << std::setprecision(6) << res.metrics.price << ','
                 << res.metrics.std_error << ',' << res.metrics.elapsed_seconds << '\n';
    }

    csv_file.close();
    std::cout << "[C++] Benchmarks were completed and saved to " << csv_path << "\n";

    return 0;
}

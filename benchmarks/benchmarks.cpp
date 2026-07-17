#include "heston.hpp"
#include <filesystem> // Handle files and paths
#include <format>
#include <fstream> // Write on CSV
#include <iostream>

constexpr unsigned int RNG_SEED = 123;

struct BenchmarkResult {
    DiscretizationScheme scheme;
    int steps;
    int paths;
    PricingResult metrics;
};

int main() {
    HestonParameters params = {.S0 = 100.0,
                               .K = 100.0,
                               .r = 0.05,
                               .T = 1.0,
                               .v0 = 0.04,
                               .theta = 0.04,
                               .kappa = 2.0,
                               .sigma = 0.1,
                               .rho = -0.5};

    std::vector<int> num_paths = {1'000, 10'000, 100'000, 1'000'000};
    const int NUM_STEPS = 365;
    std::vector<BenchmarkResult> results;

    HestonSimulator pricingEngine(params, RNG_SEED);

    // EULER
    for (int paths : num_paths) {
        PricingResult metrics = pricingEngine.price_european_call(
            paths, NUM_STEPS, DiscretizationScheme::EulerMaruyama);

        BenchmarkResult result = {DiscretizationScheme::EulerMaruyama, NUM_STEPS, paths, metrics};
        results.push_back(result);
    }

    // QE
    for (int paths : num_paths) {
        PricingResult metrics = pricingEngine.price_european_call(
            paths, NUM_STEPS, DiscretizationScheme::QuadraticExponential);

        BenchmarkResult result = {DiscretizationScheme::QuadraticExponential, NUM_STEPS, paths,
                                  metrics};
        results.push_back(result);
    }

    // OPEN THE FILE

    std::filesystem::path root_path = PROJECT_ROOT_DIR;
    std::filesystem::path output_dir = root_path / "benchmarks";
    std::filesystem::path csv_path = output_dir / "benchmarks.csv";
    std::filesystem::create_directories(output_dir);
    std::ofstream csv_file(csv_path);

    if (!csv_file.is_open()) {
        std::cerr << "Could not create or open the file benchmarking_results.csv\n";
        return 1;
    }

    csv_file << "Scheme,Paths,Timesteps,Price,StdError,TimeSeconds\n";

    for (const auto &res : results) {

        // Format does not know how to format user-defined types
        std::string scheme_str =
            res.scheme == (DiscretizationScheme::EulerMaruyama) ? "EulerMaruyama" : "QE";

        csv_file << std::format("{},{},{},{:.6f},{:.6f},{:.6f}\n", scheme_str, res.paths, res.steps,
                                res.metrics.price, res.metrics.std_error,
                                res.metrics.elapsed_seconds);
    }

    csv_file.close();

    std::cout << "[C++] Benchmarks completed and written to benchmarking_results.csv\n";

    return 0;
}
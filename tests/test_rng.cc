#include <cmath>
#include <gtest/gtest.h>
#include <memory>

// Cabeceras modulares de la nueva arquitectura
#include "core/pricer_interface.hpp"
#include "core/types.hpp"
#include "cpu/cpu_heston_pricer.hpp"

class ReproducibilityTest : public ::testing::Test {
  protected:
    void SetUp() override { pricer = std::make_unique<CPUHestonPricer>(); }

    static constexpr unsigned int seed = 42;

    HestonParameters params{.S0 = 100.0,
                            .K = 100.0,
                            .r = 0.05,
                            .T = 1.0,
                            .v0 = 0.04,
                            .kappa = 2.0,
                            .theta = 0.04,
                            .xi = 0.1,
                            .rho = -0.5};

    static constexpr size_t num_paths = 100'000;
    static constexpr size_t num_steps = 365;

    std::unique_ptr<IHestonPricer> pricer;
};

TEST_F(ReproducibilityTest, SameSeedSameResult) {
    PricingResult results1 =
        pricer->price(params, num_paths, num_steps, DiscretizationScheme::EulerMaruyama, seed);

    PricingResult results2 =
        pricer->price(params, num_paths, num_steps, DiscretizationScheme::EulerMaruyama, seed);

    EXPECT_DOUBLE_EQ(results1.price, results2.price);
    EXPECT_DOUBLE_EQ(results1.std_error, results2.std_error);
}

TEST_F(ReproducibilityTest, DiffSeedDiffResult) {
    constexpr unsigned int seed1 = 42;
    constexpr unsigned int seed2 = 123;

    PricingResult results1 =
        pricer->price(params, num_paths, num_steps, DiscretizationScheme::EulerMaruyama, seed1);

    PricingResult results2 =
        pricer->price(params, num_paths, num_steps, DiscretizationScheme::EulerMaruyama, seed2);

    EXPECT_GT(std::abs(results1.price - results2.price), 1e-6);
}

TEST_F(ReproducibilityTest, StandardErrorDecreasesWithMorePaths) {
    constexpr size_t small_paths = 10'000;
    constexpr size_t large_paths = 100'000;

    auto result_small =
        pricer->price(params, small_paths, num_steps, DiscretizationScheme::EulerMaruyama, seed);

    auto result_large =
        pricer->price(params, large_paths, num_steps, DiscretizationScheme::EulerMaruyama, seed);

    EXPECT_LT(result_large.std_error, result_small.std_error);
}
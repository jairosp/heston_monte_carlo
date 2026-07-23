#include <cmath>
#include <gtest/gtest.h>
#include <memory>

#include "core/pricer_interface.hpp"
#include "core/types.hpp"
#include "tests/pricer_configs.hpp"

class ReproducibilityTest : public ::testing::TestWithParam<PricerConfig> {
  protected:
    static constexpr unsigned int seed = 42;

    static constexpr size_t num_paths = 100'000;
    static constexpr size_t num_steps = 365;

    HestonParameters params{.S0 = 100.0,
                            .K = 100.0,
                            .r = 0.05,
                            .T = 1.0,
                            .v0 = 0.04,
                            .kappa = 2.0,
                            .theta = 0.04,
                            .xi = 0.1,
                            .rho = -0.5};

    PricingResult RunPrice(const HestonParameters &p, size_t paths, size_t steps,
                           unsigned int rng_seed) {
        const auto &cfg = GetParam();

        auto pricer = cfg.create_pricer();

        return pricer->price(p, paths, steps, cfg.scheme, rng_seed);
    }
};

TEST_P(ReproducibilityTest, SameSeedSameResult) {
    auto result1 = RunPrice(params, num_paths, num_steps, seed);

    auto result2 = RunPrice(params, num_paths, num_steps, seed);

    EXPECT_DOUBLE_EQ(result1.price, result2.price);
    EXPECT_DOUBLE_EQ(result1.std_error, result2.std_error);
}

TEST_P(ReproducibilityTest, DifferentSeedsDifferentResult) {
    constexpr unsigned int seed1 = 42;
    constexpr unsigned int seed2 = 123;

    auto result1 = RunPrice(params, num_paths, num_steps, seed1);

    auto result2 = RunPrice(params, num_paths, num_steps, seed2);

    EXPECT_GT(std::abs(result1.price - result2.price), 1e-6);
}

TEST_P(ReproducibilityTest, StandardErrorDecreasesWithMorePaths) {
    constexpr size_t small_paths = 10'000;
    constexpr size_t large_paths = 100'000;

    auto result_small = RunPrice(params, small_paths, num_steps, seed);

    auto result_large = RunPrice(params, large_paths, num_steps, seed);

    EXPECT_LT(result_large.std_error, result_small.std_error);
}

INSTANTIATE_TEST_SUITE_P(AllPricers, ReproducibilityTest, ::testing::ValuesIn(configs));
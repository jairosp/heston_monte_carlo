#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <memory>

#include "core/pricer_interface.hpp"
#include "core/types.hpp"
#include "cpu/cpu_heston_pricer.hpp"

class FinancialPropertiesTest : public ::testing::Test {
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

TEST_F(FinancialPropertiesTest, NonNegativePrice) {
    auto result =
        pricer->price(params, num_paths, num_steps, DiscretizationScheme::EulerMaruyama, seed);

    EXPECT_GE(result.price, 0.0);
}

TEST_F(FinancialPropertiesTest, PriceLessThanSpot) {
    auto result =
        pricer->price(params, num_paths, num_steps, DiscretizationScheme::EulerMaruyama, seed);

    EXPECT_LE(result.price, params.S0);
}

TEST_F(FinancialPropertiesTest, HigherSpotIncreasesPrice) {
    auto low_spot = params;
    auto high_spot = params;

    low_spot.S0 = 100.0;
    high_spot.S0 = 110.0;

    auto result_low =
        pricer->price(low_spot, num_paths, num_steps, DiscretizationScheme::EulerMaruyama, seed);

    auto result_high =
        pricer->price(high_spot, num_paths, num_steps, DiscretizationScheme::EulerMaruyama, seed);

    EXPECT_LT(result_low.price, result_high.price);
}

TEST_F(FinancialPropertiesTest, HigherStrikeDecreasesPrice) {
    auto low_strike = params;
    auto high_strike = params;

    low_strike.K = 100.0;
    high_strike.K = 110.0;

    auto result_low =
        pricer->price(low_strike, num_paths, num_steps, DiscretizationScheme::EulerMaruyama, seed);

    auto result_high =
        pricer->price(high_strike, num_paths, num_steps, DiscretizationScheme::EulerMaruyama, seed);

    EXPECT_GT(result_low.price, result_high.price);
}

TEST_F(FinancialPropertiesTest, ConfidenceIntervalContainsPrice) {
    auto result =
        pricer->price(params, num_paths, num_steps, DiscretizationScheme::EulerMaruyama, seed);

    EXPECT_LT(result.ci_lower, result.price);
    EXPECT_GT(result.ci_upper, result.price);
}

TEST_F(FinancialPropertiesTest, PriceAboveNoArbitrageLowerBound) {
    auto result =
        pricer->price(params, num_paths, num_steps, DiscretizationScheme::EulerMaruyama, seed);

    // C >= max(S0 - K * exp(-r * T), 0)
    const double lower_bound = std::max(params.S0 - params.K * std::exp(-params.r * params.T), 0.0);

    EXPECT_GE(result.price, lower_bound);
}
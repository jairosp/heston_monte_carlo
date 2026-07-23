#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <memory>

#include "core/pricer_interface.hpp"
#include "core/types.hpp"
#include "tests/pricer_configs.hpp"

class FinancialPropertiesTest : public ::testing::TestWithParam<PricerConfig> {
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

    PricingResult RunPrice(const HestonParameters &p) {
        const auto &cfg = GetParam();

        auto pricer = cfg.create_pricer();

        return pricer->price(p, num_paths, num_steps, cfg.scheme, seed);
    }
};

TEST_P(FinancialPropertiesTest, NonNegativePrice) {
    auto result = RunPrice(params);

    EXPECT_GE(result.price, 0.0);
}

TEST_P(FinancialPropertiesTest, PriceLessThanSpot) {
    auto result = RunPrice(params);

    EXPECT_LE(result.price, params.S0);
}

TEST_P(FinancialPropertiesTest, HigherSpotIncreasesPrice) {
    auto low_spot = params;
    auto high_spot = params;

    low_spot.S0 = 100.0;
    high_spot.S0 = 110.0;

    auto result_low = RunPrice(low_spot);
    auto result_high = RunPrice(high_spot);

    EXPECT_LT(result_low.price, result_high.price);
}

TEST_P(FinancialPropertiesTest, HigherStrikeDecreasesPrice) {
    auto low_strike = params;
    auto high_strike = params;

    low_strike.K = 100.0;
    high_strike.K = 110.0;

    auto result_low = RunPrice(low_strike);
    auto result_high = RunPrice(high_strike);

    EXPECT_GT(result_low.price, result_high.price);
}

TEST_P(FinancialPropertiesTest, ConfidenceIntervalContainsPrice) {
    auto result = RunPrice(params);

    EXPECT_LT(result.ci_lower, result.price);
    EXPECT_GT(result.ci_upper, result.price);
}

TEST_P(FinancialPropertiesTest, PriceAboveNoArbitrageLowerBound) {
    auto result = RunPrice(params);

    const double lower_bound = std::max(params.S0 - params.K * std::exp(-params.r * params.T), 0.0);

    EXPECT_GE(result.price, lower_bound);
}

INSTANTIATE_TEST_SUITE_P(AllPricers, FinancialPropertiesTest, ::testing::ValuesIn(configs));
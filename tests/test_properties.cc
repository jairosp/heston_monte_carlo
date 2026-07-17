#include "heston.hpp"
#include <gtest/gtest.h>

// NOTE: FIXTURES CREATE SHARED CONTEXT BETWEEN TESTS
class FinancialPropertiesTest : public ::testing::Test {
  protected:
    static constexpr unsigned int seed = 42;

    HestonParameters params{.S0 = 100.0,
                            .K = 100.0,
                            .r = 0.05,
                            .T = 1.0,
                            .v0 = 0.04,
                            .theta = 0.04,
                            .kappa = 2.0,
                            .sigma = 0.1,
                            .rho = -0.5};

    static constexpr size_t num_paths = 100000;
    static constexpr size_t num_steps = 365;
};

TEST_F(FinancialPropertiesTest, NonNegativePrice) {
    HestonSimulator engine(params, seed);

    auto result = engine.price_european_call(num_paths, num_steps,
                                             DiscretizationScheme::QuadraticExponential);

    EXPECT_GE(result.price, 0.0);
}

TEST_F(FinancialPropertiesTest, PriceLessThanSpot) {
    HestonSimulator engine(params, seed);

    auto result = engine.price_european_call(num_paths, num_steps,
                                             DiscretizationScheme::QuadraticExponential);

    EXPECT_LE(result.price, params.S0);
}

TEST_F(FinancialPropertiesTest, HigherSpotIncreasesPrice) {
    auto low_spot = params;
    auto high_spot = params;

    low_spot.S0 = 100.0;
    high_spot.S0 = 110.0;

    HestonSimulator sim_low(low_spot, seed);
    HestonSimulator sim_high(high_spot, seed);

    auto result_low = sim_low.price_european_call(num_paths, num_steps,
                                                  DiscretizationScheme::QuadraticExponential);

    auto result_high = sim_high.price_european_call(num_paths, num_steps,
                                                    DiscretizationScheme::QuadraticExponential);

    EXPECT_LT(result_low.price, result_high.price);
}

TEST_F(FinancialPropertiesTest, HigherStrikeDecreasesPrice) {
    auto low_strike = params;
    auto high_strike = params;

    low_strike.K = 100.0;
    high_strike.K = 110.0;

    HestonSimulator sim_low(low_strike, seed);
    HestonSimulator sim_high(high_strike, seed);

    auto result_low = sim_low.price_european_call(num_paths, num_steps,
                                                  DiscretizationScheme::QuadraticExponential);

    auto result_high = sim_high.price_european_call(num_paths, num_steps,
                                                    DiscretizationScheme::QuadraticExponential);

    EXPECT_GT(result_low.price, result_high.price);
}

TEST_F(FinancialPropertiesTest, ConfidenceIntervalContainsPrice) {
    HestonSimulator engine(params, seed);

    auto result = engine.price_european_call(num_paths, num_steps,
                                             DiscretizationScheme::QuadraticExponential);

    EXPECT_LT(result.ci_lower, result.price);

    EXPECT_GT(result.ci_upper, result.price);
}

TEST_F(FinancialPropertiesTest, PriceAboveNoArbitrageLowerBound) {
    HestonSimulator engine(params, seed);

    auto result = engine.price_european_call(num_paths, num_steps,
                                             DiscretizationScheme::QuadraticExponential);

    const double lower_bound = std::max(params.S0 - params.K * std::exp(-params.r * params.T), 0.0);

    EXPECT_GE(result.price, lower_bound);
}
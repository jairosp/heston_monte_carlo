// Test reproductibility
#include <gtest/gtest.h>
#include "heston.hpp"

class ReproducibilityTest : public ::testing::Test
{
protected:
    static constexpr unsigned int seed = 42;

    HestonParameters params{
        .S0 = 100.0,
        .K = 100.0,
        .r = 0.05,
        .T = 1.0,
        .v0 = 0.04,
        .theta = 0.04,
        .kappa = 2.0,
        .sigma = 0.1,
        .rho = -0.5
    };

    static constexpr size_t num_paths = 100000;
    static constexpr size_t num_steps = 1000;
};

TEST_F(ReproducibilityTest, SameSeedSameResult){
    HestonSimulator engine1(params, seed);
    HestonSimulator engine2(params, seed);

    PricingResult results1 = engine1.price_european_call(num_paths, num_steps); 
    PricingResult results2 = engine2.price_european_call(num_paths, num_steps); 

    EXPECT_DOUBLE_EQ(results1.price, results2.price);
}

TEST_F(ReproducibilityTest, DiffSeedDiffResult){
    const unsigned int seed1 = 42;
    const unsigned int seed2 = 123;

    HestonSimulator engine1(params, seed1);
    HestonSimulator engine2(params, seed2);

    PricingResult results1 = engine1.price_european_call(num_paths, num_steps); 
    PricingResult results2 = engine2.price_european_call(num_paths, num_steps); 

    EXPECT_GT(abs(results1.price - results2.price), 1e-8);
}

TEST_F(ReproducibilityTest, StandardErrorDecreasesWithMorePaths){
    HestonSimulator engine1(params, seed);
    HestonSimulator engine2(params, seed);

    auto result_small = engine1.price_european_call(10000, num_steps);
    auto result_large = engine2.price_european_call(100000, num_steps);

    EXPECT_LT(result_large.std_error, result_small.std_error);
}
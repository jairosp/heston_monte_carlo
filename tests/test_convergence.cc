#include <gtest/gtest.h>
#include "heston.hpp"

// Process CSV 
#include <fstream>
#include <sstream>
#include <vector>

// PRINT TO FUNCTION
#include <ostream>

struct TestCase{
    double name;
    HestonParameters params;
    double expected_price;
};

// EXTRACT DATA FROM CSV
std::vector<TestCase> load_test_cases(const std::string& filename){
    std::vector<TestCase> cases;

    std::ifstream file(filename);

    if(!file)
        throw std::runtime_error("Cannot open file: " + filename);
    
    std::string line;
    
    // SKIP HEADERS
    std::getline(file, line);

    while(std::getline(file, line)){
        std::stringstream ss(line);
        std::string token;
        TestCase tc;

        std::getline(ss, token, ',');
        tc.params.S0 = std::stod(token);

        std::getline(ss, token, ',');
        tc.params.K = std::stod(token);

        std::getline(ss, token, ',');
        tc.params.r = std::stod(token);

        std::getline(ss, token, ',');
        tc.params.T = std::stod(token);

        std::getline(ss, token, ',');
        tc.params.v0 = std::stod(token);

        std::getline(ss, token, ',');
        tc.params.theta = std::stod(token);

        std::getline(ss, token, ',');
        tc.params.kappa = std::stod(token);

        std::getline(ss, token, ',');
        tc.params.xi = std::stod(token);

        std::getline(ss, token, ',');
        tc.params.rho = std::stod(token);

        std::getline(ss, token, ',');
        tc.expected_price = std::stod(token);

        cases.push_back(tc);
    }

    return cases;
}

// LOAD DATA
std::vector<TestCase> all_cases = load_test_cases("../tests/tests_data.csv");

class HestonTest :
    public ::testing::TestWithParam<TestCase>{};


TEST_P(HestonTest, Convergence)
{
    constexpr unsigned int RNG_SEED = 42;
    const TestCase& tc = GetParam();

    HestonSimulator engine(tc.params, RNG_SEED);

    auto result =
        engine.price_european_call(
            10000,
            100
        );

    EXPECT_NEAR(
        result.price,
        tc.expected_price,
        3.0 * result.std_error
    );
}

INSTANTIATE_TEST_SUITE_P(
    CSVCases,
    HestonTest,
    ::testing::ValuesIn(all_cases),
    [](const ::testing::TestParamInfo<TestCase>& info)
    {
        return std::string("Case") + std::to_string(info.index);
    }
);

void PrintTo(const TestCase& tc, std::ostream* os)
{
    *os << "S0=" << tc.params.S0
        << ", K=" << tc.params.K
        << ", expected=" << tc.expected_price;
}
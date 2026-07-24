#include <fstream>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// New architecture headers
#include "core/pricer_interface.hpp"
#include "core/types.hpp"
#include "cpu/cpu_heston_pricer.hpp"
#include "cpu/openmp_heston_pricer.hpp"
#include "tests/pricer_configs.hpp"

constexpr size_t NUM_PATHS = 100'000;
constexpr size_t NUM_STEPS = 365;
constexpr unsigned int RNG_SEED = 42;

struct TestCase {
    HestonParameters params;
    double expected_price;
};

// Read CSV and so on
inline std::vector<TestCase> load_test_cases(const std::string &filename) {
    std::vector<TestCase> cases;
    std::ifstream file(filename);

    if (!file) {
        file.open("../" + filename);
        if (!file) {
            throw std::runtime_error("Could not open the file: " + filename);
        }
    }

    std::string line;
    // Skip headers
    std::getline(file, line);

    while (std::getline(file, line)) {
        if (line.empty())
            continue;

        std::stringstream ss(line);
        std::string token;
        TestCase tc{};

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

// Lazy load
static const auto all_cases = load_test_cases("tests/tests_data.csv");

using ParamType = std::tuple<TestCase, PricerConfig>;

class HestonTest : public ::testing::TestWithParam<ParamType> {};

TEST_P(HestonTest, Convergence) {
    const auto &[tc, config] = GetParam();

    auto pricer = config.create_pricer();

    auto result = pricer->price(tc.params, NUM_PATHS, NUM_STEPS, config.scheme, RNG_SEED);

    EXPECT_NEAR(result.price, tc.expected_price, 3.0 * result.std_error);
}

INSTANTIATE_TEST_SUITE_P(CSVCases, HestonTest,
                         ::testing::Combine(::testing::ValuesIn(all_cases),
                                            ::testing::ValuesIn(configs)));

// Output formatting
void PrintTo(const TestCase &tc, std::ostream *os) {
    *os << "{ S0: " << tc.params.S0 << ", K: " << tc.params.K << ", T: " << tc.params.T
        << ", expected: " << tc.expected_price << " }";
}
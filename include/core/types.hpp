#pragma once

enum class EngineType { CPU, GPU };
enum class DiscretizationScheme { EulerMaruyama, QuadraticExponential };

struct HestonParameters {
    double S0;
    double K;
    double r;
    double T;
    double v0;
    double kappa;
    double theta;
    double xi;
    double rho;
};

struct PricingResult {
    double price;
    double std_error;
    double ci_lower;
    double ci_upper;
    double elapsed_seconds;
};
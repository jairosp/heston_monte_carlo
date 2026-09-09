# Heston Monte Carlo Option Pricer (CPU / CPU-Parallel / GPU)

A Monte Carlo pricer for European options under the Heston stochastic
volatility model, implemented with three execution backends (single-threaded
CPU, multithreaded CPU, CUDA GPU) and two discretization schemes
(Euler-Maruyama and the Quadratic-Exponential scheme of Andersen, 2008).

## Why this project

Heston is the standard benchmark for stochastic volatility pricing because
the variance process (CIR) can go negative under naive discretization,
which makes it a good stress test for both **numerical schemes** and
**parallel implementations**. This project compares:

- **Discretization bias**: Euler-Maruyama (simple, biased at coarse steps)
  vs. Quadratic-Exponential (near-unbiased even with large `dt`).
- **Compute backends**: naive CPU loop vs. multithreaded CPU vs. CUDA GPU,
  measuring real speedup as a function of path count.

## Results at a glance

Fixed number of paths (1e7)

| Engine       | Discretization Model | Time      |
|--------------|----------------------|-----------|
| CPU          | EM                   | 17.493192 |
| Parallel CPU | EM                   | 0.688317  |
| GPU          | EM                   | 0.050397  |
| CPU          | QE                   | 18.225789 |
| Parallel CPU | QE                   | 0.697019  |
| GPU          | QE                   | 0.130071  |

Average speedup

| Discretization Model |Parallel CPU / CPU | GPU / CPU | GPU / Parallel CPU |
|----------------------|------------------:|----------:|-------------------:|
| EM                   | 25.42×            | 347.11×   | 13.66×             |
| QE                   | 26.15×            | 140.12×   | 5.36×              |
| Average              | 25.79×            | 243.62×   | 9.51×              |


![Speedup vs path count](/benchmarks/reports/time_vs_paths_em.png)
![Speedup vs path count](/benchmarks/reports/time_vs_paths_qe.png)
![EM vs QE convergence](/benchmarks/reports/price_vs_paths.png)

## Model
The Heston model is a stochastic volatility model in which both the asset price ($S_t$) and its variance ($v_t$) evolve randomly over time.

$$
dS_t = rS_t,dt + \sqrt{v_t}S_t,dW_t^S
$$

$$
dv_t = \kappa(\theta - v_t),dt + \xi\sqrt{v_t},dW_t^v
$$

with correlation 

$$
(dW_t^S dW_t^v = \rho,dt).
$$

Since no closed-form solution exists for the simulated paths, option prices are estimated using Monte Carlo simulation. The first discretization scheme implemented is Euler–Maruyama (EM), a simple and widely used numerical method for stochastic differential equations. We then implement the Quadratic Exponential (QE) scheme, which is specifically designed for the Heston variance process and generally provides greater stability and accuracy while preserving the positivity of variance.

Parameters: `S0, K, T, r, kappa, theta, xi, v0, rho`.

## Architecture

* **.github/**: GitHub workflows for CI/CD.
* **benchmarks/**: Benchmarking scripts, reports, plots, and performance comparisons between different pricing engines and numerical schemes.
* **include/**: Header files and project interfaces.

  * **core/**: Core types, interfaces, and shared utilities.
  * **cpu/**: CPU-based pricing engines and supporting components.
  * **cuda/**: GPU/CUDA implementations and related utilities.
  * **tests/**: Shared utilities and helper structures used by the test suite.
* **scripts/**: Python scripts used for data analysis and plot generation.
* **src/**: Source code implementation. In addition to the corresponding `.cpp` files, it contains the project entry point (`main.cpp`).
* **tests/**: Unit and validation tests covering convergence, financial properties, reproducibility, and implementation-specific behavior.

All three backends implement the same interface:

```cpp
PricingResult price(const HestonParameters& params,
                     size_t num_paths,
                     size_t num_steps,
                     DiscretizationScheme scheme,
                     unsigned int seed);
```

so they are drop-in interchangeable in benchmarks and tests.

## Discretization schemes

- **Euler-Maruyama** — full-truncation scheme; variance floored at zero
  each step. Simple, fast per step, but biased when the Feller condition
  `2*kappa*theta >= xi^2` is violated (common in calibrated equity params),
  and the bias grows with coarser `dt`.
- **Quadratic-Exponential (QE)** — moment-matches the next variance to a
  squared-Gaussian (low `psi`) or an exponential-with-atom-at-zero (high
  `psi`) distribution instead of truncating. Near-unbiased even at large
  `dt`, at the cost of a branch and slightly more RNG draws per step.

## Validation

- Compared against a semi-closed-form Heston price via numerical
  integration of the characteristic function (Gatheral's "Little
  Heston Trap" formulation, chosen for numerical stability of the
  complex logarithm).
- Checked `xi -> 0` collapses to Black-Scholes.
- Checked EM converges to the QE/analytical price as `num_steps -> infinity`
  (see `figures/convergence.png`), confirming the gap between schemes at
  coarse step counts is the known EM truncation bias, not a defect.

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_CUDA=ON   # CUDA optional
cmake --build .
./heston_sim                                                   # CPU-only tests
```

Requires: CMake >= 3.18, a C++20 compiler, CUDA Toolkit >= 11.x for the
GPU backend (tested on compute capability >= 7.0).
Paramaters can be edited in src/main.cpp.

## Usage

```bash
./heston_sim --qe --gpu
```
Flags are optional, by default Euler-Maruyama scheme and CPU are chosen.

## Benchmark methodology

- Hardware:
  - CPU: AMD Ryzen Threadripper PRO 3955WX 16-Cores.
  - GPU: NVIDIA GeForce RTX 3090.

## Future Work

* Extend support to additional option types (e.g., puts, barrier options, Asian options).
* Add support for other stochastic volatility and local volatility models.
* Implement variance reduction techniques to improve Monte Carlo efficiency.
* Further optimize GPU kernels and memory usage.

## License

MIT (or your choice).

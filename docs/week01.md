# Week 1 (13 - 18 Jul)
## Objective
Fully functional CPU Heston Model engine Simulator
## Work Completed
* Fully working engine
* Different methods: QE vs EM
* Benchmarks with graphs
* Tests with Google Test
* Formatting
* Documented every step
* CMake integration with every tool
* Professional GitHub Repo.
## Validation
Checked results against Black Scholes, or analytical solutions to verify convergence via tests.
## Performance
QE performs much better when the Feller condition is not met and steps count is low. For more details check (benchmarks/reports)
## Key Challenges
First contact with some technologies, understanding structure, design decisions
## Lessons Learned
Read docs first, understand concepts fully before implementing, adopt professional settings early on
## Next Steps
Next week work will be focused on preparing the first CUDA Parallel working implementation, most likely on EM method (easier to parallelize) and we will compare GPU vs CPU performance.

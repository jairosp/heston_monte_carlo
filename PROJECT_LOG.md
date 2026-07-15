## 2026-07-13

### Session Goal
First steps, initialize the CPU scheme, understand the Heston Model and create a basic draft.

### Time Spent
4 Hours

### Completed
* Core project structure and classes
* Basic Heston Monte Carlo implementation
* Basic metrics: std_error, time_elapsed, confidence interval

### Problems encountered
* Configuring <chrono>
* Understanding new functions and keywords 

### Lessons learned
* Minimize operations by precomputing. (constexpr, aux variables...)
* Check cppreference for further information on language specs.
* Understanding project structure is key to coding faster later on.

### Next Session
* Output sample results via terminal
* Test with real data, verify results
* Review the math
* Nice to have: automate tests.

## 2026-07-14

### Session Goal
Setup tests, create terminal UI and compare with real data.

### Time Spent
4 Hours

### Completed
* GTest configuration and convergence tests
* Terminal UI for basic option pricing
* Reviewed mathematical foundations and euler-maruyama discretization

### Problems encountered
* Configuring GTest and adding CSV.  

### Lessons learned
* Read documentation for GTest and other tools.
* Organize folders from the get-go.
* Create a To-do list to track all pending tweaks.

### Next Session
* Add more general tests: edge cases, stress tests...
* Tweak CSV to label different tests
* Different types of truncation (ability to tweak via parameters)
* Ideally: Start configuring and learning about the QE-Scheme


## 2026-07-15

### Session Goal
Finish tests, Truncation, QE Scheme and Function Modularity

### Time Spent
3 Hours

### Completed
* Created modular functions for random number generation (correlated normals)
* Created modular functions for step function, which generalises the Method
* Started digging into the QE Scheme
* Added 10 tests for financial properties and reproducibility

### Problems encountered
* Test Fixtures, OOP Nuances in C++, Tweak structure in order to generalize  

### Lessons learned
* Modularity beats complexity
* Testing is not actually that difficult
* Log of price can be more precise than price (Fix Me!)
* QE Scheme mathematical basis

### Next Session
* Implement QE Scheme step
* Implement X = LOG(S) for increased precision (Ito's Lemma correction)
* Check Bessel condition and implications
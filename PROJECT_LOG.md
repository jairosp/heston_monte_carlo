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

## 2026-07-16

### Session Goal
Implement the QE Step and generalization

### Time Spent
5 Hours

### Completed
* QE Method Full Understanding, implementation and robustness (zero division errors)
* X_t = LOG(S_t) modification for efficiency
* Generalisation for methods

### Problems encountered
* QE Understanding 

### Lessons learned
* Check the math first

### Next Session
* Creating Benchmarks for comparing QE and Euler with Python

## 2026-07-17

### Session Goal
Implement benchmarking with python graphs

### Time Spent
4 Hours

### Completed
* Python professional benchmarking
* Code Formatting (LLVM style)

### Problems encountered
* Creating a Python Venv with CMake
* CMake in general (I need to review this)
* Relative and absolute paths for execution

### Lessons learned
* Understanding CMake deeply can save lots of time

### Next Session
* Deeply Understand GTest (Read Docs, watch some tutorials)
* General Tweaks, clean code, write weekly report
* Review the codebase and progress, internalize the structure


## 2026-07-20

### Session Goal
Code refactoring. New architecture which isolates CUDA Kernels.

### Time Spent
5 Hours

### Completed
* New Architechture. 
* Implemented Polymorfism and abstract classes.
* Isolated CUDA Kernels
* CMake Professiona configuration

### Problems encountered
* CMake, knowing it deeply could have saved lots of time
* Advanced OOP, if implemented directly, I would have saved time

### Lessons learned
* Find a slot to learn deeply the tools of the trade

### Next Session
* Start with CUDA (Today's meeting has been postponed for Wednesday)
* Check the book and notes, start writing something (Euler Maruyama Prototype at least)
* I personally do not know how much it will take to complete the CUDA Kernels

## 2026-07-21

### Session Goal
SSH Server configuration, make sure everything works, first kernel boilerplate. Vim, tmux and general config.

### Time Spent
3 Hours

### Problems encountered
Found it tedious to adapt to tmux + vim workflow. VSCode Remote feels slow or freezes somehow. I will take this opportunity to learn a new workflow.

### Lessons learned
Coping with servers, dependencies and remote machines

### Next Session
Meeting with my professor to discuss CUDA structure, implementations and draft the project outline.



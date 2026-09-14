# How to Run LOB-sim

This project uses CMake for building. Follow these steps to compile and run the tests.

## Prerequisites

- **CMake**: Build system generator.
- **C++ Compiler**: GCC or Clang with C++17 support.

## Build Instructions

1.  **Create a build directory and configure the project:**
    ```bash
    cmake -S . -B build
    ```

2.  **Compile the code:**
    ```bash
    cmake --build build
    ```

## Running Tests

After building, you can run the generated test executables located in the `build` directory:

- **Price Level Tests:**
  ```bash
  ./build/test_price_level
  ```

- **Order Node Tests:**
  ```bash
  ./build/test_order_node
  ```

- **Limit Order Book Tests:**
  ```bash
  ./build/test_lob
  ```

## Running the analysis / benchmark programs

These are the standalone programs that back up the project's performance and
microstructure claims (see `analysis/RESULTS.md` for the full writeup of what
each one found). They're built by CMake automatically (targets below), or you
can compile any of them directly with clang++/g++ if you don't have CMake
installed:

| Program | What it does | CMake target | Direct build |
|---|---|---|---|
| Mid-price / spread / queue-imbalance study | Drives 20,000 events of controlled random order flow through the real `LimitOrderBook` under three `buyProbability` scenarios and measures spread/volatility and imbalance→forward-price-impact correlation. Writes one CSV per scenario to `analysis/output/`. | `price_impact_study` | see below |
| O(1) vs O(log n) complexity benchmark | Times `placeLimitOrder`/`cancelOrder` on the real book at increasing depth, plus an isolated `unordered_map` vs `map` microbenchmark. | `complexity_benchmark` | see below |
| Heap vs tree order book benchmark | Times the tree-based `LimitOrderBook` (std::map price ladder) against an alternate `HeapLimitOrderBook` (two `std::priority_queue`s + lazy deletion) on raw insert/cancel, then on `getBestPrice()` cost after top-of-book churn -- shows the one-time cleanup cost lazy deletion pays. | `heap_vs_tree_benchmark` | see below |

**Via CMake** (after `cmake -S . -B build && cmake --build build`):
```bash
./build/price_impact_study
./build/complexity_benchmark
./build/heap_vs_tree_benchmark
```

**Without CMake** (compile + run directly):
```bash
clang++ -std=c++17 -O2 -Iinclude -Ianalysis -IfairPriceModels \
  src/*.cpp fairPriceModels/OrnsteinUhlenbeck.cpp \
  analysis/PriceImpactStudy.cpp analysis/OrderFlowSimulator.cpp \
  -o price_impact_study && ./price_impact_study

clang++ -std=c++17 -O2 -Iinclude \
  analysis/ComplexityBenchmark.cpp src/*.cpp \
  -o complexity_benchmark && ./complexity_benchmark

clang++ -std=c++17 -O2 -Iinclude -Ianalysis \
  analysis/HeapVsTreeBenchmark.cpp src/*.cpp \
  -o heap_vs_tree_benchmark && ./heap_vs_tree_benchmark
```
(swap `clang++` for `g++` if that's what you have -- both work, no compiler-specific flags are used)

All three read/write nothing outside `analysis/output/` (created automatically
by `price_impact_study`) and take well under a minute to run.

## Troubleshooting

- **"Command not found: cmake"**: Install CMake using your package manager (e.g., `sudo pacman -S cmake` on Arch Linux, `brew install cmake` on macOS), or just use the "Without CMake" commands above -- every program here also builds with a single clang++/g++ invocation.
- **"file not found" errors**: Ensure you are running the commands from the project root directory.

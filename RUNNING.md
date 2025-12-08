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

## Troubleshooting

- **"Command not found: cmake"**: Install CMake using your package manager (e.g., `sudo pacman -S cmake` on Arch Linux).
- **"file not found" errors**: Ensure you are running the commands from the project root directory.

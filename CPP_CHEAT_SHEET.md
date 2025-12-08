# Advanced C++ Cheat Sheet

A comprehensive guide to C++ programming, ranging from essential basics to advanced modern features, with guidelines on when to use each.

## 1. Modern C++ Essentials (C++11/14/17/20)

### Type Inference (`auto`)
Let the compiler deduce the type.
```cpp
auto x = 42;        // int
auto y = 3.14;      // double
auto& ref = x;      // int&
const auto* ptr = &x; // const int*
```
> **When to use:**
> - When the type is obvious (e.g., `auto x = std::make_shared<T>()`).
> - When the type is hard to spell or verbose (e.g., iterators: `std::vector<std::map<int, std::string>>::iterator`).
> - **Avoid** when the type is unclear from context and readability suffers.

### Range-based For Loops
Iterate over containers cleanly.
```cpp
std::vector<int> vec = {1, 2, 3};
for (const auto& val : vec) {
    std::cout << val << " ";
}
```
> **When to use:**
> - Almost always when iterating over an entire container.
> - Use `const auto&` to avoid copying elements.
> - Use `auto&` if you need to modify elements.

### Lambda Expressions
Anonymous functions for local use.
```cpp
auto add = [](int a, int b) { return a + b; };
int sum = add(5, 3);
```
> **When to use:**
> - For short, one-off functions passed to algorithms (e.g., `std::sort`, `std::transform`).
> - When you need to capture local variables for a callback.

## 2. Memory Management (Smart Pointers)

**NEVER** use `new` and `delete` manually unless absolutely necessary. Use `<memory>`.

### `std::unique_ptr`
Exclusive ownership. Cannot be copied, only moved.
```cpp
std::unique_ptr<MyClass> ptr = std::make_unique<MyClass>(10);
// auto ptr2 = ptr; // ERROR: deleted copy constructor
auto ptr2 = std::move(ptr); // OK: ownership transferred
```
> **When to use:**
> - **Default choice** for any heap allocation.
> - When an object has a single clear owner.
> - For polymorphic objects (base class pointers).

### `std::shared_ptr`
Shared ownership via reference counting.
```cpp
std::shared_ptr<MyClass> ptr1 = std::make_shared<MyClass>(20);
std::shared_ptr<MyClass> ptr2 = ptr1; // Ref count = 2
```
> **When to use:**
> - When multiple parts of the code need to keep an object alive independently.
> - In complex data structures (graphs) where ownership is shared.
> - **Warning:** Slower than `unique_ptr` due to reference counting overhead.

## 3. Move Semantics (C++11)

Optimizes performance by transferring resources instead of copying them.

### Move Constructor & Assignment
```cpp
// ... (Buffer example from previous version) ...
```
> **When to use:**
> - When designing classes that manage heavy resources (memory, file handles).
> - To allow efficient return of large objects from functions.
> - When using `std::vector` of heavy objects (reallocations will move instead of copy).

## 4. Templates & Generic Programming

### Function & Class Templates
```cpp
template <typename T>
T add(T a, T b) { return a + b; }
```
> **When to use:**
> - When logic is identical for different data types (e.g., `max(a, b)`).
> - To create generic containers (like `std::vector`).
> - To avoid code duplication.

## 5. Standard Template Library (STL)

### Containers
- **`std::vector`**: Dynamic array.
    > **Use:** 90% of the time. Best cache locality and performance.
- **`std::map` / `std::unordered_map`**: Key-value pairs.
    > **Use:** When you need fast lookups by key. `unordered_map` (O(1)) is faster than `map` (O(log n)) but doesn't keep order.
- **`std::set`**: Unique elements.
    > **Use:** To maintain a sorted collection of unique items.

## 6. Concurrency (`<thread>`, `<mutex>`, `<future>`)

### Threads & Mutexes
```cpp
std::thread t1(worker, 1);
std::lock_guard<std::mutex> lock(mtx);
```
> **When to use:**
> - **Threads:** For parallel execution of long-running tasks.
> - **Mutexes:** To prevent data races when multiple threads access shared memory.

### Async & Futures
```cpp
std::future<int> result = std::async(std::launch::async, compute);
```
> **When to use:**
> - When you just want the *result* of a computation running in the background without manually managing thread lifecycles.

## 7. RAII (Resource Acquisition Is Initialization)

The most important idiom in C++. Resources are acquired in a constructor and released in the destructor.

> **When to use:**
> - **ALWAYS.**
> - For managing ANY resource: memory, files, sockets, mutex locks.
> - Ensures no leaks, even if exceptions are thrown.

## 8. Common Pitfalls

- **Dangling Pointers**: Accessing memory after it has been deleted.
    > **Fix:** Use smart pointers.
- **Slicing**: Copying a derived class object into a base class object by value.
    > **Fix:** Pass by reference or pointer.
- **`virtual` Destructors**:
    > **Rule:** If a class has virtual functions, it **must** have a virtual destructor.

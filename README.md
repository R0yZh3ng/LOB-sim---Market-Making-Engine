# LOB-sim: Limit Order Book Simulator & Market Making Engine

A C++17 limit order book with price-time priority, intended to grow into a market making simulator and backtester.

> **Status: early development.** The order book, its tests and the analysis programs work, but the engine is incomplete. See [Known issues](#known-issues).

## Goals

The target system is a deterministic limit order book simulator and market making engine with:

- realistic event generation
- queue-position-aware fill estimation
- a backtesting pipeline that attributes PnL to spread capture and inventory

These are goals, not current features.

## Component status

| Component | Location | State |
|---|---|---|
| Price level (FIFO queue) | `PriceLevel.{h,cpp}` | Implemented, has tests |
| Order node | `OrderNode.{h,cpp}` | Implemented, has tests |
| Limit order book | `LimitOrderBook.{h,cpp}` | Implemented, has tests, has known bugs |
| Heap-based order book | `HeapLimitOrderBook.{h,cpp}` | Benchmark comparison only |
| Ornstein-Uhlenbeck fair price model | `fairPriceModels/` | Implemented, used by the price impact study |
| Simple market maker | `include/simpleMM.h`, `agents/simpleMM.cpp` | Draft, compiles, not used by any program |
| Analysis and benchmarks | `analysis/` | Three programs |
| Events | `include/Event.h` | Placeholder |
| Event loop | `engine/eventLoop.cpp` | Empty file |
| Risk, clearing, backtesting, PnL attribution | — | Not started |

## Project layout

```
.
├── include/            Headers: order books, price level, order node, event, market maker
├── src/                Order book implementations (built as the lob_lib library)
├── agents/             Trading agents
├── engine/             Simulation engine
├── fairPriceModels/    Fair price processes (built with agents/ as mm_lib)
├── analysis/           Order flow simulator, studies, benchmarks and RESULTS.md
├── tests/              Assert-based test executables
├── CMakeLists.txt
├── LOBdesign.md        Design notes and data structure reasoning
├── RUNNING.md          Build and run instructions
└── CPP_CHEAT_SHEET.md  General C++ reference
```

## Building and testing

You need CMake 3.10 or later and a compiler with C++17 support. Run these from the project root:

```bash
cmake -S . -B build
cmake --build build
```

Then run the tests:

```bash
./build/test_price_level
./build/test_order_node
./build/test_lob
```

The tests rely on `assert`, so don't build them with `NDEBUG` defined (CMake's `Release` configuration defines it), or the checks are skipped.

To look for memory errors, use a separate sanitizer build:

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"
cmake --build build-asan
./build-asan/test_lob
```

## Analysis and benchmarks

The build also produces `price_impact_study`, `complexity_benchmark` and `heap_vs_tree_benchmark`. [RUNNING.md](RUNNING.md) explains how to run them, and [analysis/RESULTS.md](analysis/RESULTS.md) describes the method and a sample run. `price_impact_study` uses a fixed seed, so repeated runs on the same toolchain give the same output. Timing results depend on the machine and build settings.

## Architecture

### Matching rules

The book is designed around these rules:

- **Price priority:** the highest bid and the lowest ask are matched first.
- **Time priority:** orders at the same price fill first in, first out.
- A trade happens when an incoming order crosses the spread (bid ≥ ask).
- The trade price is the resting order's price.
- Any unfilled quantity from a limit order rests on the book.

The matching loop selects which resting orders to fill according to these rules. It doesn't yet record trades or trade prices.

### Data structures

```
LimitOrderBook
├── bidLevels : std::map<double, PriceLevel>             best bid = last element
├── askLevels : std::map<double, PriceLevel>             best ask = first element
└── OrderHashMap : unordered_map<orderId, OrderNode*>    lookup by ID

PriceLevel
head ⇄ OrderNode ⇄ OrderNode ⇄ tail                      oldest → newest
```

- **`OrderNode`** holds one order: ID, price, quantity, side, links to its neighbours and a pointer to its price level.
- **`PriceLevel`** is a doubly linked FIFO queue that also tracks total volume and order count. `removeOrder` and `popHead` delete the node, so nodes added to a level must be allocated with `new`. `unlinkOrder` detaches a node without deleting it.
- **`LimitOrderBook`** keeps a sorted map of price levels for each side, plus a hash map from order ID to node.

Intended costs, where *P* is the number of price levels on a side:

| Operation | Cost |
|---|---|
| Append to or unlink from a level's queue | O(1) |
| Find an order by ID | O(1) average |
| Find the best price | O(1) |
| Find or create a price level | O(log P) |
| Query volume at a price | O(log P) |

[LOBdesign.md](LOBdesign.md) explains why this design was chosen over heaps. It suggests a hash map for O(1) volume queries, but the code reads the volume from each side's `std::map`.

## API reference

`LimitOrderBook` is declared in [`include/LimitOrderBook.h`](include/LimitOrderBook.h). Sides are `Side::BUY` and `Side::SELL`. This section describes current behaviour.

| Method | Description |
|---|---|
| `uint64_t placeLimitOrder(double price, uint64_t quantity, Side side)` | Adds an order, matches it against the opposite side and returns its ID. Any remainder rests on the book. |
| `uint64_t placeMarketOrder(uint64_t quantity, Side side)` | Places an order at the best opposing price. It fills only at that level, and any remainder rests on the book as a limit order. Throws `std::invalid_argument` if the opposite side is empty. |
| `void cancelOrder(uint64_t orderId)` | Removes a resting order. Throws `std::invalid_argument` if the ID is unknown, already cancelled or fully filled. |
| `void modifyOrder(double price, uint64_t orderId, uint64_t quantity, Side side)` | Changes price and quantity, moving the order to the back of the queue. It doesn't match the result. `side` must be the order's original side, but this isn't checked. Throws `std::invalid_argument` for an unknown ID. |
| `double getBestPrice(Side side) const` | Returns the highest bid for `BUY` or the lowest ask for `SELL`. Throws `std::invalid_argument` if that side is empty. |
| `uint64_t getVolumeAtPrice(double price) const` | Returns the combined bid and ask volume at a price, or 0 if there is no level. |
| `bool hasOrders(Side side) const` | Returns whether that side has any resting orders. |

The book frees its resting orders when destroyed and can't be copied.

Example:

```cpp
#include "LimitOrderBook.h"

LimitOrderBook book;
book.placeLimitOrder(101.0, 10, Side::SELL);   // rests on the ask side
book.placeLimitOrder(101.0, 4, Side::BUY);     // fills 4 against it

book.getBestPrice(Side::SELL);                 // 101.0
book.getVolumeAtPrice(101.0);                  // 6
```

## Fair price model

`OrnsteinUhlenbeck` simulates a mean-reverting price using an Euler-Maruyama step:

```
price += kappa * (theta - price) * dt + sigma * sqrt(dt) * Z,    Z ~ N(0, 1)
```

| Parameter | Meaning |
|---|---|
| `initial` | Starting price |
| `theta` | Long-run mean |
| `kappa` | Speed of mean reversion |
| `sigma` | Volatility |
| `dt` | Time step per `update()` call |
| `seed` | Random generator seed |

`update()` advances one step and returns the new price. `getPrice()` returns the current price.

The same parameters and seed give the same price path on the same toolchain. `std::normal_distribution` can differ between standard library implementations. If another generator runs alongside the model, give it a different seed.

## Market making agent

`simpleMM` is a draft quoting strategy. On each `update(fairPrice)`, it is meant to cancel its previous quotes and place new ones:

```
skew = inventory * invRisk
bid  = fairPrice - spread - skew
ask  = fairPrice + spread - skew
```

`spread` is the offset on **each side** of the fair price, so the quoted bid-ask width is `2 * spread`. `size` is the quantity of each quote. A positive (long) inventory lowers both quotes, assuming `invRisk` is positive.

Because `cancelOrder` throws for a filled order, `update` throws if a previous quote was fully filled. Fills aren't tracked yet, and `update` resets inventory to 0 as a placeholder. The skew will work properly only after risk and clearing are implemented.

## Known issues

- **Market maker:** `simpleMM::update` doesn't handle the exception from cancelling a filled quote.
- **Market orders:** they don't sweep past the best opposing level, and the unfilled remainder rests on the book.
- **Modify:** a modified order that crosses the spread isn't matched.
- **Book-wide volume:** the `totalVolume` member is never increased, only decreased on fills, so the unsigned value wraps around.
- **Quantity range:** the API accepts `uint64_t`, but `OrderNode` stores `unsigned int`, so large values are truncated.
- **Prices:** `double` keys mean that computed prices which should be equal can create separate levels. Integer ticks would avoid this.
- **Order IDs:** the counter is `static`, so all instances of a book class share it.

## Roadmap

Suggested order of work:

1. Fix market orders and modify.
2. Report trades from the matching loop.
3. Use integer ticks for prices.
4. Add risk and clearing so each user's inventory is tracked from fills.
5. Connect the market maker to the simulator and handle filled quotes.
6. Implement events and the event loop.
7. Add queue-position-aware fill estimation, backtesting and PnL attribution.

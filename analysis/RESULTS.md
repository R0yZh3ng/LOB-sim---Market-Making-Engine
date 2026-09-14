## What this directory is

Backs up the resume line:

> Modeled mid-price evolution and spread dynamics under controlled order flow,
> isolating effects of queue imbalance on short-term price impact; analyzed
> O(log n) vs O(1) tradeoffs under high-frequency submission scenarios.

Two standalone programs, both built on top of the real `LimitOrderBook`
(no separate toy model):

- `PriceImpactStudy.cpp` + `OrderFlowSimulator.cpp` -- drives synthetic,
  parameter-controlled order flow through the book and studies mid-price /
  spread dynamics and the relationship between top-of-book queue imbalance
  and forward price moves.
- `ComplexityBenchmark.cpp` -- times `placeLimitOrder`/`cancelOrder` on the
  real book at increasing depth, plus an isolated `unordered_map` vs `map`
  microbenchmark, to empirically show the O(1) vs O(log n) split the engine
  is designed around.
- `HeapVsTreeBenchmark.cpp` -- times the real (tree-based) `LimitOrderBook`
  against `HeapLimitOrderBook`, an alternate implementation backed by two
  `std::priority_queue`s instead of two `std::map` price ladders, to show
  where a heap wins (raw insert/cancel) and where it loses (lazy-deletion
  cleanup cost on `getBestPrice()` after top-of-book churn).

Build (no CMake required):

```
clang++ -std=c++17 -O2 -Iinclude -Ianalysis -IfairPriceModels \
  src/*.cpp fairPriceModels/OrnsteinUhlenbeck.cpp \
  analysis/PriceImpactStudy.cpp analysis/OrderFlowSimulator.cpp \
  -o price_impact_study

clang++ -std=c++17 -O2 -Iinclude analysis/ComplexityBenchmark.cpp src/*.cpp \
  -o complexity_benchmark

clang++ -std=c++17 -O2 -Iinclude -Ianalysis analysis/HeapVsTreeBenchmark.cpp \
  src/*.cpp -o heap_vs_tree_benchmark
```

Or via CMake (`price_impact_study` / `complexity_benchmark` /
`heap_vs_tree_benchmark` targets). See `RUNNING.md` for the full list of
build/run commands for every program in this repo.

## Methodology

**Order flow simulator**: an Ornstein-Uhlenbeck process (`fairPriceModels/OrnsteinUhlenbeck.*`)
drives a latent fair price. Each event, a synthetic order arrives whose side
is drawn with a fixed `buyProbability` (the "controlled" flow parameter),
sized randomly, and priced either passively near the fair price or
aggressively enough to cross and sweep the book (probability
`aggressiveProbability`). A cancellation-churn step removes random resting
orders each event so the book reaches a steady-state depth instead of
growing without bound. Before each event we record best bid/ask, mid price,
spread, and top-of-book volume imbalance
`(bidVol - askVol) / (bidVol + askVol)`; after the event we record the
resulting mid price. 20,000 events per scenario, fixed RNG seed across
scenarios so differences are attributable to `buyProbability`, not noise.

**Price impact**: for horizons k = 1, 5, 10, 20 events, we pair each event's
pre-event imbalance with the mid-price change k events later, then compute
the Pearson correlation and an OLS regression slope (the price-impact
coefficient) across all 20,000 events.

**Heap vs tree benchmark**: `HeapLimitOrderBook` swaps the real book's two
`std::map<double, PriceLevel>` price ladders for two `std::priority_queue`s
(max-heap for bids, min-heap for asks) plus `std::unordered_map<double,
PriceLevel>` for the actual level data. A heap can't erase an arbitrary
interior element cheaply, so cancelling the last order at a price level
removes it from the level map (O(1) average) but leaves its price sitting
in the heap; `getBestPrice()` lazily pops any such stale price off the top
before returning a real one -- this is the standard "lazy deletion"
technique. Part A seeds both books identically and times raw
`placeLimitOrder`/`cancelOrder` at increasing depth. Part B specifically
targets the lazy-deletion cost: it fires N transient orders priced above a
small resting core book, cancelling each one immediately, then times the
*first* `getBestPrice()` call afterward (which must pay off the entire
backlog in one pass) against the *second* call right after (which should be
cheap again, since the backlog was just drained).

**Complexity benchmark**: Part A builds a `LimitOrderBook` with book depth N
spread over ~N/40 distinct price levels per side, then times 5,000 sampled
`placeLimitOrder` calls (touches the `std::map` price ladder) and 5,000
sampled `cancelOrder` calls (touches only the `unordered_map`) via
`std::chrono::steady_clock`, for N in {1k, 5k, 10k, 50k, 100k, 200k}. Part B
isolates the two container types directly (`unordered_map<uint64_t,...>` vs
`map<uint64_t,...>`, both holding N entries, timing 20,000 random-key
lookups) to show the underlying complexity classes without the linked-list
matching mechanics in the way.

## Results (representative run; see `analysis/output/*.csv` for raw data)

### Mid-price / spread dynamics under controlled order flow

| scenario | buyProbability | mean spread | spread std | mid-price step std dev |
|---|---|---|---|---|
| sell_heavy_flow | 0.35 | 0.0364 | 0.0306 | 0.0120 |
| neutral_flow | 0.50 | 0.0318 | 0.0262 | 0.0105 |
| buy_heavy_flow | 0.65 | 0.0337 | 0.0275 | 0.0110 |

Skewing order flow away from neutral (in either direction) widens the mean
spread and raises mid-price volatility relative to the neutral baseline --
consistent with one-sided flow consuming liquidity faster than it's
replenished on that side.

### Queue imbalance -> forward price impact (neutral_flow scenario)

| horizon (events ahead) | corr(imbalance, fwd Δmid) | OLS slope | R^2 |
|---|---|---|---|
| k=1  | 0.129 | 0.0023 | 0.017 |
| k=5  | 0.222 | 0.0081 | 0.049 |
| k=10 | 0.267 | 0.0128 | 0.071 |
| k=20 | 0.293 | 0.0181 | 0.086 |

Sign and shape are the expected microstructure result: more resting bid
volume than ask volume at the top of book predicts the mid price drifting
*up* over the following events, and the relationship strengthens as the
horizon lengthens (more of the imbalance's information gets realized into
price) before presumably decaying at longer horizons than tested here.
Magnitudes are modest (R^2 under 10%) because a large share of this
simulation's price motion is exogenous (the underlying OU fair price walks
on its own) rather than purely order-flow-driven -- a real order book would
show a similar shape with different magnitudes depending on how much of the
price process is endogenous to the book.

### Heap-based vs tree-based LimitOrderBook

**Part A -- raw insert/cancel at increasing depth** (nanoseconds/op):

| book depth | tree insert | heap insert | tree cancel | heap cancel |
|---|---|---|---|---|
| 1,000 | 113.5 | 72.9 | 114.4 | 93.2 |
| 5,000 | 282.0 | 163.0 | 253.2 | 197.2 |
| 10,000 | 269.7 | 142.9 | 212.0 | 163.2 |
| 50,000 | 365.7 | 145.4 | 259.7 | 119.7 |
| 100,000 | 483.9 | 199.4 | 393.2 | 220.3 |
| 200,000 | 703.6 | 159.6 | 758.7 | 423.2 |

The heap wins on raw insert/cancel at every depth tested. Both container
types are the same theoretical class here (heap push and map insert are
both O(log P) in the number of distinct price levels), but `unordered_map`
lookups (used for both books' order-id -> node table, and for the heap
book's price -> level table) beat `std::map`'s pointer-chasing red-black
tree on cache locality, so the heap version comes out ahead in practice.

**Part B -- cost of the first `getBestPrice()` call after N transient
top-of-book create+cancel events:**

| churn events | stale backlog | tree 1st call | heap 1st call | heap 2nd call |
|---|---|---|---|---|
| 0 | 0 | 42 ns | 0 ns | 42 ns |
| 1,000 | 1,000 | 0 ns | 79,500 ns | 42 ns |
| 5,000 | 5,000 | 0 ns | 507,834 ns | 42 ns |
| 20,000 | 20,000 | 0 ns | 2,288,084 ns | 41 ns |
| 50,000 | 50,000 | 3,291 ns | 6,020,750 ns | 42 ns |
| 100,000 | 100,000 | 0 ns | 12,363,083 ns | 42 ns |

This is the actual tradeoff lazy deletion makes: cost isn't paid per
cancel, it's deferred and paid in one lump sum by whichever caller happens
to ask for the best price after a stretch of top-of-book churn. The heap's
first call after churn costs ~120 ns per accumulated stale entry (scaling
linearly with the backlog -- 100,000 churned orders costs over 12
milliseconds for a single price lookup), while the tree-based book, which
erases dead price levels immediately on cancel, never accumulates a
backlog and stays flat regardless of churn history. The heap's *second*
call right after drops straight back to ~42 ns, confirming the cost is a
one-time payoff, not a recurring one -- so a heap-based book would be fine
under steady insert/cancel load but pathological under bursty top-of-book
churn (e.g. HFT quote flickering) if `getBestPrice()` isn't called
frequently enough to keep the backlog small.

### O(1) vs O(log n): LimitOrderBook under increasing depth

| book depth | price levels/side | avg insert (map-touching), ns | avg cancel (hash-only), ns |
|---|---|---|---|
| 1,000 | 25 | 131 | 104 |
| 5,000 | 125 | 158 | 135 |
| 10,000 | 250 | 178 | 146 |
| 50,000 | 1,250 | 315 | 253 |
| 100,000 | 2,500 | 458 | 335 |
| 200,000 | 5,000 | 560 | 595 |

### O(1) vs O(log n): isolated container microbenchmark

| n | avg unordered_map lookup, ns | avg map lookup, ns | ratio |
|---|---|---|---|
| 1,000 | 2.2 | 44.7 | 20.5x |
| 5,000 | 3.1 | 69.2 | 22.0x |
| 10,000 | 3.6 | 73.0 | 20.1x |
| 50,000 | 6.2 | 106.9 | 17.6x |
| 100,000 | 6.6 | 115.5 | 17.7x |
| 200,000 | 9.1 | 156.2 | 17.6x |

Reading these together: the isolated microbenchmark (Part B) cleanly shows
the algorithmic split -- `unordered_map` lookup stays in single-digit
nanoseconds while `std::map` lookup grows several-fold as n grows 200x,
a 17-22x advantage that holds across the whole range. In the full engine
(Part A), both operations' absolute cost grows with book depth too, because
larger heaps mean worse cache locality for the pointer-chasing linked lists
and red-black tree, not because either operation stopped being O(1)/O(log n)
algorithmically -- cancel (hash-only) stays cheaper than insert (map-touching)
at every depth up to 100k, converging at 200k where memory-locality effects
dominate the algorithmic term. That's the honest tradeoff: `unordered_map`
buys you the better complexity class, but at large scale constant-factor
memory effects start to matter as much as big-O.

## Bugs found and fixed while building this

Building a stress test that hammers `cancelOrder`/`placeLimitOrder` at high
frequency surfaced three pre-existing use-after-free bugs in the matching
engine (all: reading a field on an `OrderNode*` *after* the code path that
deletes it):

1. `cancelOrder` read `nodeToCancel->price` after `removeOrder()` had already
   deleted the node, and never erased the id from `OrderHashMap` at all
   (leaving a dangling pointer behind for any future lookup of that id).
2. `modifyOrder` called `PriceLevel::removeOrder` (which deletes the node) and
   then continued to write `order->price`/`order->quantity` on the freed
   node. Fixed by adding `PriceLevel::unlinkOrder`, which does the same
   linked-list bookkeeping without deleting, so a modified order can be
   relocated to a new price level while keeping the same identity.
3. `placeOrder`'s fully-filled-on-entry path read `ordn->price`/`ordn->orderId`
   after `removeOrder()` had deleted `ordn`.

All three were confirmed via AddressSanitizer (clean before/after), and the
existing unit test suite (`test_lob`, `test_price_level`, `test_order_node`)
still passes. A fourth bug -- `placeMarketOrder` priced against
`getBestPrice(side)` (its own side) instead of the opposite side, so a
market order never actually crossed the book -- was fixed too, though the
analysis tooling here doesn't call it directly (it builds marketable orders
manually via `placeLimitOrder` for tighter control over the simulation).

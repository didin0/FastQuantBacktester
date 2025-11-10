# FastQuantBacktester (minimal)

This workspace contains the initial CSV data loader (F1) implementation.

Files added:
- `src/DataLoader/Candle.h` — Candle struct (ISO8601 timestamp + OHLCV + optional symbol).
- `src/DataLoader/CSVDataLoader.h/.cpp` — CSV loader, lenient parsing by default.
- `examples/sample_prices.csv` — example CSV with 3 valid rows and 1 malformed row.
- `tests/test_csv_loader.cpp` — Catch2 test that verifies parsing behavior.
- `CMakeLists.txt` — minimal build to compile the loader and run tests via Catch2 (FetchContent).

Build & test (WSL / Linux):

```bash
# create build
mkdir -p build && cd build
cmake ..
cmake --build . -- -j
ctest --output-on-failure
```

Quickstart — CSV loader and engine
----------------------------------

CSV format (expected canonical columns):

- timestamp (ISO8601 UTC, e.g. 2025-11-10T00:00:00Z)
- open (float)
- high (float)
- low (float)
- close (float)
- volume (float)
- symbol (optional)

API summary:

- `CSVDataLoader::load(path, cfg)` — loads all candles into a std::vector<Candle>.
- `CSVDataLoader::stream(path, callback, cfg)` — streams candles and calls your callback for each one (supports early-stop).
- `Strategy` — abstract base class for strategies (onStart/onData/onFinish). Use `setOrderSink()` to enable order submission.
- `MovingAverageStrategy` — example SMA crossover strategy that emits orders on crossovers.
- `BacktestEngine::run(path, cfg, strategy, outTrades)` — runs a backtest by streaming candles into the strategy and collects executed `Trade`s.

Environment flags for tests & perf
--------------------------------

- `ENABLE_PERF=1` — opt-in flag to enable the perf benchmark test (`tests/bench_csv_perf.cpp`). Disabled by default to keep CI fast.
- `FQ_BENCH_ROWS` — when `ENABLE_PERF=1`, controls how many rows the perf generator will create (default 100000).

Example: run local perf with 1M rows

```bash
export ENABLE_PERF=1
export FQ_BENCH_ROWS=1000000
mkdir -p build && cd build
cmake ..
cmake --build . -- -j
ctest --output-on-failure -V
```

Next steps and notes
--------------------

- The current execution model in `BacktestEngine` is immediate: `Order`s submitted by strategies are converted to `Trade`s and executed at the order price. This is simple and deterministic; later iterations should add more realistic fills (next-open fills, slippage, fees, partial fills).
- F2 work (Strategy + BacktestEngine improvements) has been started: see `src/Strategy` and `src/BacktestEngine` for the scaffold and an example `MovingAverageStrategy`.

If you'd like, I can open a small PR that:
- Adds epoch-ms timestamp parsing tests
- Exposes `strict` config via a CLI helper
- Adds position/account P&L tracking (positions and trades integration)


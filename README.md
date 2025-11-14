# FastQuantBacktester

This repo now contains:

- **F1 – CSV Data Loader** with ISO8601 + epoch (sec / ms) timestamps, lenient/strict modes, streaming API, and perf benchmark.
- **F2 – Strategy & Engine skeleton** including `Strategy` base class, a `MovingAverageStrategy`, and a `BacktestEngine` that streams candles, executes orders, and records trades.
- **Positions / PnL module** providing a `Portfolio` that tracks cash, positions, and realized/unrealized PnL. `BacktestEngine::run` returns a `BacktestResult` containing trades + portfolio snapshot.

Reference files:
- `src/DataLoader/Candle.h`
- `src/DataLoader/CSVDataLoader.{h,cpp}`
- `src/Strategy/Strategy.h`, `src/Strategy/MovingAverageStrategy.{h,cpp}`
- `src/Model/Order.h`, `src/Model/Trade.h`, `src/Model/Portfolio.{h,cpp}`
- `src/BacktestEngine/BacktestEngine.{h,cpp}`
- Tests under `tests/` (Catch2) and sample data in `examples/`
- `CMakeLists.txt` + `.github/workflows/ci.yml`

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
- `CSVDataLoader::stream(path, callback, cfg)` — streams candles and calls your callback for each one (supports early-stop). Timestamps accept ISO8601 or numeric epoch (sec/ms).
- `Strategy` — abstract base class for strategies (onStart/onData/onFinish). Use `setOrderSink()` to enable order submission.
- `MovingAverageStrategy` — example SMA crossover strategy that emits orders on crossovers.
- `BacktestEngine::run(path, cfg, strategy, initialCapital)` — returns a `BacktestResult { candlesProcessed, trades, portfolio }`.
- `Portfolio` — maintains cash, signed positions, avg price, realized & unrealized PnL. Automatically updated by the engine when trades execute.
- `Reporter` — builds performance summaries (P&L, drawdown, win/loss counts) and exports JSON / CSV files (summary + trade log + equity curve).

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

- The current execution model in `BacktestEngine` is immediate: `Order`s submitted by strategies are converted to `Trade`s and executed at the order price. This keeps things deterministic and makes it easy to validate the Strategy/Portfolio pipeline. Upcoming improvements: model fills at next open/high/low, add slippage/fees, and support order types.
- Multithreaded backtests, CLI/config plumbing, and a Qt or web UI are the next roadmap milestones.

Want help next?
- wire strict-mode/config flags into a CLI or JSON config loader
- add JSON/CSV reporting of portfolio metrics (profit, drawdown, win-rate, trade log)
- build a simple Qt or web dashboard that consumes `BacktestResult`
- extend the perf benchmark (1M+ rows) and publish results


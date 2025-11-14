# FastQuantBacktester

FastQuantBacktester is a modern C++20 playground for running trading strategy experiments quickly. It ships with a blazing-fast CSV loader (ISO8601 + epoch timestamps, lenient/strict parsing, streaming), a plug-in `Strategy` interface with a sample moving-average crossover, a deterministic `BacktestEngine` that converts strategy orders into trades and keeps a `Portfolio` (cash, positions, realized/unrealized PnL), and a `Reporter` that summarizes the run to JSON/CSV.

Everything is dependency-light (fast-cpp-csv-parser, spdlog, Catch2, nlohmann/json) and glued together through small modules you can re-use in other apps.

## How to use it

```bash
# Configure + build (WSL/Linux)
mkdir -p build && cd build
cmake ..
cmake --build . -- -j

# Run the full Catch2 suite
ctest --output-on-failure

# Build the CLI runner (if not already built)
cmake --build . --target fastquant_cli

# Execute a backtest via JSON config
./fastquant_cli ../examples/fastquant.config.json
```

`examples/fastquant.config.json` points to `examples/sample_prices.csv` and defines:

```json
{
  "data": { "path": "examples/sample_prices.csv", "strict": false },
  "strategy": { "type": "moving_average", "short_window": 5, "long_window": 20 },
  "engine": { "initial_capital": 100000 },
  "reporter": {
    "json": "reports/latest/report.json",
    "summary_csv": "reports/latest/summary.csv",
    "trades_csv": "reports/latest/trades.csv",
    "print_summary": true
  }
}
```

When you run `fastquant_cli`:

1. The loader streams candles into the moving-average strategy.
2. The engine executes signals immediately and updates the portfolio/equity curve.
3. The reporter prints a concise summary (optional) and drops JSON/CSV artifacts to the paths defined in the config. Relative paths are resolved against the config file location.

That’s it—clone, build, tweak the config, and iterate on new strategies or reporters as needed.


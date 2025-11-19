# FastQuantBacktester

FastQuantBacktester is a modern C++20 playground for running trading strategy experiments quickly. It ships with a blazing-fast CSV loader (ISO8601 + epoch timestamps, lenient/strict parsing, streaming), a plug-in `Strategy` interface with a sample moving-average crossover, a deterministic `BacktestEngine` that converts strategy orders into trades and keeps a `Portfolio` (cash, positions, realized/unrealized PnL), and a `Reporter` that summarizes the run to JSON/CSV.

Everything is dependency-light (fast-cpp-csv-parser, spdlog, Catch2, nlohmann/json, libcurl) and glued together through small modules you can re-use in other apps.

### Execution realism (F2+)

Orders now carry type (`market` / `limit`), time-in-force, and optional per-order slippage overrides. `BacktestEngine` simulates fills using candle OHLC data, applies configurable slippage/commission (bps + per-share), tracks total fees/slippage, and records whether orders were filled or rejected (e.g., IOC expirations). These metrics flow into the `Reporter` outputs and the CLI JSON summary automatically.

### Parallel multi-strategy runs (F4)

List multiple strategy blocks in the JSON config and the engine will spin them up concurrently via `std::async`. Each strategy keeps an isolated portfolio, but they reuse the same CSV source and execution settings. Report files inherit the configured paths and automatically append a `_strategy-name_<index>` suffix so artifacts never clobber each other.

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

# Execute a backtest via JSON config (positional or --config)
./fastquant_cli ../examples/fastquant.config.json

CLI flags:

- `-c, --config <path>` – select config file (defaults to `fastquant.config.json`).
- `--validate` – load/validate the config, then exit without running.
- `--print-config` – echo the resolved, absolute paths and execution parameters.
- `--no-summary` – suppress stdout summary (useful when only artifacts matter).
- `--version`, `-h/--help` – discover metadata or usage.
```

`examples/fastquant.config.json` now demonstrates running both a CSV data source and a live-style API source:

```json
{
  "data": {
    "source": "csv",
    "path": "examples/sample_prices.csv",
    "strict": false
  },
  "strategies": [
    { "name": "sma_fast", "type": "moving_average", "short_window": 5, "long_window": 20 },
    { "name": "breakout_20", "type": "breakout", "breakout_lookback": 20, "breakout_buffer": 0.5, "order_quantity": 50, "allow_short": true }
  ],
  "engine": {
    "initial_capital": 100000,
    "execution": {
      "default_slippage_bps": 25,
      "commission_per_share": 0.01,
      "commission_bps": 5
    }
  },
  "reporter": {
    "json": "reports/latest/report.json",
    "summary_csv": "reports/latest/summary.csv",
    "trades_csv": "reports/latest/trades.csv",
    "print_summary": true
  }
}
```

Need live data instead of CSV? Swap the `data` block for an API connector:

```json
"data": {
  "source": "api",
  "endpoint": "https://api.binance.com/api/v3/klines",
  "symbol": "BTCUSDT",
  "headers": { "X-MBX-APIKEY": "${BINANCE_API_KEY}" },
  "query": { "symbol": "BTCUSDT", "interval": "1h", "limit": "500" },
  "fields": {
    "timestamp": "0",
    "open": "1",
    "high": "2",
    "low": "3",
    "close": "4",
    "volume": "5",
    "symbol": ""
  }
}
```

### 🔐 Managing API secrets with `.env`

1. Duplicate `.env.example` to `.env` (which is gitignored) and drop your real keys:

  ```
  BINANCE_API_KEY=live-key-here
  ```

2. Any string value inside the JSON config can reference `${VAR_NAME}` and it will be expanded before the run. In the example above, `"${BINANCE_API_KEY}"` pulls from the `.env` file and keeps secrets out of version control.

3. Numeric strings inside `data.fields` are treated as array indexes, which lets the API loader consume Binance-style `[openTime, open, high, ...]` payloads. Leave `fields.symbol` empty and the loader will fall back to the `data.symbol` value.
4. You can also keep multiple files such as `.env.local`; the CLI automatically attempts to load `.env`, `.env.local`, and a `.env` that lives next to the config file.

The new `APIDataLoader` uses libcurl+nlohmann/json under the hood, streams the remote OHLCV payload into candles, and feeds every configured strategy in parallel. See `examples/api_example.config.json` for a ready-to-tweak template.

### Strategy menu

Define one or many entries under `"strategies"`—each entry needs a unique `name` so the CLI can label summaries and report artifacts. (The legacy single `"strategy"` block still works; it simply becomes the first entry.)

- `moving_average` (default): classic short/long SMA crossover controlled by `short_window` and `long_window`.
- `breakout`: watches rolling highs/lows over `breakout_lookback` candles, buys above the high plus `breakout_buffer`, and (optionally) sells short below the low minus the buffer. Use `order_quantity` to size the fills and `allow_short` to toggle breakdown shorts.

Example breakout entry:

```json
{
  "name": "breakout_us_eq",
  "type": "breakout",
  "breakout_lookback": 20,
  "breakout_buffer": 0.5,
  "order_quantity": 100,
  "allow_short": true
}
```

When you run `fastquant_cli`:

1. The loader streams candles (from CSV or API) into each configured strategy (moving-average, breakout, etc.).
2. The engine prices market/limit orders off candle OHLC, applies slippage + commissions, and tracks fills/rejections per strategy.
3. The reporter prints concise per-strategy summaries (optional) – now including fee/slippage totals – and drops JSON/CSV artifacts suffixed with the strategy name + index (e.g., `_breakout_20_2`). Relative paths are resolved against the config file location.

That’s it—clone, build, tweak the config, and iterate on new strategies or reporters as needed.


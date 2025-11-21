# FastQuantBacktester 🚀# FastQuantBacktester



**FastQuantBacktester** is a high-performance, modular C++ trading simulation engine that comes with a modern **Web Dashboard**. It allows you to backtest trading strategies on historical CSV data or live crypto data from Binance, visualizing the results instantly.FastQuantBacktester is a modern C++20 playground for running trading strategy experiments quickly. It ships with a blazing-fast CSV loader (ISO8601 + epoch timestamps, lenient/strict parsing, streaming), a plug-in `Strategy` interface with a sample moving-average crossover, a deterministic `BacktestEngine` that converts strategy orders into trades and keeps a `Portfolio` (cash, positions, realized/unrealized PnL), and a `Reporter` that summarizes the run to JSON/CSV.



![Dashboard Preview](https://via.placeholder.com/800x400?text=FastQuantBacktester+Dashboard)Everything is dependency-light (fast-cpp-csv-parser, spdlog, Catch2, nlohmann/json, libcurl) and glued together through small modules you can re-use in other apps.



## ✨ Features### Execution realism (F2+)



- **⚡ High Performance**: Built with C++20 for blazing fast simulations.Orders now carry type (`market` / `limit`), time-in-force, and optional per-order slippage overrides. `BacktestEngine` simulates fills using candle OHLC data, applies configurable slippage/commission (bps + per-share), tracks total fees/slippage, and records whether orders were filled or rejected (e.g., IOC expirations). These metrics flow into the `Reporter` outputs and the CLI JSON summary automatically.

- **📊 Interactive Dashboard**: Visualize Equity Curves and Trade History with Chart.js.

- **🔌 Multiple Data Sources**:### Parallel multi-strategy runs (F4)

  - **CSV**: Load local historical data files.

  - **Binance API**: Fetch historical candles directly from Binance (e.g., BTCUSDT).List multiple strategy blocks in the JSON config and the engine will spin them up concurrently via `std::async`. Each strategy keeps an isolated portfolio, but they reuse the same CSV source and execution settings. Report files inherit the configured paths and automatically append a `_strategy-name_<index>` suffix so artifacts never clobber each other.

- **🧠 Built-in Strategies**:

  - **Moving Average Crossover**: Classic trend-following.## How to use it

  - **Breakout**: Volatility-based entry/exit.

- **🛠️ Easy Configuration**: Use Presets for quick setup or customize parameters manually.```bash

# Configure + build (WSL/Linux)

## 🚀 Quick Startmkdir -p build && cd build

cmake ..

### Prerequisitescmake --build . -- -j

- **CMake** (3.10+)

- **C++ Compiler** (GCC, Clang, or MSVC supporting C++20)# Run the full Catch2 suite

- **OpenSSL** (for API support)ctest --output-on-failure



### One-Click Run# Build the CLI runner (if not already built)

cmake --build . --target fastquant_cli

We provide helper scripts to build and run the server automatically.

# Execute a backtest via JSON config (positional or --config)

**Windows (Command Prompt / PowerShell):**./fastquant_cli ../examples/fastquant.config.json

```cmd

.\run.batCLI flags:

```

- `-c, --config <path>` – select config file (defaults to `fastquant.config.json`).

**Linux / WSL:**- `--validate` – load/validate the config, then exit without running.

```bash- `--print-config` – echo the resolved, absolute paths and execution parameters.

chmod +x run.sh- `--no-summary` – suppress stdout summary (useful when only artifacts matter).

./run.sh- `--version`, `-h/--help` – discover metadata or usage.

``
Once the server starts, open your browser and navigate to:`examples/fastquant.config.json` now demonstrates running both a CSV data source and a live-style API source:

👉 **http://localhost:8080**

``json

---{

  "data": {

## 📖 Usage Guide    "source": "csv",

    "path": "examples/sample_prices.csv",

### 1. Load Data    "strict": false

- **CSV Mode**: Select an example file from the dropdown or enter a path to your own CSV.  },

- **API Mode**: Select a symbol (e.g., `BTCUSDT`) and interval (e.g., `1h`). Click **Load Data**.  "strategies": [

    { "name": "sma_fast", "type": "moving_average", "short_window": 5, "long_window": 20 },

### 2. Configure Strategy    { "name": "breakout_20", "type": "breakout", "breakout_lookback": 20, "breakout_buffer": 0.5, "order_quantity": 50, "allow_short": true }

- Choose between **Moving Average** (SMA) or **Breakout**.  ],

- Adjust parameters like `Period`, `Lookback`, or `Threshold`.  "engine": {

    "initial_capital": 100000,

### 3. Run Backtest    "execution": {

- Click **Run Backtest**.      "default_slippage_bps": 25,

- The **Equity Curve** chart will update instantly.      "commission_per_share": 0.01,

- Scroll down to see the **Recent Trades** table with buy/sell details.      "commission_bps": 5

    }

---  },

  "reporter": {

## 🏗️ Tech Stack    "json": "reports/latest/report.json",

    "summary_csv": "reports/latest/summary.csv",

- **Backend**: C++20, `cpp-httplib`, `nlohmann/json`, `libcurl`, `fast-cpp-csv-parser`.    "trades_csv": "reports/latest/trades.csv",

- **Frontend**: HTML5, CSS3, Vanilla JavaScript, Chart.js.    "print_summary": true

- **Build**: CMake.  }

}

## 📂 Project Structure```



```Need live data instead of CSV? Swap the `data` block for an API connector:

FastQuantBacktester/

├── src/```json

│   ├── Server/        # HTTP Server & API Handlers"data": {

│   ├── DataLoader/    # CSV & API Data Loading  "source": "api",

│   ├── Strategy/      # Trading Logic (MA, Breakout)  "endpoint": "https://api.binance.com/api/v3/klines",

│   ├── BacktestEngine/# Simulation Core  "symbol": "BTCUSDT",

│   └── Reporter/      # JSON/CSV Output  "headers": { "X-MBX-APIKEY": "${BINANCE_API_KEY}" },

├── frontend/          # Web Dashboard (HTML/JS/CSS)  "query": { "symbol": "BTCUSDT", "interval": "1h", "limit": "500" },

├── data/              # Example CSV files  "fields": {

├── run.bat            # Windows Launcher    "timestamp": "0",

├── run.sh             # Linux Launcher    "open": "1",

└── CMakeLists.txt     # Build Configuration    "high": "2",

```    "low": "3",

    "close": "4",

## 🤝 Contributing    "volume": "5",

    "symbol": ""

Contributions are welcome! Feel free to submit a Pull Request or open an Issue.  }

}

## 📄 License```



MIT License.### 🔐 Managing API secrets with `.env`


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



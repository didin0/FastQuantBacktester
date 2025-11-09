# 🤖 GitHub Copilot Instructions – FastQuantBacktester

## 🎯 Project Overview
FastQuantBacktester is a modular C++ trading simulation engine designed to:
- Load and process financial market data (CSV or API)
- Run multiple trading strategies on historical data
- Backtest and calculate performance metrics
- Generate reports (JSON/CSV)
- Optionally visualize results (CLI or Qt)

The project emphasizes **architecture**, **performance**, and **scalability** — not complex math.

---

## 🧩 Project Architecture (Copilot Context)

**Core Modules:**
1. `DataLoader` → Reads market data from CSV.
2. `Strategy` → Base class for custom strategies.
3. `BacktestEngine` → Runs strategy logic on historical data.
4. `Trade` / `Order` → Represent trades & positions.
5. `Reporter` → Outputs results and statistics.

**Optional Modules:**
- `Visualizer` → Qt-based graph display (later phase).
- `APIConnector` → Loads live data (future feature).

---

## 🧠 Copilot Guidance Rules

### 1. General Coding Style
- Use **C++20 standard** and **modern OOP** (smart pointers, RAII, STL).
- Follow **SOLID** principles and modular design.
- Comment code clearly — explain *why*, not just *what*.
- Avoid external dependencies unless specified.
- Use `clang-format` conventions for readability.

---

### 2. Copilot Behavior per Folder

#### `/src/DataLoader/`
> “Copilot, implement a C++ class `CSVDataLoader` that efficiently reads large CSV files with millions of rows using fast-cpp-csv-parser.  
Include error handling for missing or malformed data. Provide getters to access data as vectors of structs.”

#### `/src/Strategy/`
> “Create a base abstract class `Strategy` with virtual methods `onData()` and `onFinish()`.  
Then define two concrete strategies:  
- `MovingAverageStrategy` (simple SMA crossover)  
- `BreakoutStrategy` (price breakout detection).”

#### `/src/BacktestEngine/`
> “Implement `BacktestEngine` that loops through all price data points and executes a given strategy’s logic.  
It must track trades, positions, and compute profit/loss per run.  
Add support for multithreaded execution of multiple strategies using `std::async`.”

#### `/src/Reporter/`
> “Copilot, write a `Reporter` class that outputs backtest results to JSON and CSV files.  
It should include metrics like total profit, max drawdown, win rate, and trade count.”

#### `/tests/`
> “Generate unit tests for each module using Catch2.  
Test CSV parsing, strategy signals, and profit calculation correctness.”

---

## 🧩 Advanced Features Prompts

### 🔁 Multithreading
> “Add support in BacktestEngine to execute multiple strategies in parallel threads.  
Use `std::async` or a thread pool.  
Ensure thread-safe access to shared data.”

### 📊 Visualization (optional)
> “Generate a simple Qt6 widget that plots price and trade signals using `QChartView`.  
Show buy/sell markers over the price line.”

### 🧠 Strategy Optimization (optional)
> “Add a StrategyOptimizer class that runs multiple parameter combinations and finds the most profitable configuration.”

### ⚙️ Configuration System
> “Allow users to specify data file paths and strategy parameters via a JSON config file.  
Parse using `nlohmann::json`.”

---

## 🧰 Tools Copilot Can Use

| Purpose | Tool / Library |
|----------|----------------|
| CSV Parsing | `fast-cpp-csv-parser` |
| JSON Config | `nlohmann/json` |
| Logging | `spdlog` |
| Testing | `Catch2` |
| Threading | `std::thread`, `std::async` |
| Visualization | `Qt6` |
| Build System | `CMake` |
| Version Control | `git` + GitHub Actions (CI) |

---

## 💬 Copilot Prompt Templates

### 🏗️ For Implementing New Components
> “Copilot, create a new class named `<ClassName>` in `<path>`.  
It should handle `<responsibility>`, follow OOP principles, and use smart pointers if ownership is required.”

### 🧪 For Writing Tests
> “Copilot, write unit tests for the `<ClassName>` class using Catch2.  
Mock dependencies where possible and test both valid and invalid inputs.”

### ⚡ For Performance Improvements
> “Optimize `<function>` to handle large datasets more efficiently.  
Suggest memory-efficient data structures and parallel execution options.”

### 📄 For Documentation
> “Generate Doxygen-style comments for all public methods in `<file>`.  
Add a top-level comment summarizing the class purpose and usage.”

---

## 📈 Development Milestones

| Phase | Copilot Goal | Example Task |
|--------|---------------|---------------|
| Phase 1 | Core Data Loading | CSV parser + tests |
| Phase 2 | Strategy Framework | MovingAverageStrategy |
| Phase 3 | Backtest Engine | Trade simulation logic |
| Phase 4 | Multithreading | Parallel backtests |
| Phase 5 | Reporting | JSON/CSV output |
| Phase 6 | Visualization | Qt dashboard (optional) |

---

## ✅ Final Reminder to Copilot
> Focus on **clean, modular C++ design**.  
> Avoid unnecessary complexity.  
> Prioritize readability and extensibility over micro-optimization.  
> Assume the user is learning — include clear code comments and examples.

---

**Authored by:** Mehdi ABBADI  
**Project:** FastQuantBacktester  
**Goal:** Build a professional-grade, portfolio-ready fintech simulation engine in modern C++.

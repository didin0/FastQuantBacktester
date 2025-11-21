# 📘 Product Requirements Document (PRD)# 📘 Product Requirements Document (PRD)



## **FastQuantBacktester – A Modular C++ Trading Simulation Engine**## **FastQuantBacktester -- A Modular C++ Trading Simulation Engine**



### 1. Context & Vision### 1. Context & Vision



FastQuantBacktester is a modular, high-performance C++ application designed to simulate and analyze trading strategies using historical financial data. The project bridges the gap between low-latency systems engineering and accessible financial analysis by providing a robust C++ backend coupled with a modern, interactive Web Dashboard.FastQuantBacktester is a modular, high-performance C++ application

designed to simulate and analyze trading strategies using historical

---financial data. The project's purpose is to bridge the gap between

software engineering and financial technology by focusing on

### 2. Product Objectivesarchitecture, performance, and data management --- without deep

mathematical complexity.

| Type | Objective |

| :--- | :--- |------------------------------------------------------------------------

| **Technical** | Modular architecture using modern C++20 (DataSource, Strategy, Engine, Server). |

| **Performance** | Efficiently process large datasets (CSV or API) with minimal latency. |### 2. Product Objectives

| **Functional** | Support multiple strategies (Moving Average, Breakout) and dynamic data sources. |

| **UX/UI** | **New:** Provide a user-friendly Web Interface for configuration, execution, and visualization. |  -----------------------------------------------------------------------

| **Scalability** | Support interchangeable data sources (Local CSV, Binance API). |  Type                    Objective

  ----------------------- -----------------------------------------------

---  **Technical**           Modular architecture using OOP principles

                          (DataSource, Strategy, Engine, Report).

### 3. Core Features

  **Performance**         Handle millions of data points efficiently.

| ID | Feature | Description | Priority | Status |

| :--- | :--- | :--- | :--- | :--- |  **Functional**          Run multiple trading strategies in parallel.

| F1 | **Data Loader** | Read historical data from CSV files or fetch live/historical data via Binance API. | High | ✅ Done |

| F2 | **Strategy Engine** | Plug-and-play strategy interface. Includes Moving Average and Breakout strategies. | High | ✅ Done |  **UX/UI**               Provide clear visualization via Qt or console

| F3 | **Backtest Engine** | Event-driven simulation of orders, fills, slippage, and commission. | High | ✅ Done |                          output.

| F4 | **Web Dashboard** | Interactive frontend to configure runs, view Equity Curves (Chart.js), and analyze Trade Logs. | High | ✅ Done |

| F5 | **Reporting** | JSON API responses containing statistics, equity arrays, and trade lists. | Medium | ✅ Done |  **Scalability**         Support interchangeable data sources (CSV, API,

| F6 | **Presets** | Quick-select dropdowns for example datasets and popular crypto symbols. | Low | ✅ Done |                          DB).

  -----------------------------------------------------------------------

---

------------------------------------------------------------------------

### 4. Technical Architecture

### 3. Core Features (MVP)

The system operates as a local web server application. The C++ backend handles the heavy lifting (data parsing, simulation), while the frontend handles presentation.

  ---------------------------------------------------------------------------

```mermaid  ID      Feature              Description               Priority

graph TD  ------- -------------------- ------------------------- --------------------

    Client[Web Browser] <-->|HTTP/JSON| Server[C++ Server (httplib)]  F1      **Data Loader**      Read and parse historical High

    Server -->|Load| DataLoader[Data Loader Module]                               price data (CSV or API).  

    DataLoader <-->|Read| CSV[CSV Files]  **Data Loader**      Read and parse historical price data (CSV or HTTP API). | High

    DataLoader <-->|Fetch| API[Binance API]  F2      **Strategy           Define and test multiple  High

    Server -->|Run| Engine[Backtest Engine]          Management**         trading strategies.       

    Engine -->|Execute| Strategy[Strategy Logic]

    Engine -->|Generate| Report[JSON Report]  F3      **Backtest Engine**  Simulate trading logic    High

```                               and calculate             

                               profit/loss.              

---

  F4      **Multithreading**   Run multiple backtests    Medium

### 5. Tech Stack                               concurrently.             



| Domain | Tools |  F5      **Reporting**        Generate results in       Medium

| :--- | :--- |                               JSON/CSV format.          

| **Language** | C++20 |

| **Build System** | CMake |  F6      **Visualization**    Display charts using Qt   Low

| **Web Server** | `cpp-httplib` |                               (optional).               

| **Frontend** | HTML5, Vanilla JS, Chart.js |  ---------------------------------------------------------------------------

| **Data Parsing** | `fast-cpp-csv-parser`, `nlohmann/json` |

| **API Client** | `libcurl` (Binance Integration) |------------------------------------------------------------------------

| **Logging** | `spdlog` |

| **Testing** | `Catch2` |### 4. Technical Architecture



---The system follows a modular design with independent components for data

loading, strategy execution, and result reporting.\

### 6. Success Metrics (KPIs)This allows for easy scalability and replacement of modules (e.g.,

switch from CSV to API data without altering the engine).

| Category | Indicator | Target |

| :--- | :--- | :--- |                    +-----------------------+

| **Usability** | Users can run a backtest in < 3 clicks. | ✅ |                    |     User Interface    |

| **Performance** | Backtest execution time for 1 year of 1m data < 500ms. | ✅ |                    +----------+------------+

| **Visualization** | Instant rendering of Equity Curve and Trade History. | ✅ |                               |

| **Reliability** | Graceful error handling for missing files or API failures. | ✅ |                               v

            +------------------+------------------+

---            |           Backtest Engine          |

            +------------------+------------------+

### 7. Development Roadmap                               |

               +---------------+---------------+

| Phase | Goals | Status |               |                               |

| :--- | :--- | :--- |    +--------------------+         +---------------------+

| **Phase 1** | Core C++ Engine, CSV Loading, Basic Strategies. | ✅ Completed |    |     Data Loader    |         |     Strategy API    |

| **Phase 2** | CLI Interface & JSON Reporting. | ✅ Completed |    | (CSV / API / DB)   |         | (MA, Breakout, etc) |

| **Phase 3** | **Web Server Integration & API Support.** | ✅ Completed |    +--------------------+         +---------------------+

| **Phase 4** | **Frontend Visualization (Charts, Tables, Presets).** | ✅ Completed |                               |

| **Phase 5** | Advanced Strategy Optimizer & Live Trading (Future). | ⏳ Pending |                               v

                     +--------------------+

---                     |      Reporter      |

                     | (JSON, Graph, CSV) |

### 8. Elevator Pitch                     +--------------------+



> FastQuantBacktester is the best of both worlds: the raw speed of C++ combined with the ease of use of a modern web app.------------------------------------------------------------------------

> It allows traders and developers to rapidly prototype strategies on real crypto data (Binance) or local files, visualizing performance instantly without writing a single line of plotting code.

### 5. Tech Stack

  Domain            Tools
  ----------------- --------------------------
  **Language**      C++20
  **Build**         CMake
  **CSV Parsing**   fast-cpp-csv-parser
  **JSON**          nlohmann/json
  **Logging**       spdlog
  **HTTP**          libcurl
  **Testing**       Catch2 / GoogleTest
  **UI**            Qt6
  **Threads**       std::thread / std::async
  **CI/CD**         GitHub Actions

------------------------------------------------------------------------

### 6. Success Metrics (KPIs)

  ------------------------------------------------------------------------
  Category                  Indicator                   Target
  ------------------------- --------------------------- ------------------
  **Performance**           Process 1M+ data rows in    ✅
                            under 2 seconds.            

  **Code Quality**          Full test coverage with     ✅
                            successful builds.          

  **Extensibility**         Add new strategies without  ✅
                            changing core engine.       

  **UX**                    Readable, well-structured   ✅
                            reports.                    

  **Documentation**         Clear README, diagrams, and ✅
                            usage guide.                
  ------------------------------------------------------------------------

------------------------------------------------------------------------

### 7. Development Roadmap

  ------------------------------------------------------------------------
  Phase                 Duration                     Goals
  --------------------- ---------------------------- ---------------------
  **Week 1--2**         Build CSV reader & core      MVP loading logic
                        architecture                 

  **Week 3--4**         Implement strategies &       Backtest prototype
                        engine                       

  **Week 5--6**         Add multithreading &         Performance layer
                        reporting                    

  **Week 7--8**         Build Qt UI & finalize       Showcase version
                        documentation                

  **Week 9+**           Add API and DB support       Real-world
                                                     scalability
  ------------------------------------------------------------------------

------------------------------------------------------------------------

### 8. Elevator Pitch

> FastQuantBacktester is a high-performance, modular backtesting engine
> built in C++.\
> It empowers developers to simulate financial trading strategies at
> scale, analyze results, and extend the system easily.\
> It showcases skills in performance optimization, OOP architecture, and
> modular software design --- making it a standout project for fintech
> or systems software engineering roles.

# 📘 Product Requirements Document (PRD)

## **FastQuantBacktester -- A Modular C++ Trading Simulation Engine**

### 1. Context & Vision

FastQuantBacktester is a modular, high-performance C++ application
designed to simulate and analyze trading strategies using historical
financial data. The project's purpose is to bridge the gap between
software engineering and financial technology by focusing on
architecture, performance, and data management --- without deep
mathematical complexity.

------------------------------------------------------------------------

### 2. Product Objectives

  -----------------------------------------------------------------------
  Type                    Objective
  ----------------------- -----------------------------------------------
  **Technical**           Modular architecture using OOP principles
                          (DataSource, Strategy, Engine, Report).

  **Performance**         Handle millions of data points efficiently.

  **Functional**          Run multiple trading strategies in parallel.

  **UX/UI**               Provide clear visualization via Qt or console
                          output.

  **Scalability**         Support interchangeable data sources (CSV, API,
                          DB).
  -----------------------------------------------------------------------

------------------------------------------------------------------------

### 3. Core Features (MVP)

  ---------------------------------------------------------------------------
  ID      Feature              Description               Priority
  ------- -------------------- ------------------------- --------------------
  F1      **CSV Data Loader**  Read and parse historical High
                               price data.               

  F2      **Strategy           Define and test multiple  High
          Management**         trading strategies.       

  F3      **Backtest Engine**  Simulate trading logic    High
                               and calculate             
                               profit/loss.              

  F4      **Multithreading**   Run multiple backtests    Medium
                               concurrently.             

  F5      **Reporting**        Generate results in       Medium
                               JSON/CSV format.          

  F6      **Visualization**    Display charts using Qt   Low
                               (optional).               
  ---------------------------------------------------------------------------

------------------------------------------------------------------------

### 4. Technical Architecture

The system follows a modular design with independent components for data
loading, strategy execution, and result reporting.\
This allows for easy scalability and replacement of modules (e.g.,
switch from CSV to API data without altering the engine).

                    +-----------------------+
                    |     User Interface    |
                    +----------+------------+
                               |
                               v
            +------------------+------------------+
            |           Backtest Engine          |
            +------------------+------------------+
                               |
               +---------------+---------------+
               |                               |
    +--------------------+         +---------------------+
    |     Data Loader    |         |     Strategy API    |
    | (CSV / API / DB)   |         | (MA, Breakout, etc) |
    +--------------------+         +---------------------+
                               |
                               v
                     +--------------------+
                     |      Reporter      |
                     | (JSON, Graph, CSV) |
                     +--------------------+

------------------------------------------------------------------------

### 5. Tech Stack

  Domain            Tools
  ----------------- --------------------------
  **Language**      C++20
  **Build**         CMake
  **CSV Parsing**   fast-cpp-csv-parser
  **JSON**          nlohmann/json
  **Logging**       spdlog
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

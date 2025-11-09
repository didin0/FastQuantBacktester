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

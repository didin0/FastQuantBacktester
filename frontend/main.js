import { loadData, runBacktest } from "./api.js";

const sourceTypeSelect = document.getElementById("sourceType");
const csvInputs = document.getElementById("csvInputs");
const apiInputs = document.getElementById("apiInputs");

const csvPathInput = document.getElementById("csvPath");
const apiSymbolInput = document.getElementById("apiSymbol");
const apiIntervalSelect = document.getElementById("apiInterval");

const strategyTypeSelect = document.getElementById("strategyType");
const paramsMa = document.getElementById("params-ma");
const paramsBreakout = document.getElementById("params-breakout");
const smaInput = document.getElementById("smaPeriod");
const breakoutLookbackInput = document.getElementById("breakoutLookback");
const breakoutThresholdInput = document.getElementById("breakoutThreshold");

const dataInfoEl = document.getElementById("dataInfo");
const resultInfoEl = document.getElementById("resultInfo");
const loadBtn = document.getElementById("loadBtn");
const runBtn = document.getElementById("runBtn");

const exampleSelect = document.getElementById("exampleSelect");
const presetSymbolSelect = document.getElementById("presetSymbol");
const equityChartEl = document.getElementById("equityChart");
const tradesSection = document.getElementById("tradesSection");
const tradesTableBody = document.querySelector("#tradesTable tbody");

let lastDatasetSummary = null;
let chartInstance = null;

// --- Initialization & Event Listeners ---

function rememberLabels() {
  [loadBtn, runBtn].forEach((btn) => {
    btn.dataset.label = btn.textContent;
  });
}

rememberLabels();
loadBtn.addEventListener("click", handleLoad);
runBtn.addEventListener("click", handleRun);

sourceTypeSelect.addEventListener("change", () => {
  const isApi = sourceTypeSelect.value === "api";
  csvInputs.style.display = isApi ? "none" : "block";
  apiInputs.style.display = isApi ? "block" : "none";
});

strategyTypeSelect.addEventListener("change", () => {
  const type = strategyTypeSelect.value;
  paramsMa.style.display = type === "MA" ? "block" : "none";
  paramsBreakout.style.display = type === "Breakout" ? "block" : "none";
});

// Presets
exampleSelect.addEventListener("change", () => {
  if (exampleSelect.value) {
    csvPathInput.value = exampleSelect.value;
  }
});

presetSymbolSelect.addEventListener("change", () => {
  if (presetSymbolSelect.value) {
    apiSymbolInput.value = presetSymbolSelect.value;
  }
});

// Load examples on startup
async function loadExamples() {
  try {
    const res = await fetch("http://localhost:8080/list-examples");
    if (res.ok) {
      const files = await res.json();
      files.forEach(file => {
        const opt = document.createElement("option");
        opt.value = file;
        opt.textContent = file.split('/').pop(); // Show filename only
        exampleSelect.appendChild(opt);
      });
    }
  } catch (e) {
    console.warn("Could not load examples", e);
  }
}
loadExamples();

showMessage(dataInfoEl, "No data loaded yet.");
showMessage(resultInfoEl, "No backtest run yet.");


// --- Core Logic ---

async function handleLoad() {
  const source = sourceTypeSelect.value;
  let payload = { source };

  if (source === "csv") {
    const path = csvPathInput.value.trim();
    if (!path) {
      showMessage(dataInfoEl, "Please enter a CSV path.", "error");
      return;
    }
    payload.path = path;
  } else {
    const symbol = apiSymbolInput.value.trim();
    if (!symbol) {
      showMessage(dataInfoEl, "Please enter a symbol.", "error");
      return;
    }
    payload.symbol = symbol;
    payload.interval = apiIntervalSelect.value;
  }

  setLoading(loadBtn, true);
  try {
    const summary = await loadData(payload);
    lastDatasetSummary = summary;
    showMessage(dataInfoEl, formatDataset(summary));
    showMessage(resultInfoEl, "Dataset ready. Run the backtest next.");
  } catch (err) {
    showMessage(dataInfoEl, `Load failed: ${err.message}`, "error");
  } finally {
    setLoading(loadBtn, false);
  }
}

async function handleRun() {
  if (!lastDatasetSummary) {
    showMessage(resultInfoEl, "Load a dataset first.", "error");
    return;
  }

  const strategyType = strategyTypeSelect.value;
  let payload = { strategyType };

  if (strategyType === "MA") {
    const period = Number.parseInt(smaInput.value, 10);
    if (!Number.isFinite(period) || period <= 0) {
      showMessage(resultInfoEl, "SMA period must be positive.", "error");
      return;
    }
    payload.period = period;
  } else {
    const lookback = Number.parseInt(breakoutLookbackInput.value, 10);
    const threshold = Number.parseFloat(breakoutThresholdInput.value);
    if (!Number.isFinite(lookback) || lookback <= 0) {
      showMessage(resultInfoEl, "Lookback must be positive.", "error");
      return;
    }
    payload.lookback = lookback;
    payload.threshold = threshold;
  }

  setLoading(runBtn, true);
  try {
    const result = await runBacktest(payload);
    showMessage(resultInfoEl, formatBacktest(result));
    renderChart(result.equityCurve);
    renderTrades(result.recentTrades);
  } catch (err) {
    showMessage(resultInfoEl, `Backtest failed: ${err.message}`, "error");
  } finally {
    setLoading(runBtn, false);
  }
}

// --- UI Helpers ---

function showMessage(el, message, type = "info") {
  el.textContent = message;
  el.dataset.type = type;
}

function setLoading(btn, loading) {
  btn.disabled = loading;
  btn.textContent = loading ? "Please wait…" : btn.dataset.label;
}

function formatDataset(summary) {
  if (!summary) return "No data loaded.";
  const {
    symbol = "?",
    rows = 0,
    startDate = "-",
    endDate = "-",
    timeframe = "?"
  } = summary;
  return `${symbol} • ${rows} rows • ${timeframe}\n${startDate} → ${endDate}`;
}

function formatBacktest(result) {
  if (!result) return "No backtest run yet.";
  const {
    strategy = "-",
    trades = 0,
    winRate = 0,
    totalProfit = 0,
    maxDrawdown = 0
  } = result;
  const pct = (value) => `${(value * 100).toFixed(2)}%`;
  const profitStr = typeof totalProfit === "number" ? totalProfit.toFixed(2) : totalProfit;
  return `${strategy}\nTrades: ${trades} • Win rate: ${pct(winRate)}\nProfit: ${profitStr} • Max DD: ${pct(maxDrawdown)}`;
}

function renderChart(equityCurve) {
  if (!equityCurve || equityCurve.length === 0) {
    equityChartEl.style.display = "none";
    return;
  }

  equityChartEl.style.display = "block";
  const ctx = equityChartEl.getContext("2d");

  if (chartInstance) {
    chartInstance.destroy();
  }

  const labels = equityCurve.map((_, i) => i); // Simple index for x-axis
  const data = equityCurve.map(pt => pt.value);

  chartInstance = new Chart(ctx, {
    type: 'line',
    data: {
      labels: labels,
      datasets: [{
        label: 'Equity Curve',
        data: data,
        borderColor: '#4CAF50',
        backgroundColor: 'rgba(76, 175, 80, 0.1)',
        borderWidth: 2,
        pointRadius: 0,
        fill: true,
        tension: 0.1
      }]
    },
    options: {
      responsive: true,
      interaction: {
        mode: 'index',
        intersect: false,
      },
      plugins: {
        legend: {
          display: false
        },
        tooltip: {
          enabled: true
        }
      },
      scales: {
        x: {
          display: false // Hide x-axis labels for cleaner look
        },
        y: {
          grid: {
            color: '#333'
          },
          ticks: {
            color: '#aaa'
          }
        }
      }
    }
  });
}

function renderTrades(trades) {
  if (!trades || trades.length === 0) {
    tradesSection.style.display = "none";
    return;
  }
  tradesSection.style.display = "block";
  tradesTableBody.innerHTML = "";
  
  trades.forEach(t => {
    const row = document.createElement("tr");
    const dateStr = new Date(t.date * 1000).toLocaleString();
    const typeColor = t.type === "BUY" ? "#4CAF50" : "#F44336";
    
    row.innerHTML = `
      <td style="padding: 8px; color: ${typeColor}; font-weight: bold;">${t.type}</td>
      <td style="padding: 8px;">${t.price.toFixed(2)}</td>
      <td style="padding: 8px;">${t.qty.toFixed(4)}</td>
      <td style="padding: 8px; color: #888;">${dateStr}</td>
    `;
    tradesTableBody.appendChild(row);
  });
}

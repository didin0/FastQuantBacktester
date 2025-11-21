const API_BASE = "http://localhost:8080";

async function postJson(path, payload) {
  const res = await fetch(`${API_BASE}${path}`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload ?? {})
  });
  if (!res.ok) {
    throw new Error(`Request failed (${res.status})`);
  }
  return res.json();
}

export async function loadData(payload) {
  return postJson("/load-data", payload);
}

export async function runBacktest(params) {
  return postJson("/run-backtest", params);
}

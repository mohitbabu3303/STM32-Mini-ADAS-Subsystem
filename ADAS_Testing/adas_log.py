"""Parsing helpers shared by plot.py and dashboard.py.

The firmware (UART @ 115200) and the PC simulation print one line per sample:
    DATA,ms,lane,dist_m,speed_kmh,thr_m,lane_w,obst_w,ovspd_w,risk
Everything else (ALERT:, HEALTH:, CAN RX ...) is kept as event text.
"""
from __future__ import annotations

import io
from typing import Iterable

import pandas as pd

RISK_LEVELS = ["SAFE", "LOW", "MEDIUM", "HIGH", "CRITICAL"]
RISK_COLORS = {
    "SAFE": "#2e7d32",
    "LOW": "#9e9d24",
    "MEDIUM": "#f9a825",
    "HIGH": "#ef6c00",
    "CRITICAL": "#c62828",
}
DATA_COLUMNS = ["ms", "lane", "dist_m", "speed_kmh", "thr_m",
                "lane_w", "obst_w", "ovspd_w", "risk"]
NO_OBSTACLE_M = 255


def parse_uart_lines(lines: Iterable[str]) -> tuple[pd.DataFrame, list[str]]:
    """Split a UART/simulation log into a DATA table and a list of events."""
    rows, events = [], []
    for raw in lines:
        line = raw.strip()
        if not line:
            continue
        if line.startswith("DATA,"):
            parts = line.split(",")[1:]
            if len(parts) != len(DATA_COLUMNS) or parts[-1] not in RISK_LEVELS:
                continue  # partial line (e.g. capture started mid-line)
            try:
                rows.append([int(p) for p in parts[:-1]] + [parts[-1]])
            except ValueError:
                continue
        elif line.startswith(("ALERT:", "HEALTH:", "CAN RX", "FATAL", "===", "Build", "CAN mode")):
            events.append(line)

    df = pd.DataFrame(rows, columns=DATA_COLUMNS)
    if not df.empty:
        df["t_s"] = (df["ms"] - df["ms"].iloc[0]) / 1000.0
        df["risk_level"] = df["risk"].map(RISK_LEVELS.index)
        # 255 = "no obstacle": leave a gap in distance plots instead of a spike
        df["dist_plot"] = df["dist_m"].where(df["dist_m"] != NO_OBSTACLE_M)
    return df, events


def parse_uart_text(text: str) -> tuple[pd.DataFrame, list[str]]:
    return parse_uart_lines(io.StringIO(text))


def load_test_results(path: str) -> pd.DataFrame:
    """Load log.csv written by the SIL test runner (test.c)."""
    df = pd.read_csv(path)
    df["pass"] = df["result"].eq("PASS")
    return df

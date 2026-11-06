"""Static plots for the Mini ADAS SIL results.

Usage (from ADAS_Testing/):
    python plot.py                    # test results from log.csv
    python plot.py sim_log.txt        # + time plots of a simulation/UART log
Figures are saved to screenshots/ and shown in a window (if a display exists).
"""
from __future__ import annotations

import os
import sys

import matplotlib
if not os.environ.get("DISPLAY") and sys.platform.startswith("linux"):
    matplotlib.use("Agg")  # headless (CI / SSH)
import matplotlib.pyplot as plt

from adas_log import RISK_COLORS, RISK_LEVELS, load_test_results, parse_uart_text

OUT_DIR = "screenshots"


def plot_test_results(path: str = "log.csv") -> None:
    df = load_test_results(path)
    colors = [RISK_COLORS[r] for r in df["actual_risk"]]

    fig, axes = plt.subplots(3, 1, figsize=(10, 9), sharex=True)
    fig.suptitle(f"SIL test results - {df['pass'].sum()}/{len(df)} PASS")

    ax = axes[0]
    ax.bar(df["id"], df["actual_risk"].map(RISK_LEVELS.index), color=colors)
    ax.set_yticks(range(len(RISK_LEVELS)), RISK_LEVELS)
    ax.set_ylabel("Risk level")
    ax.grid(axis="y", alpha=0.3)

    ax = axes[1]
    dist = df["dist"].where(df["dist"] != 255)
    ax.plot(df["id"], dist, "o-", label="Obstacle distance (m)")
    ax.step(df["id"], df["threshold"], where="mid", label="Speed-based threshold (m)")
    ax.set_ylabel("Metres")
    ax.set_ylim(0, 130)  # far targets (>130 m) are irrelevant for warnings
    ax.legend(loc="upper right")
    ax.grid(alpha=0.3)

    ax = axes[2]
    ax.bar(df["id"], df["time_ns"], color=["#2e7d32" if p else "#c62828" for p in df["pass"]])
    ax.set_ylabel("Host time (ns)\ngreen=PASS red=FAIL")
    ax.set_xlabel("Test case ID")
    ax.grid(axis="y", alpha=0.3)

    fig.tight_layout()
    os.makedirs(OUT_DIR, exist_ok=True)
    fig.savefig(os.path.join(OUT_DIR, "test_results.png"), dpi=120)


def plot_timeline(path: str) -> None:
    with open(path, encoding="utf-8", errors="replace") as f:
        df, _ = parse_uart_text(f.read())
    if df.empty:
        print(f"No DATA lines found in {path}")
        return

    fig, axes = plt.subplots(3, 1, figsize=(11, 8), sharex=True)
    fig.suptitle(f"ADAS timeline - {path}")

    axes[0].plot(df["t_s"], df["speed_kmh"], color="#1565c0")
    axes[0].axhline(120, ls="--", color="grey", lw=1, label="Overspeed limit")
    axes[0].axhline(30, ls=":", color="grey", lw=1, label="ADAS active from")
    axes[0].set_ylabel("Speed (km/h)")
    axes[0].legend(loc="upper left", fontsize=8)

    axes[1].plot(df["t_s"], df["dist_plot"], "o-", ms=3, label="Filtered distance")
    axes[1].step(df["t_s"], df["thr_m"], where="post", label="Warning threshold")
    axes[1].set_ylabel("Metres")
    axes[1].legend(loc="upper right", fontsize=8)

    axes[2].scatter(df["t_s"], df["risk_level"], c=[RISK_COLORS[r] for r in df["risk"]], s=18)
    axes[2].set_yticks(range(len(RISK_LEVELS)), RISK_LEVELS)
    axes[2].set_xlabel("Time (s)")
    for ax in axes:
        ax.grid(alpha=0.3)

    fig.tight_layout()
    os.makedirs(OUT_DIR, exist_ok=True)
    fig.savefig(os.path.join(OUT_DIR, "timeline.png"), dpi=120)


if __name__ == "__main__":
    plot_test_results()
    if len(sys.argv) > 1:
        plot_timeline(sys.argv[1])
    print(f"Figures saved to {OUT_DIR}/")
    if matplotlib.get_backend().lower() != "agg":
        plt.show()

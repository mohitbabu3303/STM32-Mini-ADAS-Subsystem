"""Interactive dashboard for the Mini ADAS firmware and SIL tests.

    pip install streamlit pandas matplotlib
    streamlit run dashboard.py

Data sources:
  * UART capture from the STM32 (e.g. saved from PuTTY/Tera Term/CubeIDE
    console at 115200 baud) or the PC simulation output (./adas_sim > sim_log.txt)
  * SIL test results (log.csv written by ./adas_test)
"""
from __future__ import annotations

import os

import pandas as pd
import streamlit as st

from adas_log import RISK_COLORS, RISK_LEVELS, load_test_results, parse_uart_text

st.set_page_config(page_title="Mini ADAS Dashboard", layout="wide")
st.title("STM32 Mini ADAS - Dashboard")

tab_run, tab_sil = st.tabs(["Runtime log (UART / simulation)", "SIL test results"])

# ---------------------------------------------------------------------------
with tab_run:
    uploaded = st.file_uploader("UART capture or simulation output (.txt/.log)",
                                type=["txt", "log", "csv"])
    text = None
    if uploaded is not None:
        text = uploaded.getvalue().decode("utf-8", errors="replace")
    elif os.path.exists("sim_log.txt"):
        st.caption("Showing sim_log.txt (generate with: ./adas_sim > sim_log.txt)")
        with open("sim_log.txt", encoding="utf-8", errors="replace") as f:
            text = f.read()

    if text is None:
        st.info("Upload a log, or create sim_log.txt with the PC simulation.")
    else:
        df, events = parse_uart_text(text)
        if df.empty:
            st.warning("No DATA lines found. Expected: DATA,ms,lane,dist_m,speed_kmh,...")
        else:
            worst = RISK_LEVELS[int(df["risk_level"].max())]
            c1, c2, c3, c4 = st.columns(4)
            c1.metric("Samples", len(df))
            c2.metric("Duration", f"{df['t_s'].iloc[-1]:.1f} s")
            c3.metric("Worst risk", worst)
            c4.metric("Samples with warning",
                      int((df[["lane_w", "obst_w", "ovspd_w"]].sum(axis=1) > 0).sum()))

            st.subheader("Speed (km/h)")
            st.line_chart(df.set_index("t_s")[["speed_kmh"]])

            st.subheader("Obstacle distance vs. warning threshold (m)")
            st.line_chart(df.set_index("t_s")[["dist_plot", "thr_m"]]
                          .rename(columns={"dist_plot": "distance", "thr_m": "threshold"}))

            st.subheader("Risk level (0 = SAFE ... 4 = CRITICAL)")
            st.bar_chart(df.set_index("t_s")[["risk_level"]])

            counts = df["risk"].value_counts().reindex(RISK_LEVELS, fill_value=0)
            st.subheader("Time spent per risk level")
            st.dataframe(pd.DataFrame({"samples": counts,
                                       "share %": (100 * counts / len(df)).round(1)}))

        if events:
            st.subheader("Events (ALERT / HEALTH / CAN)")
            st.code("\n".join(events[-200:]), language="text")

# ---------------------------------------------------------------------------
with tab_sil:
    if not os.path.exists("log.csv"):
        st.info("Run the SIL tests first: make test  (creates log.csv)")
    else:
        res = load_test_results("log.csv")
        passed = int(res["pass"].sum())
        c1, c2, c3 = st.columns(3)
        c1.metric("Test cases", len(res))
        c2.metric("Passed", passed)
        c3.metric("Failed", len(res) - passed)

        def colour_result(val: str) -> str:
            return "color: #2e7d32" if val == "PASS" else "color: #c62828; font-weight: bold"

        def colour_risk(val: str) -> str:
            return f"color: {RISK_COLORS.get(val, 'black')}"

        st.dataframe(res.drop(columns=["pass"]).style
                     .map(colour_result, subset=["result"])
                     .map(colour_risk, subset=["expected_risk", "actual_risk"]),
                     use_container_width=True)
        st.subheader("Host execution time per test (ns)")
        st.bar_chart(res.set_index("id")[["time_ns"]])

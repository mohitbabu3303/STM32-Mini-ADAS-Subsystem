# ADAS_Testing – Software-in-the-Loop tests

These tools compile the firmware's own `Mini_ADAS_RTOS/Core/Src/adas_logic.c`
and `adas_can.c` on a PC. A passing test therefore means the code that runs on
the STM32 behaves as specified.

| File | Purpose |
|---|---|
| `test.c` | Test runner: 25 CSV scenarios + filter and CAN codec unit tests, exit code 0 = all pass |
| `test_cases.csv` | Inputs, expected warning flag and expected risk level |
| `log.csv` | Results written by `adas_test` (incl. host timing in ns) |
| `sim_log.txt` | Example output of the PC simulation (same format as the STM32 UART) |
| `adas_log.py` | Parser shared by the plotting and dashboard scripts |
| `plot.py` | Saves `screenshots/test_results.png` and `screenshots/timeline.png` |
| `dashboard.py` | Streamlit dashboard for UART captures, simulation output and test results |
| `Makefile` | Builds `adas_test` and `adas_sim` |

```
make test                          # build + run tests (Windows: mingw32-make test)
make adas_sim && ./adas_sim > sim_log.txt
pip install pandas matplotlib streamlit
python plot.py sim_log.txt
streamlit run dashboard.py
```

To analyse the real board: save the UART output (115200 baud) to a text file
and upload it in the dashboard; it parses the same `DATA,...` lines.

Timing values are measured on the host PC and only show the relative cost
of the logic, not the execution time on the Cortex-M3.

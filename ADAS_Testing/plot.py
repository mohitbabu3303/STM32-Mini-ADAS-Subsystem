import pandas as pd
import matplotlib.pyplot as plt

data = pd.read_csv("log.csv")

data["result_num"] = data["result"].apply(lambda x: 1 if x == "PASS" else 0)

# ---------- Distance ----------
plt.figure()
plt.plot(data["id"], data["dist"], marker='o')
plt.title("Distance vs Test Case")
plt.xlabel("Test Case ID")
plt.ylabel("Distance")
plt.grid()

# ---------- Execution Time ----------
plt.figure()
plt.plot(data["id"], data["time"] * 1e6, marker='o')
plt.title("Execution Time per Test")
plt.xlabel("Test Case ID")
plt.ylabel("Time (microseconds)")
plt.grid()

# ---------- PASS/FAIL ----------
plt.figure()
plt.scatter(data["id"], data["result_num"])
plt.title("Test Results")
plt.xlabel("Test Case ID")
plt.ylabel("PASS=1, FAIL=0")
plt.yticks([0, 1], ["FAIL", "PASS"])
plt.grid()

plt.show()
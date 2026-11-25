
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path
from io import StringIO

# ==========================================
# FILE SETTINGS
# ==========================================

FILE = Path(
    r"C:\Users\Gautam Ganesh\Downloads\motor_stopping_distance\ticks_vs_time.txt"
)

# ==========================================
# READ ARDUINO CSV DATA
# ==========================================

with open(FILE, "r", encoding="utf-8-sig") as f:
    lines = f.readlines()

csv_lines = []
recording = False

for line in lines:
    line = line.strip()

    if line.startswith("Time_us,Encoder_Ticks,"):
        csv_lines = [line]
        recording = True
        continue

    if line == "END_OF_DATA":
        break

    if recording and len(line.split(",")) == 4:
        try:
            float(line.split(",")[0])
            float(line.split(",")[1])
            float(line.split(",")[2])
            csv_lines.append(line)
        except ValueError:
            pass

if len(csv_lines) < 2:
    raise ValueError("No valid CSV data found.")

df = pd.read_csv(StringIO("\n".join(csv_lines)))

# ==========================================
# SEPARATE SAMPLES AND EVENTS
# ==========================================

samples = df[df["Event"] == "SAMPLE"].copy()
samples = samples.sort_values("Time_us").reset_index(drop=True)

events = df[df["Event"].isin(["PWM_ON", "PWM_OFF"])]

if samples.empty:
    raise ValueError("No encoder samples found.")

on_event = events[events["Event"] == "PWM_ON"]
off_event = events[events["Event"] == "PWM_OFF"]

if on_event.empty or off_event.empty:
    raise ValueError("PWM_ON or PWM_OFF event missing.")

# Convert microseconds to milliseconds
samples["Time_ms"] = samples["Time_us"] / 1000.0

time = samples["Time_ms"]
ticks = samples["Encoder_Ticks"]

pwm_on_time = on_event.iloc[0]["Time_us"] / 1000.0
pwm_off_time = off_event.iloc[0]["Time_us"] / 1000.0

pwm_on_ticks = int(on_event.iloc[0]["Encoder_Ticks"])
pwm_off_ticks = int(off_event.iloc[0]["Encoder_Ticks"])

# ==========================================
# FIND FIRST ENCODER MOVEMENT
# ==========================================

after_on = samples[
    samples["Time_ms"] >= pwm_on_time
]

movement = after_on[
    after_on["Encoder_Ticks"] != pwm_on_ticks
]

first_movement_time = None
first_movement_ticks = None

if not movement.empty:
    first_movement_time = float(
        movement.iloc[0]["Time_ms"]
    )

    first_movement_ticks = int(
        movement.iloc[0]["Encoder_Ticks"]
    )

# ==========================================
# FIND FINAL ENCODER MOVEMENT
# ==========================================

# Find every sample where ticks changed
tick_changes = samples["Encoder_Ticks"].diff()

change_indices = samples.index[
    tick_changes.notna() & (tick_changes != 0)
]

final_stable_time = None
final_stable_ticks = None

if len(change_indices) > 0:

    last_change_idx = int(change_indices[-1])

    # First unchanged sample after final tick change
    if last_change_idx + 1 < len(samples):

        final_stable_time = float(
            samples.loc[last_change_idx + 1, "Time_ms"]
        )

        final_stable_ticks = int(
            samples.loc[last_change_idx + 1, "Encoder_Ticks"]
        )

# ==========================================
# CALCULATE RESULTS
# ==========================================

final_ticks = int(ticks.iloc[-1])

pulse_duration = pwm_off_time - pwm_on_time
post_stop_ticks = final_ticks - pwm_off_ticks

print("\n========== MOTOR RESULTS ==========")

print(f"PWM ON time:          {pwm_on_time:.3f} ms")
print(f"PWM OFF time:         {pwm_off_time:.3f} ms")
print(f"Pulse duration:       {pulse_duration:.3f} ms")

if first_movement_time is not None:

    response_delay = first_movement_time - pwm_on_time

    print(
        f"First movement:       {first_movement_time:.3f} ms"
    )

    print(
        f"Response delay:       {response_delay:.3f} ms"
    )

else:
    print("First movement:       Not detected")

if final_stable_time is not None:

    print(
        f"Movement stops:       {final_stable_time:.3f} ms"
    )

    print(
        f"Final stable ticks:   {final_stable_ticks}"
    )

else:
    print("Movement stops:       Not confirmed")

print(f"Ticks at PWM ON:      {pwm_on_ticks}")
print(f"Ticks at PWM OFF:     {pwm_off_ticks}")
print(f"Final ticks:         {final_ticks}")
print(f"Post-OFF movement:    {post_stop_ticks:+d} ticks")

print("===================================\n")

# ==========================================
# PLOT: ENCODER TICKS VS TIME
# ==========================================

fig, ax = plt.subplots(figsize=(12, 6))

# Actual encoder position
ax.plot(
    time,
    ticks,
    color="blue",
    linewidth=2,
    label="Encoder ticks"
)

# Highlight when PWM is applied
ax.axvspan(
    pwm_on_time,
    pwm_off_time,
    color="green",
    alpha=0.10,
    label="50% PWM applied"
)

# PWM ON marker
ax.axvline(
    pwm_on_time,
    color="green",
    linestyle="--",
    linewidth=1.8,
    label=f"PWM ON: {pwm_on_time:.3f} ms"
)

# PWM OFF marker
ax.axvline(
    pwm_off_time,
    color="red",
    linestyle="--",
    linewidth=1.8,
    label=f"PWM OFF: {pwm_off_time:.3f} ms"
)

# First movement marker
if first_movement_time is not None:

    ax.scatter(
        first_movement_time,
        first_movement_ticks,
        color="orange",
        s=80,
        zorder=6,
        label=f"First movement: {first_movement_time:.3f} ms"
    )

# Final movement marker
if final_stable_time is not None:

    ax.scatter(
        final_stable_time,
        final_stable_ticks,
        color="purple",
        s=90,
        zorder=7,
        label=f"Movement stops: {final_stable_time:.3f} ms"
    )

    ax.annotate(
        "Movement stops",
        xy=(final_stable_time, final_stable_ticks),
        xytext=(-95, -45),
        textcoords="offset points",
        arrowprops=dict(
            arrowstyle="->",
            color="purple"
        ),
        fontsize=10,
        color="purple"
    )

# ==========================================
# RESULTS BOX
# ==========================================

results = (
    f"PWM ON: {pwm_on_time:.3f} ms\n"
    f"PWM OFF: {pwm_off_time:.3f} ms\n"
    f"Pulse: {pulse_duration:.3f} ms\n"
    f"Final position: {final_ticks} ticks\n"
    f"Post-OFF movement: {post_stop_ticks:+d} ticks"
)

if first_movement_time is not None:
    results += (
        f"\nResponse delay: "
        f"{first_movement_time - pwm_on_time:.3f} ms"
    )

if final_stable_time is not None:
    results += (
        f"\nMovement stops: {final_stable_time:.3f} ms"
    )

ax.text(
    0.02,
    0.97,
    results,
    transform=ax.transAxes,
    fontsize=10,
    verticalalignment="top",
    bbox=dict(
        boxstyle="round",
        facecolor="white",
        edgecolor="gray",
        alpha=0.9
    )
)

# ==========================================
# GRAPH SETTINGS
# ==========================================

ax.set_xlabel("Time (ms)", fontsize=12)
ax.set_ylabel("Encoder ticks", fontsize=12)

ax.set_title(
    "DC Motor Step Response - 50% PWM for 250 ms",
    fontsize=14
)

ax.set_xlim(0, time.max())

ax.grid(True, alpha=0.3)

# Put legend outside so it doesn't cover the curve
ax.legend(
    loc="upper left",
    bbox_to_anchor=(1.02, 1)
)

plt.tight_layout()

# Save high-resolution graph
plt.savefig(
    "motor_step_response.png",
    dpi=300,
    bbox_inches="tight"
)

plt.show()

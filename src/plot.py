import pandas as pd
import matplotlib.pyplot as plt

# Reading the CSV data
data = pd.read_csv("steeringAngles.csv")

# Convert timestamps from microseconds to seconds for easier readability
data["Timestamp"] = (data["Timestamp"] - data["Timestamp"].min()) / 1e6

# Plotting
plt.figure(figsize=(10, 5))
plt.plot(data["Timestamp"], data[" SteeringAngle"], label="Steering Angle", marker="o")
plt.plot(
    data["Timestamp"], data[" OriginalSteering"], label="Original Steering", marker="x"
)

# Adding titles and labels
plt.title("Steering Angle Comparison Over Time")
plt.xlabel("Time (seconds from start)")
plt.ylabel("Steering Angle")
plt.legend()

# Display the plot
plt.grid(True)
plt.show()

import pandas as pd
import matplotlib.pyplot as plt

# Load the CSV file
df = pd.read_csv("steeringAngles.csv")

# Ensure there's no leading/trailing whitespace in column names
df.columns = df.columns.str.strip()

# Convert 'Timestamp' from Unix time (microseconds since epoch) to datetime
df["Timestamp"] = pd.to_datetime(df["Timestamp"], unit="us")

# Calculate the bounds for being within 25% of the original steering
df["LowerBound"] = df["OriginalSteering"] * 0.75
df["UpperBound"] = df["OriginalSteering"] * 1.25

# Create a mask for when the SteeringAngle is within ±25% of OriginalSteering
within_range_mask = (df["SteeringAngle"] >= df["LowerBound"]) & (
    df["SteeringAngle"] <= df["UpperBound"]
)

# Plotting only the green line for within range data
plt.figure(figsize=(12, 6))  # Set the figure size for better visibility
plt.scatter(
    df.loc[within_range_mask, "Timestamp"],
    df.loc[within_range_mask, "SteeringAngle"],
    label="Within 25% Range",
    color="green",
)
plt.title("Steering Angles Within 25% of Original Steering Over Time")
plt.xlabel("Timestamp")
plt.ylabel("Steering Angle (degrees)")
plt.legend()

# Rotate date labels for better readability
plt.gcf().autofmt_xdate()  # Auto-formats the x-axis labels for better visibility

# Show the plot
plt.show()

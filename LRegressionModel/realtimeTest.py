import pandas as pd
from sklearn.preprocessing import StandardScaler
from sklearn.linear_model import LinearRegression
from sklearn.model_selection import train_test_split
from sklearn.metrics import mean_squared_error
from sklearn.impute import SimpleImputer
import joblib

# Load trained model and preprocessing tools
model = joblib.load("modelmultiple.pkl")
scaler = joblib.load("scaler.pkl")
imputer = joblib.load("imputer.pkl")


def load_data(filepath, columns):
    df = pd.read_csv(filepath, delimiter=";", usecols=columns)
    df["timestamp"] = pd.to_datetime(
        df["sampleTimeStamp.seconds"] * 1e6 + df["sampleTimeStamp.microseconds"],
        unit="us",
    )
    df.drop(
        ["sampleTimeStamp.seconds", "sampleTimeStamp.microseconds"],
        axis=1,
        inplace=True,
    )
    df.set_index("timestamp", inplace=True)

    print(df.head())
    return df


def preprocess_and_predict(features):
    features_scaled = scaler.transform(features)
    features_imputed = imputer.transform(features_scaled)
    predictions = model.predict(features_imputed)
    return predictions


# Load data
angular_velocity_data = load_data(
    "CSV-Files/CID-140-recording-2020-03-18_150001-selection.rec.csv/opendlv.proxy.AngularVelocityReading-0.csv",
    [
        "sampleTimeStamp.seconds",
        "sampleTimeStamp.microseconds",
        "angularVelocityX",
        "angularVelocityY",
        "angularVelocityZ",
    ],
)
ground_steering_data = load_data(
    "CSV-Files/CID-140-recording-2020-03-18_150001-selection.rec.csv/opendlv.proxy.GroundSteeringRequest-0.csv",
    ["sampleTimeStamp.seconds", "sampleTimeStamp.microseconds", "groundSteering"],
)

# Merge data
merged_data = pd.merge_asof(
    angular_velocity_data.sort_index(),
    ground_steering_data.sort_index(),
    left_index=True,
    right_index=True,
    direction="nearest",
    tolerance=pd.Timedelta("1s"),
)

# Predict and assign predictions
features = merged_data[["angularVelocityX", "angularVelocityY", "angularVelocityZ"]]
merged_data["predicted_steering"] = preprocess_and_predict(features)


import matplotlib.pyplot as plt

# Plotting results
# plt.figure(figsize=(10, 5))
# plt.plot(merged_data.index, merged_data["groundSteering"], label="Actual Steering")
# plt.plot(
#     merged_data.index,
#     merged_data["predicted_steering"],
#     label="Predicted Steering",
#     linestyle="--",
# )
# plt.legend()
# plt.title("Comparison of Actual and Predicted Steering Angles")
# plt.xlabel("Timestamp")
# plt.ylabel("Steering Angle")
# plt.show()


merged_data["upper_bound"] = merged_data["groundSteering"] * 1.25
merged_data["lower_bound"] = merged_data["groundSteering"] * 0.75

# Plotting results with shaded area for ±25% range
plt.figure(figsize=(12, 6))
plt.plot(
    merged_data.index,
    merged_data["groundSteering"],
    label="Actual Steering",
    color="blue",
)
plt.plot(
    merged_data.index,
    merged_data["predicted_steering"],
    label="Predicted Steering",
    linestyle="--",
    color="red",
)

# Add shaded area
plt.fill_between(
    merged_data.index,
    merged_data["lower_bound"],
    merged_data["upper_bound"],
    color="gray",
    alpha=0.5,
    label="±25% Range",
)

plt.legend()
plt.title("Comparison of Actual and Predicted Steering Angles with ±25% Range")
plt.xlabel("Timestamp")
plt.ylabel("Steering Angle")
plt.grid(True)
plt.show()

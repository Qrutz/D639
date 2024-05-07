import pandas as pd
from sklearn.preprocessing import StandardScaler
from sklearn.linear_model import LinearRegression
from sklearn.model_selection import train_test_split
from sklearn.metrics import mean_squared_error
from sklearn.impute import SimpleImputer
import joblib


def load_and_clean(filepath, columns):
    """Load CSV file with specified delimiter and keep only necessary columns."""
    df = pd.read_csv(filepath, delimiter=";", usecols=columns)
    return df


def merge_dataframes(dataframes, merge_on):
    """Merge dataframes based on the given keys."""
    merged_df = dataframes[0]
    for df in dataframes[1:]:
        merged_df = pd.merge(merged_df, df, on=merge_on, how="outer")
    return merged_df


def train_model(combined_df):
    """Train a linear regression model."""
    # Normalize data for training/model preparation
    scaler = StandardScaler()
    features_to_scale = [
        "IRLeft_voltage",
        "IRRight_voltage",
        "groundSteering",
        "angularVelocityX",
        "angularVelocityY",
        "angularVelocityZ",
    ]
    combined_df[features_to_scale] = scaler.fit_transform(
        combined_df[features_to_scale]
    )

    # Set target variable
    X = combined_df.drop(["groundSteering"], axis=1)
    y = combined_df["groundSteering"]

    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, random_state=42
    )

    imputer = SimpleImputer(strategy="mean")

    # Fit and transform the data to handle missing values
    X_train_imputed = imputer.fit_transform(X_train)
    X_test_imputed = imputer.transform(X_test)

    # Make sure input y also has no missing values
    y_train = y_train.fillna(y_train.mean())
    y_test = y_test.fillna(y_test.mean())

    # Train model
    model = LinearRegression()
    model.fit(X_train_imputed, y_train)

    # Evaluate model
    y_pred = model.predict(X_test_imputed)
    mse = mean_squared_error(y_test, y_pred)
    print(f"Mean Squared Error: {mse}")

    return model, imputer


# data[0] = "CSV-Files/REC0 /opendlv.proxy.VoltageReading-1.csv"
# data[1] = "CSV-Files/REC0 /opendlv.proxy.VoltageReading-3.csv"
# data[2] = "CSV-Files/REC0 /opendlv.proxy.GroundSteeringRequest-0.csv"
# data[3] = "CSV-Files/REC0 /opendlv.proxy.AngularVelocityReading-0.csv"
def preprocess_data(
    voltageReading1,
    voltageReading3,
    GroundSteering,
    AngularVelocityReading,
    for_training=True,
):
    columns_IR = ["sampleTimeStamp.seconds", "sampleTimeStamp.microseconds", "voltage"]
    columns_velocity = [
        "sampleTimeStamp.seconds",
        "sampleTimeStamp.microseconds",
        "angularVelocityX",
        "angularVelocityY",
        "angularVelocityZ",
    ]

    IRLeft = load_and_clean(voltageReading1, columns_IR)
    IRLeft.columns = [
        "sampleTimeStamp.seconds",
        "sampleTimeStamp.microseconds",
        "IRLeft_voltage",
    ]
    IRRight = load_and_clean(voltageReading3, columns_IR)
    IRRight.columns = [
        "sampleTimeStamp.seconds",
        "sampleTimeStamp.microseconds",
        "IRRight_voltage",
    ]
    merge_on = ["sampleTimeStamp.seconds", "sampleTimeStamp.microseconds"]
    # Steering is only available during training
    if for_training:
        columns_steering = [
            "sampleTimeStamp.seconds",
            "sampleTimeStamp.microseconds",
            "groundSteering",
        ]
        Steering = load_and_clean(GroundSteering, columns_steering)
        Velocity = load_and_clean(AngularVelocityReading, columns_velocity)
        # Merge dataframes based on timestamp since dataflow is not perfectly synchronized

        combined_df = merge_dataframes([IRLeft, IRRight, Steering, Velocity], merge_on)
    else:
        Velocity = load_and_clean(AngularVelocityReading, columns_velocity)
        combined_df = merge_dataframes([IRLeft, IRRight, Velocity], merge_on)

    # Sort dataframe by timestamp if necessary
    combined_df.sort_values(by=merge_on, inplace=True)

    return combined_df


def test_model(model, new_data, imputer):
    """Make predictions using the trained model."""
    # missing value handling
    new_data_imputed = imputer.transform(new_data)

    # Make predictions
    predictions = model.predict(new_data_imputed)

    return predictions


def loadAllData(file_groups):
    all_data_frames = []
    for group in file_groups:
        preprocessed_data = preprocess_data(
            voltageReading1=group[0],
            voltageReading3=group[1],
            GroundSteering=group[2],
            AngularVelocityReading=group[3],
            for_training=True,
        )
        all_data_frames.append(preprocessed_data)
    return pd.concat(all_data_frames, ignore_index=True)


if __name__ == "__main__":

    RecordingToPredict = [
        "CSV-Files/CID-140-recording-2020-03-18_150001-selection.rec.csv/opendlv.proxy.VoltageReading-1.csv",
        "CSV-Files/CID-140-recording-2020-03-18_150001-selection.rec.csv/opendlv.proxy.VoltageReading-3.csv",
        "CSV-Files/CID-140-recording-2020-03-18_150001-selection.rec.csv/opendlv.proxy.AngularVelocityReading-0.csv",
    ]

    # TrainingData = preprocess_data(
    #     voltageReading1="CSV-Files/REC0/opendlv.proxy.VoltageReading-1.csv",
    #     voltageReading3="CSV-Files/REC0/opendlv.proxy.VoltageReading-3.csv",
    #     GroundSteering="CSV-Files/REC0/opendlv.proxy.GroundSteeringRequest-0.csv",
    #     AngularVelocityReading="CSV-Files/REC0/opendlv.proxy.AngularVelocityReading-0.csv",
    #     for_training=True,
    # )

    file_paths = [
        [
            "CSV-Files/CID-140-recording-2020-03-18_144821-selection.rec.csv/opendlv.proxy.VoltageReading-1.csv",
            "CSV-Files/CID-140-recording-2020-03-18_144821-selection.rec.csv/opendlv.proxy.VoltageReading-3.csv",
            "CSV-Files/CID-140-recording-2020-03-18_144821-selection.rec.csv/opendlv.proxy.GroundSteeringRequest-0.csv",
            "CSV-Files/CID-140-recording-2020-03-18_144821-selection.rec.csv/opendlv.proxy.AngularVelocityReading-0.csv",
        ],
        [
            "CSV-Files/CID-140-recording-2020-03-18_145043-selection.rec.csv/opendlv.proxy.VoltageReading-1.csv",
            "CSV-Files/CID-140-recording-2020-03-18_145043-selection.rec.csv/opendlv.proxy.VoltageReading-3.csv",
            "CSV-Files/CID-140-recording-2020-03-18_145043-selection.rec.csv/opendlv.proxy.GroundSteeringRequest-0.csv",
            "CSV-Files/CID-140-recording-2020-03-18_145043-selection.rec.csv/opendlv.proxy.AngularVelocityReading-0.csv",
        ],
        [
            "CSV-Files/CID-140-recording-2020-03-18_145233-selection.rec.csv/opendlv.proxy.VoltageReading-1.csv",
            "CSV-Files/CID-140-recording-2020-03-18_145233-selection.rec.csv/opendlv.proxy.VoltageReading-3.csv",
            "CSV-Files/CID-140-recording-2020-03-18_145233-selection.rec.csv/opendlv.proxy.GroundSteeringRequest-0.csv",
            "CSV-Files/CID-140-recording-2020-03-18_145233-selection.rec.csv/opendlv.proxy.AngularVelocityReading-0.csv",
        ],
        [
            "CSV-Files/CID-140-recording-2020-03-18_145641-selection.rec.csv/opendlv.proxy.VoltageReading-1.csv",
            "CSV-Files/CID-140-recording-2020-03-18_145641-selection.rec.csv/opendlv.proxy.VoltageReading-3.csv",
            "CSV-Files/CID-140-recording-2020-03-18_145641-selection.rec.csv/opendlv.proxy.GroundSteeringRequest-0.csv",
            "CSV-Files/CID-140-recording-2020-03-18_145641-selection.rec.csv/opendlv.proxy.AngularVelocityReading-0.csv",
        ],
    ]

    # # Load all data
    TrainingData = loadAllData(file_paths)

    # Train model
    [model, imputer] = train_model(TrainingData)

    # Save model
    joblib.dump(model, "modelmultiple.pkl")

    # load the trained model
    model = joblib.load("modelmultiple.pkl")

    # Load and preprocess new data
    PredictionData = preprocess_data(
        voltageReading1=RecordingToPredict[0],
        voltageReading3=RecordingToPredict[1],
        GroundSteering=None,
        AngularVelocityReading=RecordingToPredict[2],
        for_training=False,
    )

    predictions = test_model(model, PredictionData, imputer)

    # save the predictions
    pd.DataFrame(predictions).to_csv("predictions.csv")

    # plot the original steering data and the predictions
    import matplotlib.pyplot as plt

    Steering_new = load_and_clean(
        "CSV-Files/CID-140-recording-2020-03-18_150001-selection.rec.csv/opendlv.proxy.GroundSteeringRequest-0.csv",
        columns=[
            "sampleTimeStamp.seconds",
            "sampleTimeStamp.microseconds",
            "groundSteering",
        ],
    )

    plt.plot(Steering_new["groundSteering"], label="Original Steering")
    plt.plot(predictions, label="Predicted Steering")
    plt.legend()

    ## calculate whether the predicted steering is withing a +- 25% range, but only for when prediction and steering are both available

    plt.show()

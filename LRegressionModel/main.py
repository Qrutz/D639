import pandas as pd


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


if __name__ == "__main__":
    columns_IR = ["sampleTimeStamp.seconds", "sampleTimeStamp.microseconds", "voltage"]
    columns_steering = [
        "sampleTimeStamp.seconds",
        "sampleTimeStamp.microseconds",
        "groundSteering",
    ]
    columns_velocity = [
        "sampleTimeStamp.seconds",
        "sampleTimeStamp.microseconds",
        "angularVelocityX",
        "angularVelocityY",
        "angularVelocityZ",
    ]

    IRLeft = load_and_clean(
        "CSV-Files/REC0 /opendlv.proxy.VoltageReading-1.csv", columns_IR
    )
    IRLeft.columns = [
        "sampleTimeStamp.seconds",
        "sampleTimeStamp.microseconds",
        "IRLeft_voltage",
    ]
    IRRight = load_and_clean(
        "CSV-Files/REC0 /opendlv.proxy.VoltageReading-3.csv", columns_IR
    )
    IRRight.columns = [
        "sampleTimeStamp.seconds",
        "sampleTimeStamp.microseconds",
        "IRRight_voltage",
    ]
    Steering = load_and_clean(
        "CSV-Files/REC0 /opendlv.proxy.GroundSteeringRequest-0.csv", columns_steering
    )
    Velocity = load_and_clean(
        "CSV-Files/REC0 /opendlv.proxy.AngularVelocityReading-0.csv", columns_velocity
    )

    # Merge dataframes based on timestamp since dataflow is not perfectly synchronized (i think...)
    merge_on = ["sampleTimeStamp.seconds", "sampleTimeStamp.microseconds"]
    combined_df = merge_dataframes([IRLeft, IRRight, Steering, Velocity], merge_on)

    # Sort dataframe by timestamp if necessary
    combined_df.sort_values(by=merge_on, inplace=True)

    # Print combined dataframe
    print(combined_df)

    # Save the combined dataframe
    combined_df.to_csv("combined_df.csv", index=False)

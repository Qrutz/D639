#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "CommonDefs.hpp"
#include "AngleCalculator.hpp"
#include <iostream>

AngleCalculator::AngleCalculator() {}

float AngleCalculator::CalculateSteeringAngle(cv::Mat &yellowInputImage, cv::Mat &blueInputImage, float &steeringWheelAngle, bool isClockwise, float maxSteering, float minSteering) {
    std::vector<std::vector<cv::Point>> blueContours;
    std::vector<std::vector<cv::Point>> yellowContours;

    // Assuming you know the dimensions of the image and the distracting area
    int cropHeight = 50;  // Height in pixels to crop from the bottom
    cv::Rect roi(0, 0, yellowInputImage.cols, yellowInputImage.rows - cropHeight);
    cv::Mat croppedBlueImage = blueInputImage(roi);
    cv::Mat croppedYellowImage = yellowInputImage(roi);

    cv::findContours(croppedBlueImage, blueContours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
    cv::findContours(croppedYellowImage, yellowContours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    // Define minimum and maximum contour areas
    double minArea = 130.0;  // Minimum area to consider a contour
    double maxArea = 1000.0;  // Maximum area to avoid abnormally large contours

    // Define aspect ratio thresholds (specific to the expected shape of the cones)
    float minAspectRatio = 0.5;  // Minimum aspect ratio
    float maxAspectRatio = 2.0;  // Maximum aspect ratio

    std::vector<std::vector<cv::Point>> filteredYellowContours;
    for (const auto& contour : yellowContours) {
        double area = cv::contourArea(contour);

        // Calculate bounding rectangle to get aspect ratio
        cv::Rect boundingBox = cv::boundingRect(contour);
        float aspectRatio = static_cast<float>(boundingBox.width) / boundingBox.height;

        // Check if contour matches area and aspect ratio criteria
        if (area >= minArea && area <= maxArea && aspectRatio >= minAspectRatio && aspectRatio <= maxAspectRatio) {
            filteredYellowContours.push_back(contour);
        }
    }

    // Filter yellow contours based on area
    std::vector<std::vector<cv::Point>> filteredBlueContours;
    for (const auto& contour : blueContours) {
        double area = cv::contourArea(contour);
        if (area >= minArea && area <= maxArea) {
            filteredBlueContours.push_back(contour);
        }
    }

    cv::Point imageCenter(blueInputImage.cols / 2, blueInputImage.rows / 2);

    cv::Point blueCentroid = calculateCentroid(filteredBlueContours, imageCenter);
    cv::Point yellowCentroid = calculateCentroid(filteredYellowContours, imageCenter);

    // Create a visual output by combining the blue and yellow images
    cv::Mat visualOutput;
    cv::cvtColor(blueInputImage, visualOutput, cv::COLOR_GRAY2BGR); // Convert blue image to color for visualization
    cv::Mat yellowBGR;
    cv::cvtColor(yellowInputImage, yellowBGR, cv::COLOR_GRAY2BGR);  // Convert yellow image to color
    cv::addWeighted(visualOutput, 0.5, yellowBGR, 0.5, 0.0, visualOutput);  // Blend both images

    // Draw image center
    cv::circle(visualOutput, imageCenter, 5, cv::Scalar(0, 255, 0), -1); // Green color

    // Draw centroids
    cv::circle(visualOutput, blueCentroid, 5, cv::Scalar(255, 0, 0), -1); // Blue color for blue centroid
    cv::circle(visualOutput, yellowCentroid, 5, cv::Scalar(0, 255, 255), -1); // Yellow color for yellow centroid

    // Draw lines from image center to centroids
    cv::line(visualOutput, imageCenter, blueCentroid, cv::Scalar(255, 0, 0), 2); // Blue line to blue centroid
    cv::line(visualOutput, imageCenter, yellowCentroid, cv::Scalar(0, 255, 255), 2); // Yellow line to yellow centroid

    // Display the visual output
    cv::imshow("Visual Output", visualOutput);

    float newSteering = steeringWheelAngle;
    float blueAdjustment = adjustSteering(blueCentroid, imageCenter, !isClockwise);
    float yellowAdjustment = adjustSteering(yellowCentroid, imageCenter, isClockwise);

    newSteering += (isClockwise ? blueAdjustment - yellowAdjustment : -blueAdjustment + yellowAdjustment);

    // Apply smoothing to new steering angle to ensure gentle transitions
    newSteering = smoothSteering(steeringWheelAngle, newSteering, 0.1f); // Example smoothing factor, adjust as needed

    // Clamp the steering value to be within allowed limits
    newSteering = std::max(minSteering, std::min(maxSteering, newSteering));

       // More detailed debug output based on steering decision
    if (newSteering > steeringWheelAngle) {
        std::cout << "Turning Right. New Steering Angle: " << newSteering << std::endl;
    } else if (newSteering < steeringWheelAngle) {
        std::cout << "Turning Left. New Steering Angle: " << newSteering << std::endl;
    } else {
        std::cout << "Maintaining Direction. Steering Angle: " << newSteering << std::endl;
    }

    return newSteering;
}


cv::Point AngleCalculator::calculateCentroid(const std::vector<std::vector<cv::Point>>& contours, const cv::Point& imageCenter) {
    cv::Moments m;
    int xSum = 0, ySum = 0, count = 0;

    for (const auto& contour : contours) {
        m = cv::moments(contour);
        if (m.m00 > std::numeric_limits<double>::epsilon()) {  // Avoid division by zero by checking against epsilon
            xSum += static_cast<int>(m.m10 / m.m00);
            ySum += static_cast<int>(m.m01 / m.m00);
            count++;
        }
    }
    return count > 0 ? cv::Point(xSum / count, ySum / count) : imageCenter;  // Fallback to image center if no valid centroids found
}

float AngleCalculator::adjustSteering(const cv::Point& centroid, const cv::Point& center, bool shouldSteerRight) {
    float error = 

    return shouldSteerRight ? error : -error;
}

float AngleCalculator::smoothSteering(float currentSteering, float newSteering, float smoothingFactor) {
    return currentSteering * (1.0f - smoothingFactor) + newSteering * smoothingFactor;
}
    


#include "DesiredpathPlanner.hpp"
#include <opencv2/imgproc.hpp>

PathCalculator::PathCalculator() {}

std::vector<cv::Point2f> PathCalculator::calculatePathPoints(const std::vector<std::vector<cv::Point>> &blueContours, const std::vector<std::vector<cv::Point>> &yellowContours)
{
    std::vector<cv::Point2f> pathPoints;
    std::vector<cv::Point2f> blueCenters, yellowCenters;

    // Calculate centroids for blue contours
    for (const auto &contour : blueContours)
    {
        cv::Moments m = cv::moments(contour, true);
        blueCenters.push_back(cv::Point2f(m.m10 / m.m00, m.m01 / m.m00));
    }

    // Calculate centroids for yellow contours
    for (const auto &contour : yellowContours)
    {
        cv::Moments m = cv::moments(contour, true);
        yellowCenters.push_back(cv::Point2f(m.m10 / m.m00, m.m01 / m.m00));
    }

    // Pair and find path points
    for (const auto &blueCenter : blueCenters)
    {
        float minDistance = std::numeric_limits<float>::max();
        cv::Point2f closestYellowCenter;

        for (const auto &yellowCenter : yellowCenters)
        {
            float distance = cv::norm(blueCenter - yellowCenter);
            if (distance < minDistance)
            {
                minDistance = distance;
                closestYellowCenter = yellowCenter;
            }
        }

        cv::Point2f midpoint = (blueCenter + closestYellowCenter) / 2.0f;
        pathPoints.push_back(midpoint);
    }

    return pathPoints;
}

void PathCalculator::drawPath(cv::Mat &image, const std::vector<cv::Point2f> &pathPoints)
{
    for (const auto &pathPoint : pathPoints)
    {
        cv::circle(image, pathPoint, 5, cv::Scalar(0, 255, 0), -1);
    }
}

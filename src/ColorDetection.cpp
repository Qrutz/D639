// ColorDetection.cpp
#include "ColorDetection.hpp"
#include <opencv2/imgproc/imgproc.hpp>

ColorDetector::ColorDetector(const ColorThreshold &blueThreshold, const ColorThreshold &yellowThreshold)
    : m_blueThreshold(blueThreshold), m_yellowThreshold(yellowThreshold) {}

cv::Mat ColorDetector::detectColor(const cv::Mat &inputFrame, const ColorThreshold &threshold)
{
    cv::Mat imgHSV;
    cv::cvtColor(inputFrame, imgHSV, cv::COLOR_BGR2HSV);
    cv::Mat mask;
    cv::inRange(imgHSV, threshold.lower, threshold.upper, mask);
    cv::Mat coloredOnly;
    cv::bitwise_and(inputFrame, inputFrame, coloredOnly, mask);
    return coloredOnly;
}

cv::Mat ColorDetector::detectBlue(const cv::Mat &inputFrame)
{
    return detectColor(inputFrame, m_blueThreshold);
}

cv::Mat ColorDetector::detectYellow(const cv::Mat &inputFrame)
{
    return detectColor(inputFrame, m_yellowThreshold);
}

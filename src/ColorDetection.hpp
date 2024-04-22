// ColorDetection.hpp
#ifndef COLOR_DETECTION_HPP
#define COLOR_DETECTION_HPP

#include <opencv2/core/core.hpp>

// Struct to hold color thresholds
struct ColorThreshold
{
    cv::Scalar lower;
    cv::Scalar upper;
};

class ColorDetector
{
public:
    ColorDetector(const ColorThreshold &blueThreshold, const ColorThreshold &yellowThreshold);
    cv::Mat detectColor(const cv::Mat &inputFrame, const ColorThreshold &threshold);
    cv::Mat detectBlue(const cv::Mat &inputFrame);
    cv::Mat detectYellow(const cv::Mat &inputFrame);

private:
    ColorThreshold m_blueThreshold;
    ColorThreshold m_yellowThreshold;
};

#endif // COLOR_DETECTION_HPP

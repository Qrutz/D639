#ifndef PATH_CALCULATION_HPP
#define PATH_CALCULATION_HPP

#include <opencv2/core.hpp>
#include <vector>

class PathCalculator
{
public:
    PathCalculator();
    std::vector<cv::Point2f> calculatePathPoints(const std::vector<std::vector<cv::Point>> &blueContours, const std::vector<std::vector<cv::Point>> &yellowContours);
    void drawPath(cv::Mat &image, const std::vector<cv::Point2f> &pathPoints);
};

#endif

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "NoiseRemover.hpp"

NoiseRemover::NoiseRemover(){};

cv::Mat NoiseRemover::RemoveNoise(const cv::Mat &inputFrame){
        cv::GaussianBlur(inputFrame, inputFrame, cv::Size(3, 3), 0);   //Blur Effect
        cv::erode(inputFrame, inputFrame, 0);         // Erode Filter Effect
        cv::dilate(inputFrame, inputFrame, 0);        // Dilate Filter Effect

        return inputFrame;
}

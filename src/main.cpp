/*
 * Copyright (C) 2020  Christian Berger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Include the single-file, header-only middleware libcluon to create high-performance microservices
#include "cluon-complete.hpp"
// Include the OpenDLV Standard Message Set that contains messages that are usually exchanged for automotive or robotic applications

// Include the OpenDLV Standard Message Set that contains messages that are usually exchanged for automotive or robotic applications
#include "opendlv-standard-message-set.hpp"

// Include the GUI and image processing header files from OpenCV
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

int32_t main(int32_t argc, char **argv)
{
    int32_t retCode{1};
    // Parse the command line parameters as we require the user to specify some mandatory information on startup.
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ((0 == commandlineArguments.count("cid")) ||
        (0 == commandlineArguments.count("name")) ||
        (0 == commandlineArguments.count("width")) ||
        (0 == commandlineArguments.count("height")))
    {
        std::cerr << argv[0] << " attaches to a shared memory area containing an ARGB image." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --cid=<OD4 session> --name=<name of shared memory area> [--verbose]" << std::endl;
        std::cerr << "         --cid:    CID of the OD4Session to send and receive messages" << std::endl;
        std::cerr << "         --name:   name of the shared memory area to attach" << std::endl;
        std::cerr << "         --width:  width of the frame" << std::endl;
        std::cerr << "         --height: height of the frame" << std::endl;
        std::cerr << "Example: " << argv[0] << " --cid=253 --name=img --width=640 --height=480 --verbose" << std::endl;
    }
    else
    {
        // Extract the values from the command line parameters
        const std::string NAME{commandlineArguments["name"]};
        const uint32_t WIDTH{static_cast<uint32_t>(std::stoi(commandlineArguments["width"]))};
        const uint32_t HEIGHT{static_cast<uint32_t>(std::stoi(commandlineArguments["height"]))};
        const bool VERBOSE{commandlineArguments.count("verbose") != 0};


        // Attach to the shared memory.
        std::unique_ptr<cluon::SharedMemory> sharedMemory{new cluon::SharedMemory{NAME}};
        if (sharedMemory && sharedMemory->valid())
        {
            std::clog << argv[0] << ": Attached to shared memory '" << sharedMemory->name() << " (" << sharedMemory->size() << " bytes)." << std::endl;

            // Interface to a running OpenDaVINCI session where network messages are exchanged.
            // The instance od4 alblueLowS you to send and receive messages.
            cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))};

            opendlv::proxy::GroundSteeringRequest gsr;
            std::mutex gsrMutex;
            auto onGroundSteeringRequest = [&gsr, &gsrMutex](cluon::data::Envelope &&env)
            {
                // The envelope data structure provide further details, such as sampleTimePoint as shown in this test case:
                // https://github.com/chrberger/libcluon/blob/master/libcluon/testsuites/TestEnvelopeConverter.cpp#L31-L40
                std::lock_guard<std::mutex> lck(gsrMutex);
                gsr = cluon::extractMessage<opendlv::proxy::GroundSteeringRequest>(std::move(env));
                std::cout << "lambda: groundSteering = " << gsr.groundSteering() << std::endl;
            };

            od4.dataTrigger(opendlv::proxy::GroundSteeringRequest::ID(), onGroundSteeringRequest);


            int iLowH = 0;
            int iHighH = 179;

            int iLowS = 0; 
            int iHighS = 255;

            int iLowV = 0;
            int iHighV = 255;

            // Add trackbars for manually adjusting HSV during runtime. Makes it easier to experiment with filters and finding
            // the correct HSV values.
            cv::namedWindow("Control", cv::WINDOW_AUTOSIZE);
            cv::createTrackbar("LowH", "Control", &iLowH, 179); //Hue (0 - 179)
            cv::createTrackbar("HighH", "Control", &iHighH, 179);

            cv::createTrackbar("LowS", "Control", &iLowS, 255); //Saturation (0 - 255)
            cv::createTrackbar("HighS", "Control", &iHighS, 255);

            cv::createTrackbar("LowV", "Control", &iLowV, 255); //Value (0 - 255)
            cv::createTrackbar("HighV", "Control", &iHighV, 255);


            // Endless loop; end the program by pressing Ctrl-C.
            while (od4.isRunning())
            {
                // OpenCV data structure to hold an image.
                
                cv::Mat img;
                cv::Mat hsvImg;    // HSV Image
                cv::Mat hsvImg2;    // HSV Image
                cv::Mat threshImg;   // Thresh Image
                cv::Mat threshImg2;   // Thresh Image
                cv::Mat finalThresh;

                // Wait for a notification of a new frame.
                sharedMemory->wait();

                // Lock the shared memory.
                sharedMemory->lock();
                {
                    // Copy the pixels from the shared memory into our own data structure.
                    cv::Mat wrapped(HEIGHT, WIDTH, CV_8UC4, sharedMemory->data());
                    img = wrapped.clone();
                }
                // Crop bottom half. Only bottom 50% part will be used for processing and contour tracking.
                cv::Rect roi(0, img.rows / 2, img.cols, img.rows / 2);
                cv::Mat croppedImg = img(roi);

                // inRange filters out blue colors. Use gaussian blur to smooth out image, and morphological operations
                // Erode makes objects smaller but fills in the holes. Dilate does the opposite, so if you combine them
                // it will make a nice end result
                cv::cvtColor(croppedImg, hsvImg, CV_BGR2HSV);
                hsvImg2 = hsvImg.clone();
                cv::inRange(hsvImg, cv::Scalar(iLowH, iLowS, iLowV), cv::Scalar(iHighH, iHighS, iHighV), threshImg);
                cv::inRange(hsvImg2, cv::Scalar(15, 50, 50), cv::Scalar(30, 255, 255), threshImg2);

                cv::bitwise_or(threshImg, threshImg2, finalThresh);

                cv::GaussianBlur(finalThresh, finalThresh, cv::Size(5, 5), 0);   //Blur Effect
                cv::erode(finalThresh, finalThresh, 0);         // Erode Filter Effect
                cv::dilate(finalThresh, finalThresh, 0);        // Dilate Filter Effect

                // Find contours and save them in contours.
                std::vector<std::vector<cv::Point>> contours;
                cv::findContours(finalThresh, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

                // Create a black image, size of original img.
                cv::Mat contourOutput = cv::Mat::zeros(img.size(), img.type());
                // Add the processed contour to the black image. We can not just add the contours to the black image and then
                // add it to original image by using addWeighted. We have to manually offset the points to the correct position.
                // This is to ensure that the contours appear where they should in the original picture.
                for (const auto& contour : contours) {
                    std::vector<cv::Point> shiftedContour;
                    for (const auto& point : contour) {
                        shiftedContour.push_back(cv::Point(point.x, point.y + img.rows / 2)); // Offset vertically
                        }
                    cv::drawContours(contourOutput, std::vector<std::vector<cv::Point>>{shiftedContour}, -1, cv::Scalar(0, 255, 0), 2);
                }

                // Now we can combine the contours with the original picture. The reason for doing this is to eliminate the noise
                // in the top part of the picture, and the contour processing is only done on 50% of the original image, which should
                // increase performance.
                cv::Mat finalOutput;
                cv::addWeighted(img, 1.0, contourOutput, 1.0, 0.0, finalOutput);

                // TODO: Here, you can add some code to check the sampleTimePoint when the current frame was captured.
                sharedMemory->unlock();

                // Display image on your screen.
                if (VERBOSE)
                {

                    //cv::imshow(sharedMemory->name().c_str(), img);
                    cv::imshow("ResultImg", threshImg);
                    cv::imshow("Blue tracking", finalOutput);

                    cv::waitKey(1);
                }
            }
        }
        retCode = 0;
    }
    return retCode;
}

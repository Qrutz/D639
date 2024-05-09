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
#include "HsvColorSeparator.hpp"

#include "NoiseRemover.hpp"
#include "ContourFinder.hpp"

int32_t main(int32_t argc, char **argv)
{
    int32_t retCode{1};

    // Open a file for writing our steering angles
    std::ofstream outputFile("../steeringAngles.csv", std::ios::out | std::ios::app);

    // Write headers if the file is new or empty
    if (outputFile.tellp() == 0)
    {
        outputFile << "Timestamp, SteeringAngle, OriginalSteering, Within_25_percent?\n";
    }
    else
    {
        // If the file is not empty, we need to clear it
        outputFile.close();
        outputFile.open("../steeringAngles.csv", std::ios::out | std::ios::trunc);
        outputFile << "Timestamp, SteeringAngle, OriginalSteering, Within_25_percent?\n";
    }
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

        ColorThreshold blueThreshold{{100, 150, 50}, {140, 255, 255}};  // HSV
        ColorThreshold yellowThreshold{{20, 100, 100}, {30, 255, 255}}; // HSV

        ColorDetector detector(blueThreshold, yellowThreshold); //  ColorDetector object

        double steeringAngle = 0.0; // default steering angle

        double LeftIR;
        double RightIR;

        // For TESTING STUFF
        int totalEntries = 0;
        int totalWithinRange = 0;

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

            HsvColorSeparator colorSeparator;
            NoiseRemover noiseRemover;
            ContourFinder contourFinder;

            int maxContourArea = 1000;
            int minContourArea = 100;

            cv::namedWindow("Color tracking", cv::WINDOW_AUTOSIZE);
            cv::createTrackbar("maxContourArea", "Color tracking", &maxContourArea, 2500);
            cv::createTrackbar("minContourArea", "Color tracking", &minContourArea, 2500);


            opendlv::proxy::VoltageReading vr;
            std::mutex vrMutex;
            auto onVoltageReading = [&vr, &vrMutex, &LeftIR, &RightIR](cluon::data::Envelope &&env)
            {
                std::lock_guard<std::mutex> lck(vrMutex);
                vr = cluon::extractMessage<opendlv::proxy::VoltageReading>(std::move(env));

                // stamp 3 is right sensor and stamp 1 is left sensor
                if (env.senderStamp() == 3)
                {
                    // SET RIGHT IR SENSOR VALUE
                    RightIR = vr.voltage();
                }
                else if (env.senderStamp() == 1)
                {

                    // SET LEFT IR SENSOR VALUE
                    LeftIR = vr.voltage();
                }
            };

            od4.dataTrigger(opendlv::proxy::VoltageReading::ID(), onVoltageReading);

            opendlv::proxy::AccelerationReading ar;
            std::mutex arMutex;
            auto onAccelerationReading = [&ar, &arMutex](cluon::data::Envelope &&env)
            {
                std::lock_guard<std::mutex> lck(arMutex);
                ar = cluon::extractMessage<opendlv::proxy::AccelerationReading>(std::move(env));
                std::cout << "Acceleration: " << ar.accelerationX() << std::endl;
            };

            od4.dataTrigger(opendlv::proxy::AccelerationReading::ID(), onAccelerationReading);

            // Open a file for writing
            // std::ofstream outputFile("output.txt");

            // Check if the file is successfully opened
            if (!outputFile.is_open())
            {
                std::cerr << "Error opening file!" << std::endl;
                return 1;
            }

            // Endless loop; end the program by pressing Ctrl-C.
            while (od4.isRunning())
            {
                // OpenCV data structure to hold an image.
                
                cv::Mat img;
                cv::Mat hsvImg;    // HSV Image
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

                // Draw blue and yellow objects, the image has to be cut off the top 50% first
                cv::Rect roi(0, img.rows / 2, img.cols, img.rows / 2);
                cv::Mat blueObjects = img(roi).clone();
                cv::Mat yellowObjects = img(roi).clone();

                // Detect blue and yellow objects
                cv::Mat blueMask = detector.detectBlue(blueObjects);
                cv::Mat yellowMask = detector.detectYellow(yellowObjects);

                // find the contours of the blue and yellow objects on the bottom half of the image
                std::vector<std::vector<cv::Point>> blueContours, yellowContours;
                cv::findContours(blueMask, blueContours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
                cv::findContours(yellowMask, yellowContours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

                // cv::cvtColor(blueMask, blueObjects, cv::COLOR_GRAY2BGR);
                // cv::cvtColor(yellowMask, yellowObjects, cv::COLOR_GRAY2BGR);

                for (const auto &contour : blueContours)
                {
                    std::vector<cv::Point> offsetContour;
                    for (const auto &point : contour)
                    {
                        offsetContour.push_back(cv::Point(point.x, point.y + img.rows / 2));
                    }
                    cv::drawContours(img, std::vector<std::vector<cv::Point>>{offsetContour}, -1, cv::Scalar(255, 0, 0), 2);
                }

                for (const auto &contour : yellowContours)
                {
                    std::vector<cv::Point> offsetContour;
                    for (const auto &point : contour)
                    {
                        offsetContour.push_back(cv::Point(point.x, point.y + img.rows / 2));
                    }
                    cv::drawContours(img, std::vector<std::vector<cv::Point>>{offsetContour}, -1, cv::Scalar(0, 255, 255), 2);
                }

                // After finding contours in the ROI
                std::vector<cv::Point2f> blueCentroids, yellowCentroids;

                // Calculate centroids for blue contours
                for (const auto &contour : blueContours)
                {
                    cv::Moments moments = cv::moments(contour);
                    if (moments.m00 != 0)
                    {
                        cv::Point2f centroid(moments.m10 / moments.m00, moments.m01 / moments.m00);
                        centroid.y += img.rows / 2;
                        blueCentroids.push_back(centroid);
                    }
                }

                // Calculate centroids for yellow contours
                for (const auto &contour : yellowContours)
                {
                    cv::Moments moments = cv::moments(contour);
                    if (moments.m00 != 0)
                    {
                        cv::Point2f centroid(moments.m10 / moments.m00, moments.m01 / moments.m00);
                        centroid.y += img.rows / 2;
                        yellowCentroids.push_back(centroid);
                    }
                }

                const double IR_THRESHOLD_CLOSE = 0.0075; // Example threshold needs calibration

                // If our IR sensor get too close to either side, we need to turn a bit to avoid collision
                if (RightIR < IR_THRESHOLD_CLOSE)
                {
                    steeringAngle -= 0.0035; // Steer left
                }
                else if (LeftIR < IR_THRESHOLD_CLOSE)
                {
                    steeringAngle += 0.0035; // Steer right
                }

                std::pair<bool, cluon::data::TimeStamp> ts = sharedMemory->getTimeStamp();

                int64_t sampleTimePoint = cluon::time::toMicroseconds(ts.second);
                std::string ts_string = std::to_string(sampleTimePoint);

                // TODO: Here, you can add some code to check the sampleTimePoint when the current frame was captured.
                sharedMemory->unlock();

                std::string timestamp_string = "Timestamp: " + ts_string;

                // after receving image
                cv::putText(img,
                            timestamp_string,
                            cv::Point(10, 30),
                            cv::FONT_HERSHEY_PLAIN, 1.0,
                            CV_RGB(255, 255, 255),
                            2);

                // TODO: Do something with the frame.
                // Example: Draw a red rectangle and display image.
                cv::rectangle(img, cv::Point(50, 50), cv::Point(100, 100), cv::Scalar(0, 0, 255));

                // If you want to access the latest received ground steering, don't forget to lock the mutex:
                {
                    std::lock_guard<std::mutex> lck(gsrMutex);
                    float actualSteering = gsr.groundSteering();
                    // group_XY;sampleTimeStamp in microseconds;steeringWheelAngle
                    std::cout << "group_16;" << ts_string << ";" << gsr.groundSteering() << std::endl;

                    float lowerBound = actualSteering * (float)0.75;
                    float upperBound = actualSteering * (float)1.25;
                    bool isWithinRange = (steeringAngle >= lowerBound) && (steeringAngle <= upperBound);

                    if (isWithinRange)
                    {
                        totalWithinRange++;
                    }

                    totalEntries++;

                    // write to file
                    outputFile << ts_string << "," << steeringAngle << "," << gsr.groundSteering() << "," << isWithinRange << "\n";

                    // check if the steering angle is within +-25% of the actual steering
                }

                // Display image on your screen.
                if (VERBOSE)
                {
                    cv::imshow(sharedMemory->name().c_str(), img);
                    cv::imshow("Blue Objects", blueObjects);     // Display blue objects
                    cv::imshow("Yellow Objects", yellowObjects); // Display yellow objects

                    cv::waitKey(1);
                }
            }
        }
        retCode = 0;

        std::cout << "Total entries: " << totalEntries << std::endl;
        std::cout << "Total within range: " << totalWithinRange << std::endl;
    }

    // Close the file
    outputFile.close();

    return retCode;
}

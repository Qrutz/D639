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
#include "ColorDetection.hpp"

#include "DesiredpathPlanner.hpp"

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

        ColorThreshold blueThreshold{{68, 150, 50}, {140, 255, 255}};   // HSV
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
            // The instance od4 allows you to send and receive messages.
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

                // apply mask on blueobjects img
                blueObjects.setTo(cv::Scalar(0, 0, 255), blueMask);

                // find contours in the blue mask
                std::vector<std::vector<cv::Point>> blueContours;
                cv::findContours(blueMask, blueContours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

                // find contours in the yellow mask
                std::vector<std::vector<cv::Point>> yellowContours;
                cv::findContours(yellowMask, yellowContours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

                // filter out small contours
                const int MIN_CONTOUR_AREA = 100;

                for (int i = 0; i < blueContours.size(); i++)
                {
                    if (cv::contourArea(blueContours[i]) < MIN_CONTOUR_AREA)
                    {
                        blueContours.erase(blueContours.begin() + i);
                        i--;
                    }
                }

                for (int i = 0; i < yellowContours.size(); i++)
                {
                    if (cv::contourArea(yellowContours[i]) < MIN_CONTOUR_AREA)
                    {
                        yellowContours.erase(yellowContours.begin() + i);
                        i--;
                    }
                }

                // show contours
                cv::drawContours(blueObjects, blueContours, -1, cv::Scalar(0, 255, 0), 2);

                // show contours
                cv::drawContours(yellowObjects, yellowContours, -1, cv::Scalar(0, 255, 0), 2);

                // After finding contours in the ROI
                std::vector<cv::Point2f> blueCentroids, yellowCentroids;

                // Find the centroids of the blue contours
                for (int i = 0; i < blueContours.size(); i++)
                {
                    cv::Moments M = cv::moments(blueContours[i]);
                    cv::Point2f c(M.m10 / M.m00, M.m01 / M.m00);
                    blueCentroids.push_back(c);
                }

                // show the centroids coordinates of teh closest blue object
                if (blueCentroids.size() > 0)
                {
                    cv::Point2f closestBlue = blueCentroids[0];
                    for (int i = 1; i < blueCentroids.size(); i++)
                    {
                        if (cv::norm(closestBlue - cv::Point2f(img.cols / 2, img.rows)) > cv::norm(blueCentroids[i] - cv::Point2f(img.cols / 2, img.rows)))
                        {
                            closestBlue = blueCentroids[i];
                        }
                    }

                    // Draw a circle around the closest blue object
                    cv::circle(blueObjects, closestBlue, 5, cv::Scalar(255, 0, 0), -1);

                    // also show teh coordinates of the closest blue object
                    cv::putText(blueObjects,
                                "Blue: " + std::to_string(closestBlue.x) + ", " + std::to_string(closestBlue.y),
                                cv::Point(closestBlue.x, closestBlue.y),
                                cv::FONT_HERSHEY_PLAIN, 1.0,
                                CV_RGB(255, 255, 255),
                                2);
                }

                // Find the centroids of the yellow contours
                for (int i = 0; i < yellowContours.size(); i++)
                {
                    cv::Moments M = cv::moments(yellowContours[i]);
                    cv::Point2f c(M.m10 / M.m00, M.m01 / M.m00);
                    yellowCentroids.push_back(c);
                }

                // show the centroids coordinates of teh closest yellow object
                if (yellowCentroids.size() > 0)
                {
                    cv::Point2f closestYellow = yellowCentroids[0];
                    for (int i = 1; i < yellowCentroids.size(); i++)
                    {
                        if (cv::norm(closestYellow - cv::Point2f(img.cols / 2, img.rows)) > cv::norm(yellowCentroids[i] - cv::Point2f(img.cols / 2, img.rows)))
                        {
                            closestYellow = yellowCentroids[i];
                        }
                    }

                    // Draw a circle around the closest yellow object
                    cv::circle(yellowObjects, closestYellow, 5, cv::Scalar(255, 0, 0), -1);

                    // also show teh coordinates of the closest yellow object
                    cv::putText(yellowObjects,
                                "Yellow: " + std::to_string(closestYellow.x) + ", " + std::to_string(closestYellow.y),
                                cv::Point(closestYellow.x, closestYellow.y),
                                cv::FONT_HERSHEY_PLAIN, 1.0,
                                CV_RGB(255, 255, 255),
                                2);
                }

                // draw a line to the closest blue object from the center of the image
                if (blueCentroids.size() > 0)
                {
                    cv::line(blueObjects, cv::Point(img.cols / 2, img.rows), blueCentroids[0], cv::Scalar(255, 0, 0), 2);
                }

                // draw a line to the closest yellow object from the center of the image
                if (yellowCentroids.size() > 0)
                {
                    cv::line(yellowObjects, cv::Point(img.cols / 2, img.rows), yellowCentroids[0], cv::Scalar(255, 0, 0), 2);
                }

                // IF THERES A BLUE AND YELLOW OBJECT CLOSEBY THAT WE HAVE DETECTED
                if (blueCentroids.size() > 0 && yellowCentroids.size() > 0)
                {
                    cv::Point2f blueVector = blueCentroids[0] - cv::Point2f(img.cols / 2, img.rows);
                    cv::Point2f yellowVector = yellowCentroids[0] - cv::Point2f(img.cols / 2, img.rows);

                    // print the vectors
                    std::cout << "Blue Vector: " << blueVector << std::endl;
                    std::cout << "Yellow Vector: " << yellowVector << std::endl;

                    // if the length to the blue and yellow object are somewhat similar we can assume the angle is 0
                    if (cv::norm(blueVector) < 50 || cv::norm(yellowVector) < 50)
                    {
                        steeringAngle = 0.0;
                    }

                    // angle is between -0.3 and 0.3 so we need to adjust accordingly
                    steeringAngle = atan2(yellowVector.y, yellowVector.x) - atan2(blueVector.y, blueVector.x);

                    // Convert the angle to degrees
                    steeringAngle = steeringAngle * 180 / M_PI;

                    // scale it to -0.3 to 0.3
                    steeringAngle = steeringAngle / 180 * 0.3;
                }

                // IF WE ONLY HAVE BLUE OBJECTS IN SIGHT
                else if (blueCentroids.size() > 0)
                {
                    cv::Point2f blueVector = blueCentroids[0] - cv::Point2f(img.cols / 2, img.rows);

                    // print the vectors
                    std::cout << "Blue Vector: " << blueVector << std::endl;

                    // angle is between -0.3 and 0.3 so we need to adjust accordingly
                    steeringAngle = atan2(blueVector.y, blueVector.x);

                    // Convert the angle to degrees
                    steeringAngle = steeringAngle * 180 / M_PI;

                    // scale it to -0.3 to 0.3
                    steeringAngle = steeringAngle / 180 * 0.3;
                }

                // IF WE ONLY HAVE YELLOW OBJECTS IN SIGHT
                else if (yellowCentroids.size() > 0)
                {
                    cv::Point2f yellowVector = yellowCentroids[0] - cv::Point2f(img.cols / 2, img.rows);

                    // print the vectors
                    std::cout << "Yellow Vector: " << yellowVector << std::endl;

                    // angle is between -0.3 and 0.3 so we need to adjust accordingly
                    steeringAngle = atan2(yellowVector.y, yellowVector.x);

                    // Convert the angle to degrees
                    steeringAngle = steeringAngle * 180 / M_PI;

                    // scale it to -0.3 to 0.3
                    steeringAngle = steeringAngle / 180 * 0.3;
                }

                const double IR_THRESHOLD_CLOSE = 0.0075; // Example threshold needs calibration

                // IF WE DONT HAVE ANY OBJECTS IN SIGHT, LETS USE THE IR SENSORS
                if (blueCentroids.size() == 0 && yellowCentroids.size() == 0)
                {
                    // IF THE LEFT IR SENSOR IS TRIGGERED
                    if (LeftIR < IR_THRESHOLD_CLOSE)
                    {
                        steeringAngle = steeringAngle + 0.050;
                    }
                    // IF THE RIGHT IR SENSOR IS TRIGGERED
                    else if (RightIR < IR_THRESHOLD_CLOSE)
                    {
                        steeringAngle = steeringAngle - 0.050;
                    }
                    else
                    {
                        steeringAngle = 0.0;
                    }
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
        // percentage of entries within range
        std::cout << "Percentage within range: " << (float)totalWithinRange / totalEntries * 100 << "%" << std::endl;
    }

    // Close the file
    outputFile.close();

    return retCode;
}

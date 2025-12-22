/*
 * Copyright (c) 2019 Flight Dynamics and Control Lab
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <iostream>
#include <opencv2/objdetect/aruco_detector.hpp>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/calib3d.hpp>
#include <vector>
#include <iostream>
#include <cstdlib>

#include "fdcl_common.hpp"


void drawCubeWireframe(
    cv::InputOutputArray image, cv::InputArray camera_matrix,
    cv::InputArray dist_coeffs, cv::InputArray rvec, cv::InputArray tvec,
    float l
);


int main(int argc, char **argv) {
    cv::CommandLineParser parser(argc, argv, fdcl::keys);

    const char* about = "Draw cube on ArUco marker images";
    auto success = parse_inputs(parser, about);
    if (!success) {
        return 1;
    }

    cv::VideoCapture in_video;
    success = parse_video_in(in_video, parser);
    if (!success) {
        return 1;
    }

    int wait_time = 10;

    int dictionary_id = parser.get<int>("d");
    float marker_length_m = parser.get<float>("l");
    if (marker_length_m <= 0) {
        std::cerr << "Marker length must be a positive value in meter\n";
        return 1;
    }

    cv::Mat image, image_copy;
    cv::Mat camera_matrix, dist_coeffs;

    // Create the dictionary from the same dictionary the marker was generated.
    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(
        static_cast<cv::aruco::PredefinedDictionaryType>(dictionary_id));

    // Create detector
    cv::aruco::ArucoDetector detector(dictionary);

    cv::FileStorage fs("../../calibration_params.yml", cv::FileStorage::READ);
    fs["camera_matrix"] >> camera_matrix;
    fs["distortion_coefficients"] >> dist_coeffs;

    // Define object points for a single marker (centered at origin)
    float half_size = marker_length_m / 2.0f;
    std::vector<cv::Point3f> objPoints = {
        cv::Point3f(-half_size,  half_size, 0),
        cv::Point3f( half_size,  half_size, 0),
        cv::Point3f( half_size, -half_size, 0),
        cv::Point3f(-half_size, -half_size, 0)
    };

    // Initialize a video writer to save the drawn cube.
    int frame_width = static_cast<int>(in_video.get(cv::CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(in_video.get(cv::CAP_PROP_FRAME_HEIGHT));
    int fps = 30;
    int fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
    cv::VideoWriter video(
        "out.avi", fourcc, fps, cv::Size(frame_width, frame_height), true
    );


    while (in_video.grab()) {
        in_video.retrieve(image);
        image.copyTo(image_copy);

        std::vector<int> ids;
        std::vector<std::vector<cv::Point2f>> corners;
        detector.detectMarkers(image, corners, ids);


        // If at least one marker is detected
        if (ids.size() > 0)
        {
            cv::aruco::drawDetectedMarkers(image_copy, corners, ids);

            // Estimate pose for each marker
            for (size_t i = 0; i < ids.size(); i++)
            {
                cv::Vec3d rvec, tvec;
                cv::solvePnP(objPoints, corners[i], camera_matrix, dist_coeffs, rvec, tvec);

                drawCubeWireframe(
                    image_copy, camera_matrix, dist_coeffs, rvec, tvec,
                    marker_length_m
                );

                // This section is going to print the data for the first the
                // detected marker. If you have more than a single marker, it is
                // recommended to change the below section so that either you
                // only print the data for a specific marker, or you print the
                // data for each marker separately.
                if (i == 0) {
                    drawText(image_copy, "x", tvec[0], cv::Point(10, 30));
                    drawText(image_copy, "y", tvec[1], cv::Point(10, 50));
                    drawText(image_copy, "z", tvec[2], cv::Point(10, 70));
                }
            }
        }

        video.write(image_copy);
        cv::imshow("Pose estimation", image_copy);
        char key = (char)cv::waitKey(wait_time);
        if (key == 27) {
            break;
        }
    }

    in_video.release();

    return 0;
}

void drawCubeWireframe(
    cv::InputOutputArray image, cv::InputArray camera_matrix,
    cv::InputArray dist_coeffs, cv::InputArray rvec, cv::InputArray tvec,
    float l
)
{
    float half_l = l / 2.0f;

    // Project cube points
    std::vector<cv::Point3f> axis_points;
    axis_points.push_back(cv::Point3f(half_l, half_l, l));
    axis_points.push_back(cv::Point3f(half_l, -half_l, l));
    axis_points.push_back(cv::Point3f(-half_l, -half_l, l));
    axis_points.push_back(cv::Point3f(-half_l, half_l, l));
    axis_points.push_back(cv::Point3f(half_l, half_l, 0));
    axis_points.push_back(cv::Point3f(half_l, -half_l, 0));
    axis_points.push_back(cv::Point3f(-half_l, -half_l, 0));
    axis_points.push_back(cv::Point3f(-half_l, half_l, 0));

    std::vector<cv::Point2f> image_points;
    projectPoints(
        axis_points, rvec, tvec, camera_matrix, dist_coeffs, image_points
    );

    // Draw cube edges lines
    cv::line(image, image_points[0], image_points[1], cv::Scalar(255, 0, 0), 3);
    cv::line(image, image_points[0], image_points[3], cv::Scalar(255, 0, 0), 3);
    cv::line(image, image_points[0], image_points[4], cv::Scalar(255, 0, 0), 3);
    cv::line(image, image_points[1], image_points[2], cv::Scalar(255, 0, 0), 3);
    cv::line(image, image_points[1], image_points[5], cv::Scalar(255, 0, 0), 3);
    cv::line(image, image_points[2], image_points[3], cv::Scalar(255, 0, 0), 3);
    cv::line(image, image_points[2], image_points[6], cv::Scalar(255, 0, 0), 3);
    cv::line(image, image_points[3], image_points[7], cv::Scalar(255, 0, 0), 3);
    cv::line(image, image_points[4], image_points[5], cv::Scalar(255, 0, 0), 3);
    cv::line(image, image_points[4], image_points[7], cv::Scalar(255, 0, 0), 3);
    cv::line(image, image_points[5], image_points[6], cv::Scalar(255, 0, 0), 3);
    cv::line(image, image_points[6], image_points[7], cv::Scalar(255, 0, 0), 3);
}

/*
By downloading, copying, installing or using the software you agree to this
license. If you do not agree to this license, do not download, install,
copy or use the software.

                          License Agreement
               For Open Source Computer Vision Library
                       (3-clause BSD License)

Copyright (C) 2013, OpenCV Foundation, all rights reserved.
Third party copyrights are property of their respective owners.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

  * Neither the names of the copyright holders nor the names of the contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

This software is provided by the copyright holders and contributors "as is" and
any express or implied warranties, including, but not limited to, the implied
warranties of merchantability and fitness for a particular purpose are
disclaimed. In no event shall copyright holders or contributors be liable for
any direct, indirect, incidental, special, exemplary, or consequential damages
(including, but not limited to, procurement of substitute goods or services;
loss of use, data, or profits; or business interruption) however caused
and on any theory of liability, whether in contract, strict liability,
or tort (including negligence or otherwise) arising in any way out of
the use of this software, even if advised of the possibility of such damage.
*/

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect/aruco_detector.hpp>
#include <opencv2/objdetect/aruco_board.hpp>
#include <iostream>
#include <iomanip>
#include <cmath>

using namespace cv;
using namespace std;

namespace {
const char* about =
    "Create a print-ready ArUco calibration board for A4 or Letter paper.\n"
    "Outputs physical dimensions for use with camera_calibration tool.\n\n"
    "Example usage:\n"
    "  generate_calibration_board board_a4.png -f=A4 -w=5 -h=7 -d=16\n"
    "  generate_calibration_board board_letter.png -f=Letter -w=5 -h=6 -d=16\n";

const char* keys =
    "{@outfile |<none> | Output image file (PNG recommended for print quality) }"
    "{f format |       | Paper format: A4 or Letter }"
    "{w        |       | Number of markers in X direction }"
    "{h        |       | Number of markers in Y direction }"
    "{d        | 16    | Dictionary: DICT_4X4_50=0, DICT_4X4_100=1, DICT_4X4_250=2,"
    "DICT_4X4_1000=3, DICT_5X5_50=4, DICT_5X5_100=5, DICT_5X5_250=6, DICT_5X5_1000=7, "
    "DICT_6X6_50=8, DICT_6X6_100=9, DICT_6X6_250=10, DICT_6X6_1000=11, DICT_7X7_50=12,"
    "DICT_7X7_100=13, DICT_7X7_250=14, DICT_7X7_1000=15, DICT_ARUCO_ORIGINAL=16 }"
    "{dpi      | 300   | Print resolution in DPI (dots per inch) }"
    "{margin   | 10    | Page margin in mm }"
    "{ratio    | 0.8   | Marker to cell ratio (marker_size / (marker_size + separation)) }"
    "{bb       | 1     | Number of bits in marker borders }"
    "{si       | false | Show generated image }";

// Paper dimensions in mm
struct PaperSize {
    double width_mm;
    double height_mm;
    const char* name;
};

const PaperSize PAPER_A4 = { 210.0, 297.0, "A4" };
const PaperSize PAPER_LETTER = { 215.9, 279.4, "Letter" };
}

int main(int argc, char *argv[]) {
    CommandLineParser parser(argc, argv, keys);
    parser.about(about);

    if (argc < 2) {
        parser.printMessage();
        return 0;
    }

    String out = parser.get<String>(0);
    String format = parser.get<String>("format");
    int dpi = parser.get<int>("dpi");
    double marginMm = parser.get<double>("margin");
    double markerRatio = parser.get<double>("ratio");
    int dictionaryId = parser.get<int>("d");
    int borderBits = parser.get<int>("bb");
    bool showImage = parser.get<bool>("si");

    if (!parser.has("format")) {
        cerr << "Error: Paper format (-f) is required. Use A4 or Letter." << endl;
        parser.printMessage();
        return 1;
    }

    if (!parser.has("w") || !parser.has("h")) {
        cerr << "Error: Grid size (-w and -h) is required." << endl;
        parser.printMessage();
        return 1;
    }

    int markersX = parser.get<int>("w");
    int markersY = parser.get<int>("h");

    if (!parser.check()) {
        parser.printErrors();
        return 1;
    }

    // Select paper size
    PaperSize paper;
    if (format == "A4" || format == "a4") {
        paper = PAPER_A4;
    } else if (format == "Letter" || format == "letter" || format == "LETTER") {
        paper = PAPER_LETTER;
    } else {
        cerr << "Error: Unknown paper format '" << format << "'. Use A4 or Letter." << endl;
        return 1;
    }

    // Validate marker ratio
    if (markerRatio <= 0.0 || markerRatio >= 1.0) {
        cerr << "Error: Marker ratio must be between 0 and 1 (exclusive)." << endl;
        return 1;
    }

    // Calculate pixel dimensions based on DPI
    // 1 inch = 25.4 mm
    double pixelsPerMm = dpi / 25.4;
    int pageWidthPx = static_cast<int>(paper.width_mm * pixelsPerMm);
    int pageHeightPx = static_cast<int>(paper.height_mm * pixelsPerMm);
    int marginPx = static_cast<int>(marginMm * pixelsPerMm);

    // Calculate available area for the board
    int availableWidthPx = pageWidthPx - 2 * marginPx;
    int availableHeightPx = pageHeightPx - 2 * marginPx;

    // Calculate cell size (marker + separation) to fit the grid
    // Each row has markersX markers, each column has markersY markers
    // Total width needed: markersX * markerSize + (markersX - 1) * separation + 2 * boardMargin
    // We simplify by making boardMargin = separation
    // Total width: markersX * (markerSize + separation) - separation + 2 * separation
    //            = markersX * (markerSize + separation) + separation
    //            = markersX * cellSize + separation
    // Where cellSize = markerSize + separation

    // Actually, from the original code:
    // imageSize.width = markersX * (markerLength + markerSeparation) - markerSeparation + 2 * margins
    // Let's use separation as the board margin for simplicity
    // width = markersX * cellSize - separation + 2 * separation = markersX * cellSize + separation
    // So cellSize = (width - separation) / markersX

    // We want to maximize marker size while fitting the grid
    // Let cellSize = markerSize + separation
    // markerSize = cellSize * ratio
    // separation = cellSize * (1 - ratio)

    // For the board: width = markersX * cellSize + separation (using separation as margin)
    // cellSize = width / (markersX + (1 - ratio))

    double cellSizeFromWidth = availableWidthPx / (markersX + (1.0 - markerRatio));
    double cellSizeFromHeight = availableHeightPx / (markersY + (1.0 - markerRatio));
    double cellSizePx = min(cellSizeFromWidth, cellSizeFromHeight);

    int markerLengthPx = static_cast<int>(cellSizePx * markerRatio);
    int separationPx = static_cast<int>(cellSizePx * (1.0 - markerRatio));

    // Ensure minimum sizes
    if (markerLengthPx < 10) {
        cerr << "Error: Calculated marker size is too small. Try fewer markers or higher DPI." << endl;
        return 1;
    }

    // Calculate actual board dimensions
    int boardWidthPx = markersX * (markerLengthPx + separationPx) - separationPx + 2 * separationPx;
    int boardHeightPx = markersY * (markerLengthPx + separationPx) - separationPx + 2 * separationPx;

    // Calculate physical dimensions in mm and meters
    double markerLengthMm = markerLengthPx / pixelsPerMm;
    double separationMm = separationPx / pixelsPerMm;
    double markerLengthM = markerLengthMm / 1000.0;
    double separationM = separationMm / 1000.0;
    double boardWidthMm = boardWidthPx / pixelsPerMm;
    double boardHeightMm = boardHeightPx / pixelsPerMm;

    // Create the ArUco board
    aruco::Dictionary dictionary = aruco::getPredefinedDictionary(
        static_cast<aruco::PredefinedDictionaryType>(dictionaryId));

    aruco::GridBoard board(Size(markersX, markersY), float(markerLengthPx),
                           float(separationPx), dictionary);

    // Generate the board image
    Mat boardImage;
    Size imageSize(boardWidthPx, boardHeightPx);
    board.generateImage(imageSize, boardImage, separationPx, borderBits);

    // Create full page image with margins (white background)
    Mat pageImage(pageHeightPx, pageWidthPx, CV_8UC1, Scalar(255));

    // Calculate position to center the board on the page
    int offsetX = (pageWidthPx - boardWidthPx) / 2;
    int offsetY = (pageHeightPx - boardHeightPx) / 2;

    // Copy board to page
    boardImage.copyTo(pageImage(Rect(offsetX, offsetY, boardWidthPx, boardHeightPx)));

    // Save the image
    imwrite(out, pageImage);

    // Print information
    cout << "========================================" << endl;
    cout << "Calibration Board Generated Successfully" << endl;
    cout << "========================================" << endl;
    cout << endl;
    cout << "Paper Format: " << paper.name << " (" << paper.width_mm << "mm x " << paper.height_mm << "mm)" << endl;
    cout << "Output File:  " << out << endl;
    cout << "DPI:          " << dpi << endl;
    cout << "Dictionary:   " << dictionaryId << endl;
    cout << endl;
    cout << "Board Configuration:" << endl;
    cout << "  Grid Size:      " << markersX << " x " << markersY << " markers" << endl;
    cout << "  Board Size:     " << fixed << setprecision(1) << boardWidthMm << "mm x " << boardHeightMm << "mm" << endl;
    cout << endl;
    cout << "Physical Dimensions (for calibration):" << endl;
    cout << "  Marker Length:  " << fixed << setprecision(1) << markerLengthMm << " mm" << endl;
    cout << "  Separation:     " << fixed << setprecision(1) << separationMm << " mm" << endl;
    cout << endl;
    cout << "========================================" << endl;
    cout << "Camera Calibration Command:" << endl;
    cout << "========================================" << endl;
    cout << endl;
    cout << "  camera_calibration -w=" << markersX << " -h=" << markersY
         << " -l=" << fixed << setprecision(6) << markerLengthM
         << " -s=" << fixed << setprecision(6) << separationM
         << " -d=" << dictionaryId << " calibration_params.yml" << endl;
    cout << endl;
    cout << "========================================" << endl;

    if (showImage) {
        // Scale down for display if image is too large
        Mat displayImage;
        double scale = min(1.0, min(1200.0 / pageWidthPx, 900.0 / pageHeightPx));
        if (scale < 1.0) {
            resize(pageImage, displayImage, Size(), scale, scale, INTER_AREA);
        } else {
            displayImage = pageImage;
        }
        imshow("Calibration Board", displayImage);
        waitKey(0);
    }

    return 0;
}

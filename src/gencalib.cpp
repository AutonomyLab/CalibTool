#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "calib.hpp"
using namespace cv;
using namespace std;

void help() {
	cout << "Usage: gencalib [-dh] -i <image> <rows> <cols>" << endl
	     << "       gencalib [-dh] -i <image> -o <calib> <rows> <cols> <zero-x> <zero-y> <last-x> <last-y>" << endl
	     << "Description:" << endl
	     << "  Generates a camera calibration file used to convert pixel coordinates into" << endl
	     << "  world coordinates. There are two modes of operation as shown above. The first" << endl
	     << "  simply detects internal corners and displays the indices. The second generates" << endl
	     << "  a calibration file using the world coordinates of the zeroth and last corners." << endl
	     << "Params:" << endl
	     << "  -d:     Debugging output"
	     << "  image:  the calibration input image" << endl
	     << "  calib:  the calibration output file (must end in \".yaml\")" << endl
	     << "  rows:   the number of rows in the checkerboard" << endl
	     << "  cols:   the number of columns in the checkerboard" << endl
	     << "  zero-x: the world x-coordinate for the zeroth point" << endl
	     << "  zero-y: the world y-coordinate for the zeroth point" << endl
	     << "  last-x: the world x-coordinate for the last point" << endl
	     << "  last-y: the world y-coordinate for the last point" << endl
;
}

bool hasEnding (string const &fullString, string const &ending)
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}


int main(int argc, char** argv) {

	bool debug = false;
	bool iopt = false;
	string infile;
	bool calib = false;
	string outfile;
	int c;
	while ((c = getopt(argc, argv, "hdi:o:")) != -1) {
		switch (c){
		case 'd':
			debug = true;
			break;
		case 'i':
			iopt = true;
			infile = string(optarg);
			break;
		case 'o':
			calib = true;
			outfile = string(optarg);
			if (!hasEnding(outfile, ".yaml")) {
				cout << "Invalid output file name: " << outfile << endl << endl;
				help();
				return 1;
			}
			break;
		case 'h':
			help();
			return 0;
		case '?':
			cout << "Invalid arguments." << endl << endl;
			help();
			return 1;
		}
	}

	int rem = argc - optind;
	if (!iopt || (calib && rem != 6) || (!calib && rem != 2)) {
		cout << "Invalid arguments." << endl << endl;
		help();
		return 1;
	}

	int rows = atoi(argv[optind++]);
	int cols = atoi(argv[optind++]);
	cout << "Detecting corners of " << rows << "x" << cols << " board in "
			<< infile << "..." << endl;

	if (rows < 3 || cols < 3) {
		cout << "Invalid board dimensions." << endl;
		return 1;
	}

	// inner corner dimensions
	int irows = rows - 1;
	int icols = cols - 1;
	Size dim(icols, irows);

	Mat src = imread(infile, 1);
	Mat gray;
	cvtColor(src, gray, CV_BGR2GRAY);
	vector<Point2f> corners;
	bool found = findChessboardCorners(gray, dim, corners,
			CALIB_CB_ADAPTIVE_THRESH
			//+ CALIB_CB_NORMALIZE_IMAGE
					+ CV_CALIB_CB_FILTER_QUADS
			//+ CALIB_CB_FAST_CHECK
			);

	if (found) {
		for (int i = 0; i < corners.size(); i++) {
			circle(src, corners[i], 3, Scalar(0, 0, 255), -1, 8, 0);
		}

		cornerSubPix(gray, corners, Size(30, 30), Size(-1, -1),
				TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
		for (int i = 0; i < corners.size(); i++) {
			circle(src, corners[i], 3, Scalar(0, 255, 0), -1, 8, 0);
			char ss[4];
			sprintf(ss, "%d", i);
			string s = ss;
			Point2f org(corners[i].x + 10, corners[i].y - 10);
			putText(src, s, org, CV_FONT_HERSHEY_PLAIN, 1,
					Scalar(255, 0, 255), 2, 8);
		}

		if (calib) {
			int ax = atoi(argv[optind++]);
			int ay = atoi(argv[optind++]);
			int bx = atoi(argv[optind++]);
			int by = atoi(argv[optind++]);
			int xstep = (bx - ax)/(icols-1);
			int ystep = (by - ay)/(irows-1);
			if (debug) {
				cout << "xstep: " << xstep << endl;
				cout << "ystep: " << ystep << endl;
			}
			vector<Point3f> world;
			for (int i = 0; i < irows; i++) {
				for (int j = 0; j < icols; j++) {
					world.push_back(Point3i(ax + (j * xstep), ay + (i * ystep), 0));
				}
			}

			vector<vector<Point3f> > objectPoints;
			objectPoints.push_back(world);
			vector<vector<Point2f> > imagePoints;
			imagePoints.push_back(corners);
			Mat cameraMatrix;
			Mat distCoeffs;
			vector<Mat> rvecs;
			vector<Mat> tvecs;
			double error = calibrateCamera(objectPoints, imagePoints,
					src.size(), cameraMatrix, distCoeffs, rvecs, tvecs);
			cout << "Calibration reprojection error: " << error << endl;
			FileStorage fs(outfile, FileStorage::WRITE);
			fs << "ReprojectionError" << error;
			Mat rr;
			Rodrigues(rvecs[0], rr);

			fs << "A" << cameraMatrix << "K" << distCoeffs
					<< "R" << rr << "T" << tvecs[0];

			Mat rt(rr);
			Mat c = rt.col(2);
			tvecs[0].copyTo(c);

			fs << "RT" << rt;

			Mat h = cameraMatrix * rt;
			h = h.inv();

			fs << "H" << h;

			fs.release();
			cout << "Created calibration file: " << outfile << endl;

			if (debug) {
				cout << "Reprojection of points:" << endl;
				Mat calib = loadCalib(outfile);
				float wx, wy;
				for (int i = 0; i < corners.size(); i++) {
					toWorld(calib, corners[i].x, corners[i].y, &wx, &wy);
					cout << i << ": " << world[i].x << "," << world[i].y << " => " << wx << "," << wy << endl;
				}
			}
		}

		namedWindow("img", CV_WINDOW_NORMAL | CV_WINDOW_KEEPRATIO | CV_GUI_EXPANDED);
		imshow("img", src);

		cout << "Press any key to dismiss window..." << endl;
		sleep(2);
		waitKey(0);

	} else {
		cout << "Could not detect corners." << endl;
		return 1;
	}
	return 0;

}


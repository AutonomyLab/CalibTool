#include <unistd.h>
#include <iostream>
#include <time.h>
#include "opencv2/highgui/highgui.hpp"
#include "calib.hpp"
#include "track.hpp"

using namespace std;
using namespace cv;

void help() {
	cout << "Usage: trackconf [option]* -t ..." << endl
	     << "Description:" << endl
	     << "  Streams the location of circular objects within the video feed." << endl
	     << "Params:" << endl
	     << "  -t <file>    configuration file produced by trackerconf" << endl
	     << "Options:" << endl
	     << "  -c <calib>   camera calibration file to convert to world coords" << endl
	     << "  -d           enable debugging output" << endl
	     << "  -f <fps>     max framerate at which camera is scanned (default 20)" << endl
	     << "  -v <num>     video input device number (default 0)" << endl
;
}

void toWorld(Mat m, vector<Vec3f> pixelCircles) {
    for (int i = 0; i < pixelCircles.size(); i++) {
        calib::toWorld(m, pixelCircles[i][0], pixelCircles[i][1], &pixelCircles[i][0], &pixelCircles[i][1]);
    }
}

long millis(timespec ts) {
	return ts.tv_sec*1000+ts.tv_nsec/1000000;
}

int main(int argc, char** argv) {
	bool debug = false;
	int fps = 20;
	int cam = 0;
	bool hasTrack = false;
	string trackfile;
	bool hasCalib = false;
	string calibfile;
	int c;
	while ((c = getopt(argc, argv, "dhv:t:c:f:")) != -1) {
		switch (c){
		case 'd':
			debug = true;
			break;
		case 'h':
			help();
			return 0;
		case 'f':
			fps = atoi(optarg);
			break;
		case 'v':
			cam = atoi(optarg);
			break;
		case 't':
			hasTrack = true;
			trackfile = string(optarg);
			break;
		case 'c':
			hasCalib = true;
			calibfile = string(optarg);
			break;
		case '?':
			cout << "Invalid arguments." << endl << endl;
			help();
			return 1;
		}
	}

	if (!hasTrack) {
		cout << "No tracker configuration file specified." << endl;
		help();
		return -1;
	}

	int height = 0;
	int width = 0;
	int blurSize = 0;
	double blurSigma = 0.0;
	int minRadius = 0;
	int maxRadius = 0;
	double cannyThresh = 0.0;
	double accThresh = 0.0;

	FileStorage fs(trackfile, FileStorage::READ);
	fs ["ImageHeightPx"] >> height;
	fs ["ImageWidthPx"] >> width;
	fs["BlurSize"] >> blurSize;
	fs["BlurSigma"] >> blurSigma;
	fs["CannyThreshold"] >> cannyThresh;
	fs["MinRadius"] >> minRadius;
	fs["MaxRadius"] >> maxRadius;
	fs["AccumulatorThreshold"] >> accThresh;

    VideoCapture cap(cam);

    if (!cap.isOpened()) // check if we succeeded
    {
        cout << "Cannot initialize video capturing" << endl << endl;
        return -1;
    }
    cap.set(CV_CAP_PROP_FRAME_WIDTH, width);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, height);

    int period = 1000/fps;

    Mat src;
    Mat convert;
    if (hasCalib) {
    	convert = calib::loadCalib(calibfile);
    }
    vector<Vec3f> circles;
	timespec before, after;
	long mbefore, mafter;

    while (cap.read(src)) {
    	clock_gettime(CLOCK_REALTIME, &before);
     	mbefore = millis(before);

    	track::detectCircles(src, &circles, blurSize, blurSigma, minRadius, maxRadius, cannyThresh, accThresh);
    	if (circles.size() > 0){
			cout << mbefore;
			for (int i = 0; i < circles.size(); i++) {
				float x = circles[i][0];
				float y = circles[i][1];
				if (hasCalib)
					calib::toWorld(convert, x, y, &x, &y);
				cout << " " << x << " " << y;
			}
			cout << endl;
    	}
    	clock_gettime(CLOCK_REALTIME, &after);
    	mafter = millis(after);

    	long diff = mafter - mbefore;
        if (diff < period)
        	usleep((period - diff)*1000);
    }
}

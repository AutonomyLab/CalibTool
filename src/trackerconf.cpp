#include <iostream>
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <unistd.h>
#include "track.hpp"

using namespace std;
using namespace cv;

int mode = 0;
int blurSize = 9;
int blurSigma = 20;
int cannyThresh = 100;
int accThresh = 27;
int minRadius = 60;
int maxRadius = 80;


void processImage(Mat img){
	vector<Vec3f> circles;

    double t = (double) getTickCount();

    track::detectCircles(img, &circles, blurSize, blurSigma, minRadius, maxRadius, cannyThresh, accThresh);

    t = ((double) getTickCount() - t) / getTickFrequency() * 1000;

    Mat draw;
    switch(mode){
    case 0:
        draw = img;
        break;
    case 1:
    	track::blur(&img, blurSize, ((double) blurSigma) / 10.0);
        cvtColor(img, draw, CV_GRAY2RGB);
        break;
    case 2:
    	track::blur(&img, blurSize, ((double) blurSigma) / 10.0);
        track::canny(&img, cannyThresh);
        cvtColor(img, draw, CV_GRAY2RGB);
        break;
    }


    /// Draw the circles detected
    for (size_t i = 0; i < circles.size(); i++) {
        Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
        int radius = cvRound(circles[i][2]);
        // circle center
        circle(draw, center, 3, Scalar(0, 255, 0), -1, 8, 0);
        // circle outline
        circle(draw, center, radius, Scalar(0, 0, 255), 3, 8, 0);
    }

    Point org(50, 150);
    std::stringstream sstm;
    sstm << t << "ms";

    putText(draw, sstm.str(), org, FONT_HERSHEY_SIMPLEX, 3,
            Scalar(255, 255, 255), 5, 8);
    imshow("Circles", draw);
}

void onChange(int, void *img) {
    processImage(*(Mat*)img);
}

bool hasEnding (string const &fullString, string const &ending)
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

void help() {
	cout << "Usage: trackconf [option]* " << endl
	     << "Description:" << endl
	     << "  Generates a configuration file used by track to detect circular" << endl
	     << "  objects (e.g., chatterboxes)." << endl
	     << "Options:" << endl
	     << "  -d:          enable debugging output" << endl
	     << "  -h <height>: the pixel height of the input images (default 1200)" << endl
	     << "  -o <file>:   output the configuration to file (must end in \".yaml\")" << endl
	     << "  -v <num>:    video input device number (default 0)" << endl
	     << "  -w <width>:  the pixel width of the input images (default 1600)" << endl
;
}

int main(int argc, char** argv) {

	bool debug = false;
	int cam = 0;
	int height = 1200;
	int width = 1600;
	bool out = false;
	string outfile;
	int c;
	while ((c = getopt(argc, argv, "dhy:x:v:o:")) != -1) {
		switch (c){
		case 'd':
			debug = true;
			break;
		case 'h':
			help();
			return 0;
		case 'y':
			height = atoi(optarg);
			break;
		case 'x':
			width = atoi(optarg);
			break;
		case 'v':
			cam = atoi(optarg);
			break;
		case 'o':
			out = true;
			outfile = string(optarg);
			if (!hasEnding(outfile, ".yaml")) {
				cout << "Invalid output file name: " << outfile << endl << endl;
				help();
				return 1;
			}
			break;
		case '?':
			cout << "Invalid arguments." << endl << endl;
			help();
			return 1;
		}
	}

    VideoCapture cap(cam);

    if (!cap.isOpened()) // check if we succeeded
    {
        cout << "Cannot initialize video capturing" << endl << endl;
        return -1;
    }
    cap.set(CV_CAP_PROP_FRAME_WIDTH, width);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, height);
    cout << "Capture from camera" << endl;

    namedWindow("Circles", CV_WINDOW_NORMAL | CV_WINDOW_KEEPRATIO | CV_GUI_EXPANDED);

    Mat src;

    createTrackbar("Display mode", "Circles", &mode, 2, onChange, &src);
    createTrackbar("Blur Size", "Circles", &blurSize, 20, onChange, &src);
    createTrackbar("Blur Sigma", "Circles", &blurSigma, 100, onChange, &src);
    createTrackbar("Canny Thresh", "Circles", &cannyThresh, 500, onChange, &src);
    createTrackbar("Min Radius", "Circles", &minRadius, 500, onChange, &src);
    createTrackbar("Max Radius", "Circles", &maxRadius, 500, onChange, &src);
    createTrackbar("Acc Thresh", "Circles", &accThresh, 200, onChange, &src);

    int fps = 20;
    int period = 1000/fps;
    while (cap.read(src)) {
        processImage(src);
        if (waitKey(period) == 'q')
            break;
    }

    if (out) {
    	cout << "Writing settings to " << outfile << endl;
		FileStorage fs(outfile, FileStorage::WRITE);
		fs << "ImageHeightPx" << height;
		fs << "ImageWidthPx" << width;
		fs << "BlurSize" << blurSize;
		fs << "BlurSigma" << (((double) blurSigma) / 10.0);
		fs << "CannyThreshold" << cannyThresh;
		fs << "MinRadius" << minRadius;
		fs << "MaxRadius" << maxRadius;
		fs << "AccumulatorThreshold" << accThresh;
		fs.release();
    } else {
    	cout << "ImageHeightPx" << height << endl;
    	cout << "ImageWidthPx" << width << endl;
		cout << "BlurSize" << blurSize << endl;
		cout << "BlurSigma" << (((double) blurSigma) / 10.0) << endl;
		cout << "CannyThreshold" << cannyThresh << endl;
		cout << "MinRadius" << minRadius << endl;
		cout << "MaxRadius" << maxRadius << endl;
		cout << "AccumulatorThreshold" << accThresh << endl;
    }

    return 0;
}

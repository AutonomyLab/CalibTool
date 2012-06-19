#include "opencv2/core/core.hpp"
#include "calib.hpp"

Mat loadCalib(string filename) {
	FileStorage fs(filename, FileStorage::READ);
	Mat h;
	fs["H"] >> h;
	return h;
}

void toWorld(Mat calib, float pixelX, float pixelY, float *worldX, float *worldY) {
	double pixel[3] = {pixelX, pixelY, 1.0};
	Mat p(3, 1, CV_64F, pixel);
	Mat u = calib * p;
	*worldX = u.at<double>(0)/u.at<double>(2);
	*worldY = u.at<double>(1)/u.at<double>(2);
}

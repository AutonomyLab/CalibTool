#include "opencv2/imgproc/imgproc.hpp"
#include "track.hpp"

using namespace std;
using namespace cv;

namespace track {

void blur(Mat *img, int size, double sigma) {
    if (size % 2 == 0)
    	size++;
    cvtColor(*img, *img, CV_RGB2GRAY);
    GaussianBlur(*img, *img, Size(size, size),sigma);
}

void canny(Mat *img, double threshold) {
	Canny(*img, *img, MAX(threshold/2,1), threshold, 3); // taken from hough.cpp:822
}

void detectCircles(Mat img, vector<Vec3f> *circles, int blurSize, double blurSigma,
		           int minRadius, int maxRadius, double cannyThresh, double accThresh) {
    circles->clear();
	blur(&img, blurSize, blurSigma);
    HoughCircles(img, *circles, CV_HOUGH_GRADIENT, 1, minRadius*2, cannyThresh,
            accThresh, minRadius, maxRadius);
}

bool isOccluded(Vec3f circle, int imgWidth, int imgHeight){
    float x = circle[0];
    float y = circle[1];
    float r = circle[2];
    return ((x < r)
            || (y < r)
            || (x + r > imgWidth)
            || (y + r > imgHeight));
}

void removeOccluded(vector<Vec3f> circles, int imgWidth, int imgHeight) {
    vector<Vec3f>::iterator it;
    // remove partially occluded circles
    for (it = circles.begin(); it != circles.end();){
        if (isOccluded(*it, imgWidth, imgHeight)){
            it = circles.erase(it);
        }
        else{
            ++it;
        }
    }
}

}

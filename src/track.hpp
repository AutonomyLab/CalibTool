#ifndef TRACK_HPP_
#define TRACK_HPP_


#include "opencv2/core/core.hpp"
using namespace cv;

namespace track {

/*
 * Converts a source image to a blurred greyscale image.
 */
void grayblur(Mat *img, int size, double sigma);


/*
 * Converts a source image to show the Canny edges used by detectCircles.
 */
void canny(Mat *img, double threshold);

/*
 * Detects circles using the provided configuration parameters.
 */
void detectCircles(Mat img, vector<Vec3f> *circles, int blurSize, double blurSigma,
		           int minRadius, int maxRadius, double cannyThresh, double accThresh,
		           bool overlapping = true);

/*
 * Tests whether a circle is partially occluded.
 */
bool isOccluded(Vec3f circle);


/*
 * Removes all partially occluded circles.
 */
void removeOccluded(vector<Vec3f> circles);


}
#endif /* TRACK_HPP_ */

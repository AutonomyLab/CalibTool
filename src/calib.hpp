#ifndef CALIB_H_
#define CALIB_H_


#include "opencv2/core/core.hpp"
using namespace cv;

namespace calib {
/*
 * Loads a calibration matrix from a file created by gencalib.
 */
Mat loadCalib(string filename);


/*
 * Uses the calibration matrix to convert pixel coordinates into world coordinates.
 */
void toWorld(Mat calib, float pixelX, float pixelY, float *worldX, float *worldY);

}
#endif /* CALIB_H_ */

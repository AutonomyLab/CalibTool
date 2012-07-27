#include <unistd.h>
#include <iostream>
#include <time.h>
#include <iostream> // for stringstream

#include <hiredis/hiredis.h>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/objdetect/objdetect.hpp"
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
	     << "  -h           this help info" << endl
	     << "  -r           enable redis (127.0.0.1:6379)" << endl
	     << "  -u           enable ui" << endl
	     << "  -v <num>     video input device number (default 0)" << endl
;
}

void toWorld(Mat m, vector<Vec3f> pixelCircles) {
    for (int i = 0; i < pixelCircles.size(); i++) {
        calib::toWorld(m, pixelCircles[i][0], pixelCircles[i][1], &pixelCircles[i][0], &pixelCircles[i][1]);
    }
}

long millis(timespec ts) 
{
  return ts.tv_sec*1000+ts.tv_nsec/1000000;
}

int main(int argc, char** argv) 
{
  bool debug = false;
  bool ui = false;
  int fps = 20;
  int cam = 0;
  bool hasTrack = false;
  string trackfile;
  bool hasCalib = false;
  string calibfile;
  bool useRedis = false;

  int c;
  while ((c = getopt(argc, argv, "drhuv:t:c:f:")) != -1) {
    switch (c){
    case 'd':
      debug = true;
      break;
    case 'h':
      help();
      return 0;
    case 'u':
      ui = true;
      break;
    case 'r':
      useRedis = true;
      break;
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
  
  if (!hasTrack) 
    {
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
  
  string camname;
  if (ui) {
    cout << "UI enabled for video" << cam << endl;
    std::stringstream sstm;
    sstm << "video" << cam;
    camname = sstm.str();
    namedWindow(camname, CV_WINDOW_NORMAL | CV_WINDOW_KEEPRATIO | CV_GUI_EXPANDED);
    }
  
  // todo - make these command line options
  const char* redisHost = "127.0.0.1";
  const int redisPort = 6379;  
  redisContext *redisc = NULL;
  
  if( useRedis )
    {
      redisc = redisConnect( redisHost, redisPort );
      if( redisc->err )
	{
	  printf( "Redis connection error: %s\n", redisc->errstr);
	  exit(-1);
	}
    }
  
  while (cap.read(src)) {
    clock_gettime(CLOCK_REALTIME, &before);
     	mbefore = millis(before);
	
    	track::detectCircles(src, &circles, blurSize, blurSigma, minRadius, maxRadius, cannyThresh, accThresh, !debug);
	
#if 0	
    	if (circles.size() > 0){
	  cout << mbefore;
	  for (int i = 0; i < circles.size(); i++) {
	    float x = circles[i][0];
	    float y = circles[i][1];
	    
	    if (hasCalib)
	      calib::toWorld(convert, x, y, &x, &y);
	    cout << " " << (int)x << " " << (int)y;
	  }
	  cout << endl;
    	}
#endif
	
    	clock_gettime(CLOCK_REALTIME, &after);
    	mafter = millis(after);
	
    	long diff = mafter - mbefore;
	
	// convert the circles into bounding rectangles to use OpenCV's clustering routine
	vector<Rect> rects( circles.size() );
	for (size_t i = 0; i < circles.size(); i++) 
	  rects[i] = Rect( circles[i][0], circles[i][1], circles[i][2], circles[i][2] );

	// cluster the rectangles together - similar rectangles are averaged together
	const int min_group_size = 6;
	const double rect_relative_size = 0.4;       
	cv::groupRectangles( rects, min_group_size, rect_relative_size );
	
	if( useRedis )
	  {
	    // push each rectangle into Redis as a robot position estimate
	    // the output is a string of the x and y positions of each rectangle 
	    // separated by spaces ( "(x0 y0) (x1 y1) (y2 y2)"  )
	    std::stringstream str;
	    for (size_t i = 0; i < rects.size(); i++) 
	      {
		float x = rects[i].x;
		float y = rects[i].y;
		
		if (hasCalib)
		  calib::toWorld(convert, x, y, &x, &y);
		
		str << x << ' ' << y << ' ';
		
		std::stringstream key;
		key << "camera" << cam;
		
		void* reply = redisCommand( redisc, "SET %s %s", 
					    key.str().c_str(), str.str().c_str() );
		
		if( reply == NULL )
		  printf( "Redis error on SET: %s\n", redisc->errstr );
	      }
	  }

    	if (ui) {
	  /// Draw the circles detected
	  for (size_t i = 0; i < circles.size(); i++) {
	    Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
	    int radius = cvRound(circles[i][2]);
	    // circle center
	    //circle(src, center, 1, Scalar(0, 255, 0), -1, 8, 0);
	    // circle outline
	    //circle(src, center, radius, Scalar(0, 0, 255), 1, 8, 0);
	    
	    if (debug){
	      float x = circles[i][0];
	      float y = circles[i][1];
	      if (hasCalib)
		calib::toWorld(convert, x, y, &x, &y);
	      
	      int cx = ((center.x+radius+50)/50)*50;
	      int cy = ((center.y+50)/50)*50;
	      Point org(cx, cy);
	      std::stringstream sstm;
	      sstm << (int)x << "," << (int)y;
	      
	      putText(src, sstm.str(), org, CV_FONT_HERSHEY_PLAIN, 2,
		      Scalar(255, 0, 255), 2, 8);
	    }
	  }
	  /// Draw the rectangles detected
	  for (size_t i = 0; i < rects.size(); i++) 
	    {
	      rectangle( src, 
			 Point( cvRound(rects[i].x - rects[i].width),
				cvRound(rects[i].y - rects[i].width) ),
			 Point( cvRound(rects[i].x + rects[i].height),
				cvRound(rects[i].y  + rects[i].height) ),
			 Scalar( 255,0,255 ), 3, 8, 0 );			 
	      
	      // rectangle center
	      circle( src, 
		      Point( cvRound(rects[i].x), cvRound(rects[i].y) ),
		      5,  Scalar(255, 0, 255), -1, 8, 0);
	    }
	
    	    std::stringstream pfps;
    	    pfps << (int)(1000/diff) << " proc fps";

        	clock_gettime(CLOCK_REALTIME, &after);
        	mafter = millis(after);
        	diff = mafter - mbefore;
        	std::stringstream ufps;
        	ufps << (int)(1000/diff) << " ui fps";

    	    putText(src, pfps.str(), Point(50, 150), CV_FONT_HERSHEY_PLAIN, 2,
    	            Scalar(255, 255, 255), 2, 8);
    	    putText(src, ufps.str(), Point(50, 200), CV_FONT_HERSHEY_PLAIN, 2,
    	            Scalar(255, 255, 255), 2, 8);

    	    imshow(camname, src);
    	}

        if (diff < period)
        	if (ui)
        		waitKey(period - diff);
        	else
        		usleep((period - diff)*1000);
        else if (ui)
    		waitKey(1);
    }
}

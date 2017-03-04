#include <cv.hpp>
#include <highgui.h>
#include <iostream>
#include "opencv2/imgproc.hpp"
#include <opencv2/highgui.hpp>
#include "opencv2/highgui/highgui_c.h"
#include <math.h>
#include <errno.h>

#define USE_NETWORK
#ifdef USE_NETWORK
#include "tcp_client.h"
#define PORT 5801
#define ROBOT_IP "roboRIO-1768-FRC.local"
//#define ROBOT_IP "10.17.68.22"
#endif

//#define ShowWindows

#define r640x480
//#define r1280x720
//#define r1920x1080

#define Blur
#define Dilate

#ifdef r1280x1720
#define WIDTH 1280
#define HEIGHT 720
#elif defined(r640x480)
#define WIDTH 640
#define HEIGHT 480
#elif defined(r1920x1080)
#define WIDTH 1920
#define HEIGHT 1080
#endif

//#define USE_HLS
#ifdef USE_HLS
#define HUE_LOW 60
#define HUE_HIGH 180
#define LUM_LOW 100
#define LUM_HIGH 255
#define SAT_LOW 28
#define SAT_HIGH 255
#else
#define RED_LOW 0
#define RED_HIGH 20
#define GREEN_LOW 150
#define GREEN_HIGH 255
#define BLUE_LOW 0
#define BLUE_HIGH 20
#endif



using namespace cv;
using namespace std;

#ifdef USE_RASPICAM
raspicam::RaspiCam_Cv capture;
#else
VideoCapture capture;
#endif

Mat getBWImage() {
  //Dilation
  const int dilationSize = 2;
  const Mat dilateElement = getStructuringElement(MORPH_RECT, Size(2*dilationSize + 1, 2*dilationSize + 1), Point(dilationSize, dilationSize));

#ifdef Blur
  //Blur
  const int kernelSize = 8*1+ 1;
#endif

#ifdef USE_HLS
  //HLS Thresholding
  const Scalar low = Scalar(HUE_LOW, LUM_LOW, SAT_LOW);
  const Scalar high = Scalar(HUE_HIGH, LUM_HIGH, SAT_HIGH);
#else
  //RGB Thresholding
  const Scalar low = Scalar(RED_LOW, GREEN_LOW, BLUE_LOW);
  const Scalar high = Scalar(RED_HIGH, GREEN_HIGH, BLUE_HIGH);
#endif


  Mat img;
  Mat imgFixed;
  Mat blurredImg;
  Mat imgThresh;
  Mat dilatedImg;
  Mat hls;
  
#ifdef USE_RASPICAM
  capture.grab();
  capture.retrieve(img);
#else
  capture >> img;
#endif

#ifdef ShowWindows
  imshow( "image", img);
#endif

  //Blur
#ifdef Blur
  GaussianBlur(img,blurredImg,Size(kernelSize,kernelSize), 1);
#else
  blurredImg = img;
#endif

#ifdef USE_HLS
  //HLS Threshold processing
  cvtColor(blurredImg, hls, COLOR_BGR2HLS);
  inRange(hls, low, high, imgThresh);
#else
  inRange(blurredImg, low, high, imgThresh);
#endif

#ifdef Dilate
  //Dilation
  dilate(imgThresh, dilatedImg,dilateElement);
#else
  dilatedImg = imgThresh;
#endif
#ifdef ShowWindows
  imshow("dilate", dilatedImg);
#endif
 return dilatedImg;
}

int main(int argc, char* argv[])
{
  if(argc != 2) {
    cout << "Wrong number of arguments. Needs a USB port number" << endl;
  }
  setNumThreads(0);

  cout << "Using OpenCV Version " << CV_MAJOR_VERSION << "." << CV_MINOR_VERSION << endl;

  cout << "Opening USB Camera" << endl;
  capture = VideoCapture(atoi(argv[1]));
  if(!capture.isOpened()) {
    cout << "Video Capture not opened" << endl;
    return -1;
  }

#ifdef USE_NETWORK
  tcp_client c;
  string host = ROBOT_IP;
  do {
    cout << "Trying to connect..." << endl;
  } while( !c.conn(host, PORT));
#endif

  //Contours
  const int minArea = 200;
  const int thresh = 200; //For edge detection
  Mat canny_output;
  vector<vector<Point> > contours;
  vector<Vec4i> hierarchy;
  const Scalar color = Scalar(255,255,255);

  while(1)
  {
    clock_t t = clock();
    Mat img = getBWImage();
    //Contours processing
    Canny(img, canny_output, thresh, thresh*2, 3 );
    findContours( canny_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0,0) );
    //Limit contours (area, perimeter, etc.)
    vector<vector<Point> > goodContours(0);
    vector<vector<Point> > contours_poly(contours.size());
    vector<Rect> boundRect(0);
    vector<Rect> goodRect(0);
    for(unsigned int i = 0; i < contours.size(); i++)
    {
      approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
      boundRect.push_back(boundingRect(Mat(contours_poly[i])));
      if(boundRect[i].width * boundRect[i].height > minArea){
        goodContours.push_back(contours[i]);
        goodRect.push_back(boundRect[i]);
      }
    }

#ifdef ShowWindows
    //Draw contours
    Mat drawing = Mat::zeros(canny_output.size(), CV_8UC3 );
    for(unsigned int i = 0; i < goodContours.size(); i++)
    {
     rectangle(drawing, goodRect[i].tl(), goodRect[i].br(), color, 2,8,0);
      drawContours(drawing, goodContours, i, color, 2,8,hierarchy, 0, Point() );
    }
    imshow("contours.jpg", drawing);
#endif

    vector<Rect> goodRectReal(0);

    cout << "Good rect size here " << goodRect.size() << endl;

    for(unsigned int i = 0; i < goodRect.size(); i++) {
        //cout << "Good rect " << i << " x: " << goodRect[i].x << " y: " << goodRect[i].y << endl;
        /*for(unsigned int j = i + 1; j < goodRect.size(); j++) {
            if((goodRect[i].x - 2) < goodRect[j].x && goodRect[j].x < (goodRect[j].x + 2) && (goodRect[i].y - 2) < goodRect[j].y && goodRect[j].y < (goodRect[i].y + 2)) {
                goodRectReal.push_back(goodRect[i]);
                break;
            }
        }*/
        //There isn't a duplicate of this remaining.in the rest of the array
            bool thisIsADuplicate = false;
            for(unsigned int k = 0; k < goodRectReal.size(); k++) {
                //cout << "x condition: " << ((goodRect[i].x - 4) < goodRectReal[k].x && goodRectReal[k].x < (goodRectReal[k].x + 4)) << endl;
                //cout << "y condition: " << ((goodRect[i].y - 4) < goodRectReal[k].y && goodRectReal[k].y < (goodRectReal[k].y + 4)) << endl;
                if((goodRect[i].x - 4) < goodRectReal[k].x && goodRectReal[k].x < (goodRect[i].x + 4) && (goodRect[i].y - 4) < goodRectReal[k].y && goodRectReal[k].y < (goodRect[i].y + 4)) {
                    thisIsADuplicate = true;
                    //cout << "dup of rect " << k << " x: " << goodRectReal[k].x << " y: " << goodRectReal[k].y << endl;
                    break;
                }
            }
            if(!thisIsADuplicate) {
                //cout << "This is not a duplicate" << endl;
                goodRectReal.push_back(goodRect[i]);
        }
    }

    //TODO: Determine which contour to use.
    //We'll assume there's only one contour for now, but this needs to be fixed.

    unsigned int corRectX = 0;
    unsigned int corRectWidth = 0;
    unsigned int corRectHeight = 0;
    unsigned int corRectY = 0;
    bool turnPositive = 0;


    if(goodRectReal.size() == 3) {

	unsigned int maxHeight = 0;
	unsigned int maxHeightIndex = 100; //Arbitrary value outside the possible range

	for(unsigned int i = 0; i < goodRectReal.size(); i++) {
	    if((unsigned int) (goodRectReal[i].height) > maxHeight) {
		maxHeight = goodRectReal[i].height;
	    	maxHeightIndex = i;
	    }
	}

	unsigned int bottomRect = 0;
	unsigned int bottomRectIndex = 10;

	for(unsigned int i = 0; i < goodRectReal.size(); i++) {
	    if(i != maxHeightIndex) {
		if((unsigned int) (goodRectReal[i].y) > bottomRect) {
		    bottomRect = goodRectReal[i].y;
		    bottomRectIndex = i;
		}
	    }
	}

	unsigned int topRectIndex = 10;

	for(unsigned int i = 0; i < goodRectReal.size(); i++) {
	    if(i != maxHeight && i != bottomRectIndex) {
		topRectIndex = i;
	    }
	}

	unsigned int newRectY = 0;
	unsigned int newRectX = 0;
	unsigned int newRectHeight = 0;
	unsigned int newRectWidth = 0;

	if(goodRectReal[topRectIndex].x < goodRectReal[bottomRectIndex].x) {
	    newRectX = (unsigned int)(goodRectReal[topRectIndex].x);
	    newRectWidth = (unsigned int)(goodRectReal[bottomRectIndex].x + goodRectReal[bottomRectIndex].width - goodRectReal[topRectIndex].x);
	} else {
	    newRectX = (unsigned int)(goodRectReal[bottomRectIndex].x);
	    newRectWidth = (unsigned int)(goodRectReal[topRectIndex].x + goodRectReal[topRectIndex].width - goodRectReal[bottomRectIndex].x);
	}
	newRectY = (unsigned int)(goodRectReal[topRectIndex].y);
	newRectHeight = (unsigned int)(goodRectReal[bottomRectIndex].height + goodRectReal[bottomRectIndex].y - goodRectReal[topRectIndex].y);

	//Your code using newRect values and goodRectReal[maxHeightIndex]
	// WARNING: COOL MATH AHEAD	corRectHeight = (goodRectReal[maxHeightIndex].width * ((goodRectReal[maxHeightIndex].y - newRectY) / (goodRectReal[maxHeightIndex].x - newRectX)) + (newRectWidth * tan(90 - atan((goodRectReal[maxHeightIndex].y - goodRectReal[maxHeightIndex].height - newRectY - newRectHeight) / (goodRectReal[maxHeightIndex].x - goodRectReal[maxHeightIndex].width - newRectX - newRectHeight)) + newRectHeight + goodRectReal[maxHeightIndex].height) / 2));
	corRectWidth = (newRectWidth + goodRectReal[maxHeightIndex].width) / 2;

	if(newRectX < (unsigned int)(goodRectReal[maxHeightIndex].x)) {
	    corRectX = newRectX;
	    corRectWidth = goodRectReal[maxHeightIndex].x + goodRectReal[maxHeightIndex].width - newRectX;
	} else {
	    corRectX = (unsigned int)(goodRectReal[maxHeightIndex].x);
	    corRectWidth = newRectX + newRectWidth - goodRectReal[maxHeightIndex].x;
	}

	corRectY = (unsigned int)(newRectY + goodRectReal[maxHeightIndex].y) / 2;


	if (newRectHeight * newRectWidth > (unsigned int)(goodRectReal[maxHeightIndex].height * goodRectReal[maxHeightIndex].width)) {
	    if (newRectX > (unsigned int)(goodRectReal[maxHeightIndex].x)) {
		turnPositive = 1;
	    } else {
		turnPositive = 0;
	    }
	} else {
	    if (newRectX > (unsigned int)(goodRectReal[maxHeightIndex].x)) {
		turnPositive = 0;
	    } else {
		turnPositive = 1;
	    }
	}

	unsigned int tempTop = 0;
	unsigned int tempBottom = 0;
	if(newRectHeight + newRectY > (unsigned int)(goodRectReal[maxHeightIndex].height + goodRectReal[maxHeightIndex].y)) {
	    tempBottom = newRectWidth + newRectX;
	} else {
	    tempBottom = (unsigned int)(goodRectReal[maxHeightIndex].height + goodRectReal[maxHeightIndex].y);
	}

	if(newRectY < (unsigned int)(goodRectReal[maxHeightIndex].y)) {
	    tempTop = newRectY;
	} else {
	    tempTop = (unsigned int)(goodRectReal[maxHeightIndex].y);
	}
	corRectHeight = tempBottom - tempTop;

    }

    else if(goodRectReal.size() == 2) {

	//Your code here using goodRectReal[0] and goodRectReal[1]
	    // It would have been cool if we used this... corRectHeight = (goodRectReal[1].width * ((goodRectReal[1].y - goodRectReal[0].y) / (goodRectReal[1].x - goodRectReal[0].x)) + (goodRectReal[0].width * tan(90 - atan((goodRectReal[1].y - goodRectReal[1].height - goodRectReal[0].y - goodRectReal[0].height) / (goodRectReal[1].x - goodRectReal[1].width - goodRectReal[0].x - goodRectReal[0].height)) + goodRectReal[0].height + goodRectReal[1].height) / 2));
	    //corRectHeight = (goodRectReal[0].width * ((goodRectReal[0].y - goodRectReal[1].y) / (goodRectReal[0].x - goodRectReal[1].x)) + (goodRectReal[1].width * tan(90 - atan((goodRectReal[0].y - goodRectReal[0].height - goodRectReal[1].y - goodRectReal[1].height) / (goodRectReal[0].x - goodRectReal[0].width - goodRectReal[1].x - goodRectReal[1].height)) + goodRectReal[1].height + goodRectReal[0].height) / 2));

	corRectHeight = (goodRectReal[0].height + goodRectReal[1].height) / 2;
	if(goodRectReal[0].x < goodRectReal[1].x) {
	    corRectX = (unsigned int)(goodRectReal[0].x);
	    corRectWidth = goodRectReal[1].x + goodRectReal[1].width - goodRectReal[0].x;
	} else {
	    corRectX = (unsigned int)(goodRectReal[1].x);
	    corRectWidth = goodRectReal[0].x + goodRectReal[0].width - goodRectReal[1].x;
	}

	unsigned int tempBottom = 0;
	unsigned int tempTop = 0;
	if(goodRectReal[0].height + goodRectReal[0].y > goodRectReal[1].height + goodRectReal[1].y) {
	    tempBottom = (unsigned int)(goodRectReal[0].height + goodRectReal[0].y);
	} else {
	    tempBottom = (unsigned int)(goodRectReal[1].height + goodRectReal[1].y);
	}

	if(goodRectReal[0].y < goodRectReal[1].y) {
	    tempTop = (unsigned int)(goodRectReal[0].y);
	} else {
	    tempTop = (unsigned int)(goodRectReal[1].y);
	}
	corRectHeight = tempBottom - tempTop;

	corRectY = (unsigned int)(goodRectReal[0].y + goodRectReal[1].y) / 2;

	if (goodRectReal[0].height * goodRectReal[0].width > goodRectReal[1].height * goodRectReal[1].width) {
            if ((unsigned int)goodRectReal[0].x > (unsigned int)goodRectReal[1].x) {
                turnPositive = 1;
            } else {
                turnPositive = 0;
            }
        } else {
            if (goodRectReal[0].x > goodRectReal[1].x) {
                turnPositive = 0;
            } else {
                turnPositive = 1;
            }
        }
    }

/*
    } else if(goodRectReal.size() == 1) {
	corRectHeight = goodRectReal[0].height;
	corRectX = goodRectReal[0].x;
    }
*/

    int distance = HEIGHT - corRectY;
    int angleToTurn = corRectX + (corRectWidth / 2) - WIDTH/2;
    double targetAngle;
    if (corRectHeight != 0) {
	targetAngle = (double)(corRectWidth) / (double)(corRectHeight);
    } else {
	targetAngle = 0;
    }
    if (!turnPositive) {
	targetAngle = -targetAngle;
    }

    cout << "Distance: \t" << distance << endl;
    cout << "Angle: \t\t" << angleToTurn << endl;
    cout << "goodRect Size: \t" << goodRectReal.size() << endl;
    cout << "Target Angle: \t" << targetAngle << endl;

    if(goodRectReal.size() > 1) {
        ////TODO: These are placeholder variables for testing.
        ////      We need to find the actual distance and angle here.
        //      These are in units of 16ths of an inch and hundreths of degrees
        //int distance = corRectHeight;
       // int angleToTurn = corRectX;
#ifdef USE_NETWORK
        c.send_actual_data('d', distance);
        c.send_actual_data('a', angleToTurn);
        c.send_actual_data('t', 1000*(clock() - t)/CLOCKS_PER_SEC);
#endif

    } else {
	c.send_actual_data('x', 0);
    }
    cout << CLOCKS_PER_SEC/(clock() - t) << "fps" << endl;
    //waitKey(1);
  }
  capture.release();
  return 0;
}

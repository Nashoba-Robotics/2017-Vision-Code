#include <cv.hpp>
#include <highgui.h>
#include <iostream>
#include "opencv2/imgproc.hpp"
#include <opencv2/highgui.hpp>
#include "opencv2/highgui/highgui_c.h"
#include <math.h>
#include <errno.h>
#include <pthread.h>

//#define USE_NETWORK
#ifdef USE_NETWORK
#include "tcp_client.h"
#define PORT 5800
#define ROBOT_IP "roboRIO-1768-FRC.local"
//#define ROBOT_IP "10.17.68.22"
#endif

//#define ShowWindows

#define r640x480
//#define r1280x720
//#define r1920x1080

#define Blur
//#define Dilate

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
#define RED_HIGH 100
#define GREEN_LOW 100
#define GREEN_HIGH 255
#define BLUE_LOW 0
#define BLUE_HIGH 100
#endif

using namespace cv;
using namespace std;

//Contours
const int minArea = 750;
const int thresh = 200; //For edge detection 
const Scalar color = Scalar(255,255,255);

VideoCapture capture;

Mat getBWImage(Mat img) {
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


  Mat imgFixed;
  Mat blurredImg;
  Mat imgThresh;
  Mat dilatedImg;
  Mat hls;
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

typedef struct DataStruct {
    unsigned int angle;
    unsigned int distance;
    unsigned int timestamp;
} Data;

Data processImage(Mat rawimg) {
    Mat canny_output;
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
 
    clock_t t = clock();
    Mat img = getBWImage(rawimg);
    cout << ((float) clock() - t)/CLOCKS_PER_SEC << "s" << endl;
    //Contours processing
    //Canny(img, canny_output, thresh, thresh*2, 3 );
    findContours( img /*canny_output*/, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0,0) );
    //Limit contours (area, perimeter, etc.)
    cout << ((float) clock() - t)/CLOCKS_PER_SEC << "s" << endl;
    vector<vector<Point> > goodContours(0);
    vector<vector<Point> > contours_poly(contours.size());
    vector<Rect> boundRect(0);
    vector<Rect> goodRect(0);
    for(unsigned int i = 0; i < contours.size(); i++)
    {
      approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
      boundRect.push_back(boundingRect(Mat(contours_poly[i])));
      if(boundRect[i].height * boundRect[i].width > minArea){
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
    imshow("Contours", drawing);
#endif
    //Determine which contour to use.
    //We'll assume there's only one contour for now, but this needs to be fixed.

    vector<Rect> goodRectReal(0);

    /*
    for(unsigned int i = 0; i < goodRect.size(); i++) {
        for(unsigned int j = i + 1; j < goodRect.size(); j++) {
            if((goodRect[i].x - 2) < goodRect[j].x && goodRect[j].x < (goodRect[j].x + 2) && (goodRect[i].y - 2) < goodRect[j].y && goodRect[j].y < (goodRect[i].y + 2)) {
                goodRectReal.push_back(goodRect[i]);
            }
        }
    }
    */
    

    for (unsigned int i = 0; i < goodRect.size(); i++) {
        goodRectReal.push_back(goodRect[i]);
    }

    unsigned int rectToUse1 = 100;
    unsigned int rectToUse2 = 100;
    unsigned int area1= 0;
    unsigned int area2 = 0;

    for(unsigned int i = 0; i < goodRectReal.size(); i++) {
        if((unsigned int)(goodRectReal[i].width * goodRectReal[i].height) > area1 || (unsigned int)(goodRectReal[i].width * goodRectReal[i].height) > area2) {
            if (area1 <  area2) {
                area1 = (unsigned int) (goodRectReal[i].width * goodRectReal[i].height);
                rectToUse1 = i;
            } else {
                area2 = (unsigned int) (goodRectReal[i].width * goodRectReal[i].height);
                rectToUse2 = i;
            }

        }
    }

    unsigned int rectToUse = 100;
    if(rectToUse1 != 100 && rectToUse2 != 100) {
        if ((unsigned int)(goodRectReal[rectToUse1].x) < (unsigned int)(goodRectReal[rectToUse2].x)) {
	    rectToUse = rectToUse1;
        } else {
            rectToUse = rectToUse2;
        }
    }

    cout << "goodRect Size:" << goodRectReal.size() << endl;
    if (goodRect.size() > 0) {
    	cout << "Height: \t" << goodRectReal[rectToUse].width << endl;
    	cout << "X: \t\t" << goodRectReal[rectToUse].x << endl;
    	cout << "Y: \t\t" << goodRectReal[rectToUse].y << endl;
    }

    for(unsigned int i = 0; i < goodRectReal.size(); i++)
    {
    	if(goodRectReal.size() > 1) {
           // cout << "New image: " << i << endl;
    	   // cout << "height: " << goodRect[i].width << endl;
    	   // cout << "width: \t" << goodRect[i].height << endl;
	   // cout << "X: \t" << goodRect[i].y << endl;
	   // cout << "Y: \t" << goodRect[i].x << endl;

        //TODO: These are placeholder variables for testing.
        //      We need to find the actual distance and angle here.
        //      These are in units of 16ths of an inch and hundreths of degrees 
        //int distance = goodRect[i].width;
        //int angleToTurn = (goodRect[i].y + goodRect[i].height/2) - HEIGHT/2;

       	   unsigned int distance = goodRectReal[rectToUse].x;
           unsigned int angle = goodRectReal[rectToUse].y + (goodRectReal[rectToUse].height / 2) - (HEIGHT / 2);
           unsigned int timestamp = (1000*clock() - t)/CLOCKS_PER_SEC;
   //     cout << "Distance: \t" << distance << endl;
   //     cout << "Angle: \t\t" << angleToTurn << endl;
        

    cout << ((float) clock() - t)/CLOCKS_PER_SEC << "s" << endl;
    cout << CLOCKS_PER_SEC/((float) clock() - t) << "fps" << endl;

    return (Data) { angle : angle, distance : distance, timestamp : timestamp };
      }
    }
 
    cout << ((float) clock() - t)/CLOCKS_PER_SEC << "s" << endl;
    cout << CLOCKS_PER_SEC/((float) clock() - t) << "fps" << endl;

   return (Data) {angle: 0, distance: 0, timestamp: 0};
} 

Mat crossThreadImageOne;

pthread_mutex_t crossThreadImageMutexOne;

// We have to have the unused parameter for threading to work
void* threadOneFunc(void *unused) {
 
#ifdef USE_NETWORK 
  tcp_client c;
  string host = ROBOT_IP; 
  do {
    cout << "Trying to connect..." << endl;
  } while( !c.conn(host, PORT));
#endif

  while(1) {

    pthread_mutex_lock(&crossThreadImageMutexOne);
    Mat rawimg = crossThreadImageOne;
    pthread_mutex_unlock(&crossThreadImageMutexOne);

    Data d = processImage(rawimg);


#ifdef USE_NETWORK
    c.send_actual_data('d', d.distance);
    c.send_actual_data('a', d.angle);
    c.send_actual_data('t', d.timestamp);
#endif 


   } 
}


Mat crossThreadImageTwo;

pthread_mutex_t crossThreadImageMutexTwo;

// We have to have the unused parameter for threading to work
void* threadTwoFunc(void *unused) {
 
#ifdef USE_NETWORK 
  tcp_client c;
  string host = ROBOT_IP; 
  do {
    cout << "Trying to connect..." << endl;
  } while( !c.conn(host, PORT + 2));
#endif

  while(1) {

    pthread_mutex_lock(&crossThreadImageMutexTwo);
    Mat rawimg = crossThreadImageTwo;
    pthread_mutex_unlock(&crossThreadImageMutexTwo);

    Data d = processImage(rawimg);


#ifdef USE_NETWORK
    c.send_actual_data('d', d.distance);
    c.send_actual_data('a', d.angle);
    c.send_actual_data('t', d.timestamp);
#endif 


   } 
}


int main(int argc, char* argv[])
{
  if(argc != 2) {
    cout << "Incorrect number of arguments. Needs a USB port number" << endl;
  }

  pthread_t threadOne;
  pthread_t threadTwo;

  cout << "Using OpenCV Version " << CV_MAJOR_VERSION << "." << CV_MINOR_VERSION << endl;



  cout << "Opening USB Camera" << endl;
  capture = VideoCapture(atoi(argv[1]));  
  if(!capture.isOpened()) {
    cout << "Video Capture not opened" << endl;
    return -1;
  }
  pthread_mutex_init(&crossThreadImageMutexOne, NULL);

  pthread_mutex_lock(&crossThreadImageMutexOne);

  pthread_create(&threadOne, NULL, threadOneFunc, NULL);

  pthread_mutex_init(&crossThreadImageMutexTwo, NULL);

  pthread_mutex_lock(&crossThreadImageMutexTwo);

  pthread_create(&threadTwo, NULL, threadTwoFunc, NULL);

  int firstTimeOne = 1;
  int firstTimeTwo = 1;

while(1)
  {
    Mat rawimg; 
    capture >> rawimg;    

    if(!firstTimeOne) {
        pthread_mutex_lock(&crossThreadImageMutexOne);
    } else {
        firstTimeOne = 0;
    }
  
   crossThreadImageOne = rawimg;

    pthread_mutex_unlock(&crossThreadImageMutexOne);

    capture >> rawimg;    

    if(!firstTimeTwo) {
        pthread_mutex_lock(&crossThreadImageMutexTwo);
    } else {
        firstTimeTwo = 0;
    }
  
   crossThreadImageTwo = rawimg;

    pthread_mutex_unlock(&crossThreadImageMutexTwo);
#ifdef ShowWindows 
    waitKey(1);
#endif
  }
  capture.release();
  return 0;
}

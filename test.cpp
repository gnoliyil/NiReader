#include <stdio.h>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/nonfree/nonfree.hpp"

int main(void){
    int key;
    CvCapture *capture;
    IplImage *frameImage;
    char windowNameCapture[] = "capture";

    capture = cvCaptureFromCAM(0);
    if(!capture){
    printf("there is no camera");
    return 0;
    }

    cvNamedWindow(windowNameCapture, CV_WINDOW_AUTOSIZE);

    while(1){
        frameImage = cvQueryFrame(capture);
        cvShowImage(windowNameCapture, frameImage);
        key = cvWaitKey(1);
        if(key == 'q'){
        break;
        }
    }
    cvReleaseCapture(&capture);
    cvDestroyWindow(windowNameCapture);

    return 0;
}
　　

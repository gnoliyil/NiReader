#include <openni.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <cstdio>
using namespace std;
using namespace openni; 

Device niDevice; 
VideoStream vs; 
PlaybackControl& pc = *niDevice.getPlaybackControl(); 
char outputDir[40]; 
static int totFrames = 0; 

class Listener : public VideoStream::NewFrameListener {
    void onNewFrame(VideoStream & vs)
    {
        totFrames ++; 
        if (totFrames > pc.getNumberOfFrames(vs)) {
            vs.removeNewFrameListener(this);
            return; 
        }

        VideoFrameRef vfr; 
        vs.readFrame(&vfr); 
        printf("ts = %d, width = %d, height = %d\n", vfr.getTimestamp(),  vfr.getWidth(), vfr.getHeight());
        
        int width = vfr.getWidth(), height = vfr.getHeight(); 
        cv::Mat colorArr[3]; 
        cv::Mat colorImage; 
        const RGB888Pixel *pPixel; 
        const RGB888Pixel *pImageRow; 
        char filename[40]; 

        pImageRow = (const RGB888Pixel*) vfr.getData(); 
        cv::Mat image = cv::Mat(height, width, CV_8UC3, (unsigned*)pImageRow);
        cv::cvtColor(image, image, CV_RGB2BGR);
        image.convertTo(image, -1, 1.2, 10);
        // cv::resize(image, image, cv::Size(320, 240)); 
        if (vs.getMirroringEnabled()) {
            cv::flip(image, image, 1); 
        }
        sprintf(filename, "%s/frame%04d.jpg", outputDir, totFrames); 
        cv::imwrite(filename, image); 
    }
}; 


int main(int argc, char ** argv)
{
    char deviceName[100]; 
    strcpy(deviceName, argv[1]);
    if (argc >= 2) {
        strcpy(outputDir, argv[2]); 
    } else {
        strcpy(outputDir, "output");
    }
    int err_code; 
    err_code = OpenNI::initialize(); 
    printf("errcode = %d\n", err_code); 
   
    err_code = niDevice.open(deviceName);
    printf("errcode = %d\n", err_code); 
    
    vs.create(niDevice, SENSOR_COLOR); 
    Listener lster; 
    vs.addNewFrameListener(&lster); 
    
    printf("frames = %d\n", pc.getNumberOfFrames(vs)); 
    vs.start();
    while (1) ;
    OpenNI::shutdown(); 
    return 0; 
}



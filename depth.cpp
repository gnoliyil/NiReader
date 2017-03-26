#include <openni.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <cstdio>
#include <deque>
#include <vector>
#include <map>
using namespace std;
using namespace openni; 

Device niDevice; 
VideoStream vs; 
PlaybackControl& pc = *niDevice.getPlaybackControl(); 
static int totFrames = 0;

deque< vector< DepthPixel > > averageQueue;  

void smoothFiltering(DepthPixel *pDepth, int width, int height)  {
    const int OUTER_RADIUS = 6, INNER_RADIUS = 4;
    const int OUTER_THRESH = 6, INNER_THRESH = 4;
        
    for (int r = OUTER_RADIUS; r < height - OUTER_RADIUS; r ++) {
        for (int c = OUTER_RADIUS ; c < width - OUTER_RADIUS; c ++) {
            int index = r * height + c; 
            if (pDepth[index] == 0) { // need smoothing
                int counterOuter = 0, counterInner = 0; 
                vector<pair<int, int> > filterArray; 
                for (int dx = -OUTER_RADIUS; dx <= OUTER_RADIUS; dx ++)
                    for (int dy = -OUTER_RADIUS; dy <= OUTER_RADIUS; dy ++) {
                        int index_dxdy = (r + dx) * height + (c + dy); 
                        if (pDepth[index_dxdy]) {
                            bool found = false; 
                            for (auto p = filterArray.begin(); p != filterArray.end(); p ++)
                                if (p->first == pDepth[index_dxdy]) {
                                    found = true; 
                                    p->second ++; 
                                    break; 
                                }
                            if (!found) {
                                filterArray.push_back(make_pair<int, int>(pDepth[index_dxdy], 1)); 
                            }

                            if (abs(dx) <= INNER_RADIUS || abs(dy) <= INNER_RADIUS) 
                                counterInner ++; 
                            else 
                                counterOuter ++; 
                        }
                    }

                if (counterInner >= INNER_THRESH || counterOuter >= OUTER_THRESH) {
                    int max_occ = 0, max_val = 0; 
                    for (int i = 0; i < filterArray.size(); i++) {
                        if (filterArray[i].second > max_occ) {
                            max_occ = filterArray[i].second; 
                            max_val = filterArray[i].first; 
                        }
                    }
                    pDepth[index] = max_val; 
                }
            }
        }
    }
}

void nearestFilling (cv::Mat &src, cv::Mat &dst) {
    cv::Mat tmp = src; 

    for (int r = 0; r < tmp.rows; r++) {
        //printf("%d\n", r);
        for (int c = 0; c < tmp.cols; c++) { 
            if (tmp.at<char>(r, c) == 0) { 
                for (int radius = 1; radius < 10; radius ++) {
                    for (int r_ = -radius; r_ <= radius; r_++) {
                        int c_ = radius - abs(r_);
                        //printf("r = %d c = %d\n", r, c); 
                        if (r + r_ >= 0 && r + r_ < tmp.rows && c + c_ >= 0 && c + c_ < tmp.cols
                                && tmp.at<char>(r + r_, c + c_)) {
                            tmp.at<char>(r,c) = tmp.at<char>(r + r_, c + c_);
                            goto found; 
                        }
                        
                        c_ = -c; 
                        if (r + r_ >= 0 && r + r_ < tmp.rows && c + c_ >= 0 && c + c_ < tmp.cols
                                && tmp.at<char>(r + r_, c + c_)) {
                            tmp.at<char>(r,c) = tmp.at<char>(r + r_, c + c_);
                            goto found;
                        }
                    }
                }
            }
found:  ; //printf("found\n");
        }
    }
    dst = tmp;
}

class Listener : public VideoStream::NewFrameListener {
    void onNewFrame(VideoStream & vs)
    {
        totFrames ++; 
        if (totFrames > pc.getNumberOfFrames(vs)) {
            vs.removeNewFrameListener(this);
            return; 
        }

        int width, height; 
        const DepthPixel *pDepth; 
        DepthPixel *pEdit; 
        char filename[40];

        VideoFrameRef vfr; 
        vs.readFrame(&vfr); 
        width = vfr.getWidth(), height = vfr.getHeight(); 
        printf("%d # width = %d, height = %d\n", vfr.getTimestamp(), vfr.getWidth(), vfr.getHeight());
        
        pDepth = (const DepthPixel *) vfr.getData();
        pEdit  = new DepthPixel [width * height]; 
        memcpy(pEdit, pDepth, width * height * sizeof(DepthPixel)); 
        
        for (int i = 0; i < width * height; i ++)
            pEdit[i] = (pEdit[i] / 10) * 10; 
        // smoothFiltering(pEdit, width, height); 

        for (int i = 0; i < width * height; i ++)
            pEdit[i] = pEdit[i] * 255.0 / 10000.0; 

        cv::Mat srcImage = cv::Mat(height, width, CV_16U, (unsigned*)pEdit);
        cv::Mat image, edges;
        srcImage.convertTo(image, CV_8U); 
        cv::resize(image, image, cv::Size(320, 240), 0, 0, cv::INTER_NEAREST); 
        if (vs.getMirroringEnabled()) {
            cv::flip(image, image, 1); 
            cv::flip(edges, edges, 1); 
        }
        nearestFilling (image, image); 
        // image.convertTo(image, -1, 2, 0);

        cv::medianBlur(image, image, 5);
        // cv::blur(image, edges, cv::Size(3,3) );
        // cv::Canny(edges, edges, 20, 60, 3) ;
        // cv::inRange(image, 0, 2, mask); 
        // cv::inpaint(image, mask, image, 5, cv::INPAINT_TELEA); 

        sprintf(filename, "depth/frame%04d.jpg", totFrames); 
        cv::imwrite(filename, image); // edges); 
        delete [] pEdit; 
    }
}; 


int main(int argc, char ** argv)
{
    char deviceName[100]; 
    strcpy(deviceName, argv[1]);
    int err_code; 
    err_code = OpenNI::initialize(); 
    printf("errcode = %d\n", err_code); 
   
    err_code = niDevice.open(deviceName);
    printf("errcode = %d\n", err_code); 
    
    vs.create(niDevice, SENSOR_DEPTH); 
    VideoMode vm = vs.getVideoMode(); 
    printf("pixel mode = %d\n", vm.getPixelFormat()); 
    
    Listener lster; 
    vs.addNewFrameListener(&lster); 
    
    printf("frames = %d\n", pc.getNumberOfFrames(vs)); 
    vs.start();
    while (1) ;
    OpenNI::shutdown(); 
    return 0; 
}



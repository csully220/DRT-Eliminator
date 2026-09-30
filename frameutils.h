#ifndef __FRAMEUTILS_H__
#define __FRAMEUTILS_H__

#include <iostream>
#include <string>
#include <vector>

#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "HSVFilterVals.h"

using namespace std;
using namespace cv;

class FrameUtils {

public:
    static cv::Mat NextVidFrame( cv::Mat&, VideoCapture& );

    static cv::Mat Morph(cv::Mat&);
    static cv::Mat ResizeForTouchscreen(cv::Mat&);

    // containers for circles (or features)
    //std::vector<cv::Vec3f> rawcircles;
    //std::vector<cv::Vec3f> circles;

    /*** Apply RGB/HSV conversion, do morph and blur and thresholding ***/
    static cv::Mat Threshold(cv::Mat&, int,int,int,int,int,int); // H Hi/Los, S Hi/Los, V Hi/Low
    static cv::Mat Threshold(cv::Mat&, HSVFilterVals); // H Hi/Los, S Hi/Los, V Hi/Low
    static cv::Mat Threshold(cv::Mat&, HSVFilterVals, HSVFilterVals);
    static cv::Mat CopyThresh(cv::Mat, int,int,int,int,int,int); // H Hi/Los, S Hi/Los, V Hi/Low
    static cv::Mat CopyThresh(cv::Mat, HSVFilterVals); // H Hi/Los, S Hi/Los, V Hi/Low
    static cv::Mat CopyThresh(cv::Mat, HSVFilterVals, HSVFilterVals);
    // TODO Haar Cascade - try black and white images

    // Detect Circles
    static std::vector<cv::Vec3f> DetectCircles(cv::Mat, bool, int, int, int, int, int, int);
    //void CropRoi(cv::Rect);
};
#endif

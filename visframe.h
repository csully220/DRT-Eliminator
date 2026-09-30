#ifndef __VIS_FRAME_H__
#define __VIS_FRAME_H__

#include <iostream>
#include <string>
#include <vector>

#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "HSVFilterVals.h"

class VisFrame {
private:

    cv::VideoCapture cap;
    void Morph();
    //void MorphRoi();
    void ResizeForTouchscreen();

public:

    //Constructors
    VisFrame(); 
    VisFrame(cv::Mat); 
    VisFrame(cv::VideoCapture); 

    // OpenCV Matrices
    cv::Mat imgRaw;  // Raw image - resize and convert
    cv::Mat imgThresh;
    cv::Mat imgDisp; // For display
    //cv::Mat imgHSV;
    //cv::Mat imgRoiTree;
    //cv::Mat imgRoiThresh;

    // containers for circles (or features)
    std::vector<cv::Vec3f> rawcircles;
    std::vector<cv::Vec3f> circles;

    /*** Refresh, crop, rotate, resize the image for display and for processing. ***/
    void LoadImage(std::string, bool, bool);
    void NextVidFrame(bool, bool);
    void NextCamFrame(bool, bool);
    // TODO Optional - User selects ROI to resize Matrix

    /*** Apply RGB/HSV conversion, do morph and blur and thresholding ***/
    void Threshold(bool, bool);
    void Threshold(bool, bool,int,int,int,int,int,int); // H Hi/Los, S Hi/Los, V Hi/Low
    void Threshold(bool, bool, HSVFilterVals); // H Hi/Los, S Hi/Los, V Hi/Low
    void Threshold(bool, bool, HSVFilterVals, HSVFilterVals);
    // TODO Haar Cascade - try black and white images

    // Detect Circles
    void DetectCircles(bool, int, int, int, int, int, int);
    void CropRoi(cv::Rect);

};

#endif

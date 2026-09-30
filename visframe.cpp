#include "visframe.h"

using namespace std;
using namespace cv;

VisFrame::VisFrame(VideoCapture cap) : cap(cap) {
    imgRaw = Mat(640,480,CV_8U);
    imgDisp = Mat(640,480,CV_8U);
    //imgHSV = Mat(640,480,CV_8U);
    imgThresh = Mat(640,480,CV_8U);
}

VisFrame::VisFrame() {
    imgRaw = Mat(640,480,CV_8U);
    imgDisp = Mat(640,480,CV_8U);
    //imgHSV = Mat(640,480,CV_8U);
    imgThresh = Mat(640,480,CV_8U);
}

VisFrame::VisFrame(Mat mat) {
    imgRaw = mat.clone();
    imgDisp = Mat(640,480,CV_8U);
    //imgHSV = Mat(640,480,CV_8U);
    imgThresh = Mat(640,480,CV_8U);
}

void VisFrame::DetectCircles( bool b_draw, int dp, int min_dist, int canny_edge, int center_det, int min_rad, int max_rad ) {
    rawcircles.clear();
    HoughCircles( imgThresh, rawcircles, HOUGH_GRADIENT, dp, min_dist, canny_edge, center_det, min_rad, max_rad );
    if (b_draw and rawcircles.size() > 0) {
        for( size_t i = 0; i < rawcircles.size(); i++ ) {
            Point center(cvRound(rawcircles[i][0]), cvRound(rawcircles[i][1]));
            int radius = cvRound(rawcircles[i][2]);
            circle( imgDisp, center, 3, Scalar(0,255,0), -1, 8, 0 );     // circle center
            circle( imgDisp, center, radius, Scalar(0,0,255), 3, 8, 0 ); // circle outline
        }
    }
}

void VisFrame::Threshold(bool morph, bool blur, HSVFilterVals hsv_wht, HSVFilterVals hsv_yel) {
    //cvtColor(imgRaw, imgHSV, COLOR_BGR2HSV);
    Mat imgThreshW(640,480, CV_8U);
    Mat imgThreshY(640,480, CV_8U);
    //Mat imgThreshG = Mat(640,480, CV_8U);
    // HSV white
    inRange(imgRaw, Scalar(hsv_wht.iLowH, hsv_wht.iLowS, hsv_wht.iLowV), Scalar(hsv_wht.iHighH, hsv_wht.iHighS, hsv_wht.iHighV), imgThreshW);
    // HSV yellow
    inRange(imgRaw, Scalar(hsv_yel.iLowH, hsv_yel.iLowS, hsv_yel.iLowV), Scalar(hsv_yel.iHighH, hsv_yel.iHighS, hsv_yel.iHighV), imgThreshY);
    imgThresh = imgThreshW | imgThreshY;
    // HSV green
    //inRange(imgRaw, Scalar(hsv_grn.iLowH, hsv_grn.iLowS, hsv_grn.iLowV), Scalar(hsv_grn.iHighH, hsv_grn.iHighS, hsv_grn.iHighV), imgThreshG);
    //imgThresh = imgThresh | imgThreshG;
    if(morph) Morph();
    if(blur) GaussianBlur( imgThresh,imgThresh, Size(9, 9), 2, 2 );
}

void VisFrame::Threshold(bool morph, bool blur, HSVFilterVals hsv_vals) {
    /*** Convert and Threshold ***/
    int iLowH  = hsv_vals.iLowH;
    int iHighH = hsv_vals.iHighH;
    int iLowS  = hsv_vals.iLowS;
    int iHighS = hsv_vals.iHighS;
    int iLowV  = hsv_vals.iLowV;
    int iHighV = hsv_vals.iHighV;

    inRange(imgRaw, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresh);

    if(morph) Morph();
    if(blur) GaussianBlur( imgThresh,imgThresh, Size(9, 9), 2, 2 );
}

void VisFrame::Threshold(bool morph, bool blur) {
    /*** Convert and Threshold ***/
    int iLowH  = 0;
    int iHighH = 40;
    int iLowS  = 0;
    int iHighS = 40;
    int iLowV  = 250;
    int iHighV = 255;
    inRange(imgRaw, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresh);

    if(morph) Morph();
    if(blur) GaussianBlur( imgThresh,imgThresh, Size(9, 9), 2, 2 );
}

void VisFrame::Threshold(bool morph, bool blur, int iLowH, int iHighH, int iLowS, int iHighS, int iLowV, int iHighV) {
    /*** Convert and Threshold ***/
    //Mat imgHSV;
    inRange(imgRaw, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresh);
    if(morph) Morph();
    if(blur) GaussianBlur( imgThresh,imgThresh, Size(9, 9), 2, 2 );
}

void VisFrame::Morph() {
    erode ( imgThresh, imgThresh, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    dilate( imgThresh, imgThresh, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    dilate( imgThresh, imgThresh, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    erode ( imgThresh, imgThresh, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
}

void VisFrame::NextVidFrame(bool crop, bool to_hsv) {
    if (cap.isOpened()) {
        if (!cap.read(imgRaw)) { // read a new frame from video
            cap.set( CAP_PROP_POS_AVI_RATIO, 0 );
            if (!cap.read(imgRaw)) {
                cout << "Failed to get next frame." << endl;
                exit(EXIT_FAILURE);
            }
        }
        if (crop) ResizeForTouchscreen();
        imgDisp = imgRaw.clone();
        if (to_hsv) cvtColor(imgRaw, imgRaw, COLOR_BGR2HSV);
    }
}

void VisFrame::NextCamFrame(bool crop, bool to_hsv) {
    if (cap.isOpened()) {
        if (!cap.read(imgRaw)) { // read a new frame from video
            cout << "Failed to get next frame." << endl;
            exit(EXIT_FAILURE);
        }
        // resize(imgRaw, imgDisp, Size(640, 480));
        if (crop) ResizeForTouchscreen();
        imgDisp = imgRaw.clone();
        if (to_hsv) cvtColor(imgRaw, imgRaw, COLOR_BGR2HSV);
    }
}

void VisFrame::ResizeForTouchscreen() {
    // Given a 640 x 480 frame
    if (imgRaw.size() == Size(640, 480)) {
        resize(imgRaw, imgRaw, Size(1067, 800));
        Rect2d cropTo(293, 0,  480, 800); 
        imgRaw = Mat(imgRaw, cropTo);
    } else if (imgRaw.size() == Size(480, 640)) {
    // Given a 480 x 640 frame
        resize(imgRaw, imgRaw, Size(600, 800));
        Rect2d cropTo(60, 0,  480, 800); 
        Mat imgCropSides(imgRaw, cropTo);
        imgCropSides.copyTo(imgRaw);
        //resize(imgCropSides, imgRaw, Size(480, 800));
    } 
}

void VisFrame::LoadImage(string s_fn, bool crop, bool to_hsv) {
    imgRaw = imread(s_fn);
    if (imgRaw.empty()) {
        cout << "Could not open or find the image" << endl;
        exit(EXIT_FAILURE);
        return;
    }
    if (crop) ResizeForTouchscreen();
    //    resize(imgRaw, imgRaw, Size(640, 480));
    //    Rect2d cropTo(128, 0,  384, 460);
    //    Mat imgCropSides(imgRaw, cropTo);
    //    resize(imgCropSides, imgDisp, Size(480, 800));
    //}
    //imgDisp = imgRaw.clone();
    imgRaw.copyTo(imgDisp);
    if (to_hsv) cvtColor(imgRaw, imgRaw, COLOR_BGR2HSV);
    //Mat imgTmp;
}

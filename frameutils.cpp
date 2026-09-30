#include "frameutils.h"

using namespace std;
using namespace cv;

Mat FrameUtils::NextVidFrame( Mat& img, VideoCapture& cap ) {
    if (cap.isOpened()) {
        if (!cap.read(img)) { // read a new frame from video
            cap.set( CAP_PROP_POS_AVI_RATIO, 0 );
            if (!cap.read(img)) {
                cout << "Failed to get next frame." << endl;
                exit(EXIT_FAILURE);
            }
        }
    }
    return img;
}

vector<Vec3f> FrameUtils::DetectCircles( Mat img, bool b_draw, int dp, int min_dist, int canny_edge, int center_det, int min_rad, int max_rad ) {
    vector<Vec3f> rawcircles;
    HoughCircles( img, rawcircles, HOUGH_GRADIENT, dp, min_dist, canny_edge, center_det, min_rad, max_rad );
    /*if (b_draw and rawcircles.size() > 0) {
        for( size_t i = 0; i < rawcircles.size(); i++ ) {
            Point center(cvRound(rawcircles[i][0]), cvRound(rawcircles[i][1]));
            int radius = cvRound(rawcircles[i][2]);
            circle( imgDisp, center, 3, Scalar(0,255,0), -1, 8, 0 );     // circle center
            circle( imgDisp, center, radius, Scalar(0,0,255), 3, 8, 0 ); // circle outline
        }
    }*/
    return rawcircles;
}

Mat FrameUtils::CopyThresh( Mat img, HSVFilterVals hsv_wht, HSVFilterVals hsv_yel ) {
    cvtColor(img, img, COLOR_BGR2HSV);
    Mat imgThreshW(640,480, CV_8U);
    Mat imgThreshY(640,480, CV_8U);
    // HSV white
    inRange(img, Scalar(hsv_wht.iLowH, hsv_wht.iLowS, hsv_wht.iLowV), Scalar(hsv_wht.iHighH, hsv_wht.iHighS, hsv_wht.iHighV), imgThreshW);
    // HSV yellow
    inRange(img, Scalar(hsv_yel.iLowH, hsv_yel.iLowS, hsv_yel.iLowV), Scalar(hsv_yel.iHighH, hsv_yel.iHighS, hsv_yel.iHighV), imgThreshY);
    img = imgThreshW | imgThreshY;
    return img;
}

Mat FrameUtils::CopyThresh( Mat img, HSVFilterVals hsv_vals ) {
    cvtColor(img, img, COLOR_BGR2HSV);
    int iLowH  = hsv_vals.iLowH;
    int iHighH = hsv_vals.iHighH;
    int iLowS  = hsv_vals.iLowS;
    int iHighS = hsv_vals.iHighS;
    int iLowV  = hsv_vals.iLowV;
    int iHighV = hsv_vals.iHighV;
    inRange(img, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), img);
    return img;
}

Mat FrameUtils::CopyThresh( Mat img, int iLowH, int iHighH, int iLowS, int iHighS, int iLowV, int iHighV )  {
    cvtColor(img, img, COLOR_BGR2HSV);
    inRange(img, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), img);
    return img;
}

Mat FrameUtils::Threshold( Mat& img, HSVFilterVals hsv_wht, HSVFilterVals hsv_yel ) {
    cvtColor(img, img, COLOR_BGR2HSV);
    Mat imgThreshW(640,480, CV_8U);
    Mat imgThreshY(640,480, CV_8U);
    // HSV white
    inRange(img, Scalar(hsv_wht.iLowH, hsv_wht.iLowS, hsv_wht.iLowV), Scalar(hsv_wht.iHighH, hsv_wht.iHighS, hsv_wht.iHighV), imgThreshW);
    // HSV yellow
    inRange(img, Scalar(hsv_yel.iLowH, hsv_yel.iLowS, hsv_yel.iLowV), Scalar(hsv_yel.iHighH, hsv_yel.iHighS, hsv_yel.iHighV), imgThreshY);
    img = imgThreshW | imgThreshY;
    return img;
}

Mat FrameUtils::Threshold( Mat& img, HSVFilterVals hsv_vals ) {
    cvtColor(img, img, COLOR_BGR2HSV);
    int iLowH  = hsv_vals.iLowH;
    int iHighH = hsv_vals.iHighH;
    int iLowS  = hsv_vals.iLowS;
    int iHighS = hsv_vals.iHighS;
    int iLowV  = hsv_vals.iLowV;
    int iHighV = hsv_vals.iHighV;
    inRange(img, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), img);
    return img;
}

Mat FrameUtils::Threshold( Mat& img, int iLowH, int iHighH, int iLowS, int iHighS, int iLowV, int iHighV )  {
    cvtColor(img, img, COLOR_BGR2HSV);
    inRange(img, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), img);
    return img;
}

Mat FrameUtils::Morph( Mat& img ) {
    erode ( img, img, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    dilate( img, img, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    dilate( img, img, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    erode ( img, img, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    GaussianBlur( img,img, Size(9, 9), 2, 2 );
    return img;
}

// pass by reference
Mat FrameUtils::ResizeForTouchscreen( Mat& img ) {
    // Given a 640 x 480 frame
    if (img.size() == Size(480, 800)) {
        ;
    } else if (img.size() == Size(640, 480)) {
        resize(img, img, Size(1067, 800));
        Rect2d cropTo(293, 0,  480, 800); 
        img = Mat(img, cropTo);
    } else if (img.size() == Size(480, 640)) {
    // Given a 480 x 640 frame
        resize(img, img, Size(600, 800));
        Rect2d cropTo(60, 0,  480, 800); 
        Mat img(img, cropTo);
        resize(img, img, Size(480, 800));
    } else throw std::runtime_error("ResizeForTouchscreen got an unsupported resolution");
    return img;
}

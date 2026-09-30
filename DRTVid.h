#ifndef __DRTVID_H__
#define __DRTVID_H__

// STL
#include <string>
#include <ctime>
#include <chrono>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <cmath>
#include <experimental/filesystem>
// OpenCV
#include <opencv2/tracking.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "opencv2/objdetect.hpp"
// Custom
#include "defines.h"
#include "INIReader.h"
#include "cvui.h"
#include "xmas_tree.h"
#include "log.h"
#include "visframe.h"
#include "patdet.h"
#include "frameutils.h"

//Utilities
#ifndef VM
#include <wiringPi.h>
#endif

class DRTVid {

private:
    enum COLOR { WHITE, YELLOW, GREEN, RED }; // TODO cleanup remove
    std::string s_track_dir = "";
    std::string treeprofile;
    int occlusion_timeout = 60; // num frames before tracker times out
    XmasTree::ZONE trackzones[7] = { XmasTree::ZONE::PRESTAGE, 
                                     XmasTree::ZONE::STAGE, 
                                     XmasTree::ZONE::ONE, 
                                     XmasTree::ZONE::TWO,
                                     XmasTree::ZONE::THREE, 
                                     XmasTree::ZONE::GO, 
                                     XmasTree::ZONE::FOUL };
                                     XmasTree *p_tree;
    
    // Filtering and detection params
    //int iLowH, iHighH, iLowS, iHighS, iLowV, iHighV;

    // Matrices
    cv::VideoCapture cap;
    
    // ROIs
    cv::Rect2d roiPrimary, roiZone;
    std::vector<cv::Vec3f> circles; //TODO cleanup remove
    std::vector<cv::Vec3f> pslamps;
    std::vector<cv::Vec3f> stlamps;

    // Tracker
    cv::Ptr<cv::Tracker> tracker;
    cv::CascadeClassifier tree_cascade;
    std::vector<int> cir_x;
    
    // tuple<Vec3f, Vec3f> ps_left, ps_right, st_left, st_right;
    // std::vector<cv::Vec3f> persistent_circles;
    bool isStageLamps   ( std::vector<cv::Vec3f>, float, float, float );
    //cv::Rect detectLaneLamps ( std::vector<cv::Vec3f>, float );

    enum DETECTION_MODE { NONE, MANUAL, PATTERN, HAAR } det_mode;
    void DetModeCycle();
    std::string DetModeString();
    enum SEQUENCE { IDLE, SEARCHING, TRACKING } seq;

    XmasTree tree;
    static void on_trackbar( int pos, void* );
    static void mouseEvent(int evt, int x, int y, int flags, void* param);
public:
    DRTVid();
    DRTVid(std::string);
    ~DRTVid();
    
    /*** Init and config ***/
    void SetVideo(std::string);

    /*** Capture, crop, resize, filter ***/
    // Done with VisFrame

    /*** Tree detection ***/

    /*** Executive Routines ***/
    void RunVideoHaarDetect( std::string );
    void RunVideoDebug( void );
    void RunVideoDebug( std::string );
    void RunSelectCalImage( std::string );

    void ExecSimplePlayback( void );
    void ExecSimplePlayback( std::string );
};

#endif

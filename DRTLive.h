#ifndef __DRTLIVE_H__
#define __DRTLIVE_H__

// STL
#include <string>
#include <ctime>
#include <chrono>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <pthread.h>
#include <sys/mman.h> // mlockall
// OpenCV
#include <opencv2/tracking.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "opencv2/objdetect.hpp"
//#include <mutex>
// Custom
#include "xmas_tree.h"
//#include "log.h"
//Utilities

typedef std::chrono::time_point<std::chrono::high_resolution_clock> hiresclk;

class DRTLive {
    // The Real-Time thread
    struct sched_param param;
    pthread_attr_t attr;
    pthread_t thread;

    std::string s_track_dir = "";
    std::string treeprofile = "";

    char str[10];
    // num frames before tracker times out
    int occlusion_timeout = 60; 
    int key_val;

    XmasTree tree;
    // TIMESTAMPS 
    // start and end of frame
    hiresclk frm_start_t,
             frm_end_t;
    // time of lamp illumination
    hiresclk prestage_t,
             stage_t, 
             yel1_t, 
             yel2_t, 
             yel3_t,
             grn_t;
    // heat start and end
    hiresclk heat_start_t, 
             heat_end_t;

    // DURATIONS
    std::chrono::duration<double> frame_dt, // duration of current frame
                                  heat_dt,  // duration of heat
                                  error_dt; // difference between lamp illum time expected and actual
 
    double fps; // actual frames per second
    unsigned int frame_counter = 0; // holds the current frame for video playback

    // Video Capture object
    cv::VideoCapture cap;

    // Matrices
    cv::Mat imgRaw;       // holds raw image from camera
    cv::Mat imgDisp;      // display on screen - with overlay
    cv::Mat imgProc;      // filter, blur, morph, etc.
    cv::Mat imgTrk;       // tracked ROI - encompasses the whole tree
    cv::Mat imgMask;      // ROI for a specific lamp or pair of lamps
    cv::Scalar maskIllum; // illumination value of the imgMask ROI

    // ROIs/rectangles for finding imgTrk and imMask
    cv::Rect2d roiPrimary, roiZone; 
    // vectors to hold detected circles and lamps 
    std::vector<cv::Vec3f> rawcircles;
    std::vector<cv::Vec3f> circles, 
                           pslamps,
                           stlamps;

    // Tracker
    cv::Ptr<cv::Tracker> p_tracker;

    // Enums
    enum Zone { PRESTAGE, STAGE, YEL_1, YEL_2, YEL_3, GRN, RED } zone;
    //Modes 
    enum DetectionMode { NONE, MANUAL, PATTERN, HAAR } det_mode;
    enum Sequence { IDLE, SEARCHING, TRACKING } seq;

    // flags
    bool b_run;
    bool b_pause;
    bool b_halt_tracker;

    bool b_filtimg_disp;
    bool b_morph;
    bool b_blur;
    bool b_displamps; 
    bool b_manroi;
    bool b_trk_success;

    // Control window flags
    int m_debug_level;
    int m_crop_roi_when_tracking;
    int m_morph;
    int m_blur;
    int m_fps_disp;
    int m_frmctr;
    int m_displamps;
    int m_filtimg_disp;

    // Log file
    //FILE* pFile;

    public:
    DRTLive();
    DRTLive(std::string);
    ~DRTLive();

    int InitThread( void );
    int InitThread( bool, bool );

    int RTLive( void );
    int RTClockTest( void );
    int RTCameraTest( void );
    int RTMatTest( void );
    int RTClockShareData( void );
    int RTBump( double bump_duration, bool rev_pol );
};

#endif

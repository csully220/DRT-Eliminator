#ifndef __DRTCAL_H__
#define __DRTCAL_H__

// STL
#include <string>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <cmath>
// OpenCV
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
// Custom
#include "INIReader.h"
#include "cvui.h"
#include "xmas_tree.h"
#include "log.h"
#include "visframe.h"
#include "defines.h"

class DRTCal {

private:
    static void on_trackbar( int, void* );
    static void mouseEvent(int, int, int, int, void*);
    std::string s_track_dir = "";
    std::string s_image_dir = "";
    std::string treeprofile;
    std::vector<std::string> img_files;
    XmasTree tree;
    cv::VideoCapture cap;

public:
    // pass the path to the working directory
    DRTCal(std::string);
    
    // RunRecordCalVideo: Records a video
    // arg1: filename to save to
    void RunRecordCalVideo( std::string );

    // RunSelectCalImage: Save a frame from video
    // arg1: filename to save to
    void RunSelectCalImage( std::string ); 

    // RunImageCalibrate: Prompts user to generate calibration data and saves to file (default is tree.cfg)
    // arg1: calibrate geometry
    // arg2: calibrate color
    // arg3: image file
    // arg4: filename to save to
    void RunImageCalibrate( bool, bool );   
    void RunImageCalibrate( bool, bool, std::string );
    void RunImageCalibrate( bool, bool, std::string, std::string );

    void RunViewCalSettings( void );        // Preview calibration settings
    void RunViewCalSettings( std::string ); // Preview calibration settings, specify cal file
    void RunSetSensitivity( std::string ); 
    void RunSetSensitivity( void ); // Play video, select frames where yellow is just becoming visible. Use this to set the initial threshold. Can also just play the cal video, decrementing the threshold until we start getting false positives, indicated by Mat avg value dropping back below the threshold before it should, or lamp illuminating multiple times (can process video as slowly as we need to).
    void RunCameraSettings( void ); 
};

#endif

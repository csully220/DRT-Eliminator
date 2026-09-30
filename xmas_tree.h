#ifndef __XMASTREE_H__
#define __XMASTREE_H__

#include <opencv2/core.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include "INIReader.h"
#include "defines.h"
#include "log.h"

class XmasTree {
    // for optimization Arrange the member variables of classes and structures in such a way that the most used variables are in the first 128 bytes, and then sorted from the longest object to the shortest.

//private:

public:
    // Constructors
    XmasTree(std::string);
    XmasTree();
 
    enum ZONE { PRESTAGE, STAGE, ONE, TWO, THREE, GO, FOUL };

    //int iLowS, iHighS, iLowV, iHighV;
    int wLowH, wHighH, wLowS, wHighS, wLowV, wHighV; // HSV threshold values for staging lights
    int yLowH, yHighH, yLowS, yHighS, yLowV, yHighV; 
    int gLowH, gHighH, gLowS, gHighS, gLowV, gHighV;
    int rLowH, rHighH, rLowS, rHighS, rLowV, rHighV;
    int thresh_stg, thresh_yel;
    // circles
    int dp, min_dist, canny_edge, center_det, min_rad, max_rad;
    // Methods
    cv::Rect getZone(ZONE, cv::Rect);
    cv::Mat getZone(ZONE, cv::Mat);
    std::vector<cv::Point> getLampPoints();
    std::vector<cv::Point> getLampPoints(cv::Point, float);
    bool init(std::string); 
    void setIllum(ZONE, bool);
    bool getIllum(ZONE);
    bool saveToFile(std::string);

    // properties
    std::string name;
    std::string location;
    std::string cascadefile;

    // relative offsets from top left prestage lamp
    float prestage_y, stage_y, y1_y, y2_y, y3_y, grn_y, red_y;
    float st1_x, st2_x, st3_x, st4_x, column1_x, column2_x;
    float aspect_ratio;
    float relative_radius;

    float mask_ht;   
    // tree params used to display calibration settings
    int nominal_radius;
    int calimg_tl_x, calimg_tl_y; // top left lamp of cal img
    int calimg_w; // width of cal img 
    
    //BOTTOM OF ZONE MEASURED FROM TOP - DIMENSIONLESS * 100
    float prestage_top, prestage_bot, stage_top, stage_bot, yel1_top, yel1_bot, yel2_top, yel2_bot, yel3_top, yel3_bot, grn_top, grn_bot, red_top, red_bot;

    bool ps_illum = false;
    bool st_illum = false;
    bool y1_illum = false;
    bool y2_illum = false;
    bool y3_illum = false;
    bool gr_illum = false;
    bool rd_illum = false;

    //void Update(cv::Mat);
};

#endif //__XMASTREE_H__

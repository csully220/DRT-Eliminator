#ifndef __PATDET_H__
#define __PATDET_H__

#include <iostream>
#include <vector>

#include <opencv2/core.hpp>
#include <string>
//
// Pattern detector class - analyzes vectors of circles to find pattern matching a christmas tree
class PatDet {

public:

    //Constructors
    // containers for circles (or features)
    std::vector<cv::Vec3f> rawcircles;
    std::vector<cv::Vec3f> circles;
    std::vector<int> cir_x = { };

    // Helper functions
    static std::vector<cv::Vec3f> WithinRadius_X( cv::Vec3f, std::vector<cv::Vec3f>, float );
    static std::vector<cv::Vec3f> WithinRadius_Y( cv::Vec3f, std::vector<cv::Vec3f>&, float, bool );
    static cv::Vec3f              popMinX     ( std::vector<cv::Vec3f>& );
    static cv::Vec3f              popMinY     ( std::vector<cv::Vec3f>& );
    static cv::Vec3f              popMaxY     ( std::vector<cv::Vec3f>& );
    static std::vector<cv::Vec3f> sortByX     ( std::vector<cv::Vec3f>  );
    static std::vector<cv::Vec3f> sortByY     ( std::vector<cv::Vec3f>  );
    bool CompareCircles(cv::Vec3f, cv::Vec3f, float);
    bool SearchCircles(cv::Vec3f, std::vector<cv::Vec3f>, float);

    std::vector<cv::Vec3f> sortByX (  ); // sorts circles
    std::vector<cv::Vec3f> sortByY (  ); // sorts circles

    int  LampsCenterline(bool);

    void RemoveDupeCircles(float);
};

#endif

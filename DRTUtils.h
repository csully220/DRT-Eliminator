#ifndef __DRTCAM_H__
#define __DRTCAM_H__

// STL
#include <string>
#include <ctime>
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
#include "INIReader.h"
#include "cvui.h"
#include "xmas_tree.h"
#include "log.h"
#include "visframe.h"
#include "defines.h"
#include "frameutils.h"

class DRTUtils {

private:
    
public:
    DRTUtils();
    
    static void ShowCameraFeed( void );
};

#endif

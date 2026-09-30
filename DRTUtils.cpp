#include "DRTUtils.h"

#define CAMERA_MAX_RETRIES 20

//#define DRT_MONITOR  // uncomment to compile with support for full size monitor

using namespace cv;
using namespace std;
namespace fs = std::experimental::filesystem;
//using namespace fs;

DRTUtils::DRTUtils() {
    FILE* pFile = fopen("log/drt.log", "w");
    Output2FILE::Stream() = pFile;
    FILELog::ReportingLevel() = LOG_LEVEL;
    FILE_LOG(logINFO) << "DRTUtils opening log file. Using OpenCV version  " << CV_VERSION << endl;
}

void DRTUtils::ShowCameraFeed() {
    FILE* pFile = fopen("log/drt.log", "w");
    Output2FILE::Stream() = pFile;
    FILELog::ReportingLevel() = LOG_LEVEL;
    FILE_LOG(logINFO) << "DRTUtils->ShowCameraFeed";
    
    VideoCapture cap;

    int i = 0;
    while (!cap.open(i)) {
        i++;
        if ( i == CAMERA_MAX_RETRIES ) return;
    }
    //VisFrame frame(cap);
    cvui::init("Camera");
    moveWindow("Camera", 0, 0);

    Mat imgRaw;

    int frame_width = cap.get(CAP_PROP_FRAME_WIDTH); 
    int frame_height = cap.get(CAP_PROP_FRAME_HEIGHT); 
    int capfps = cap.get(CAP_PROP_FPS);
    FILE_LOG(logINFO) << "Opening camera in " << frame_width << " x " << frame_height << " mode at " << capfps << " fps";
    // Initialize timing and framerate variables
    clock_t start_t, end_t, total_t;
    double fps = 30.0;              // The actual framerate.
    unsigned int frame_counter = 0;

    chrono::time_point<std::chrono::high_resolution_clock> start;
    int milsleep = 1;
    char str[40];
    fps = 30.0;
    sprintf(str, "FPS  %.4f", fps);

    bool b_run = true;
    while(b_run) {
        start = std::chrono::high_resolution_clock::now();
        if (!cap.read(imgRaw)) {
            break;
        }
        FrameUtils::ResizeForTouchscreen(imgRaw);
        // Press  ESC on keyboard to  exit
            
        /*** Timing and framerate ***/

        //cout << "frame_dt " << elapsed.count() << endl;
        sprintf(str, "FPS  %.4f", fps);
        //cout << "fps " << str << endl;
        putText(imgRaw, str, Point(10, 20), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
        if (cvui::button(imgRaw, COL1, ROW2, BTN_WID, 20, string("Exit"))) {
           b_run = false; 
        }
        cvui::imshow( "Camera", imgRaw );
        // Limit the playback to no faster than 30 fps
        //if (elapsed.count() < 0.0333) {
            //usleep(33333 - (elapsed.count() * 1000000)); // 1000000 = one second
            //milsleep = (int)(333 - (elapsed.count() * 10000));
            //finish = std::chrono::high_resolution_clock::now();
            //elapsed = finish - start;
            //cout << milsleep << endl;
        //} else milsleep = 1;

        char c = (char)waitKey(8);
        if( c == 27 ) break;
        
        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;
        fps = 1.0/elapsed.count();
    }

    destroyWindow("Camera");
}

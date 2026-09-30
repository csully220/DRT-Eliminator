//#include "DRTLive.h"
//#include "DRTVid.h"
#include <chrono>
#include <unistd.h>

using namespace std;
using namespace cv;

struct thread_args {
    int first;
    const char *second;
};

//void *RTVidDebug( void *data ) {
void *RT_DebugVideo( void *data ) {
    //reinterpret_cast<DRTVid*>(data)->RunVideoDebug();
    //static_cast<DRTVid*>(data)->RunVideoDebug();
    //pdrtvid->RunVideoDebug();
    return NULL;
}

void *RTPassData( void *_args ) {
    /* Cast the args into a meaningful pointer type that we can use */
    struct thread_args *args = ( thread_args *) _args;
    printf ("First: %d; Second: '%s'\n", args->first, args->second);
    pthread_exit(NULL);
}

void *RTFrameRate( void *data) {
    cout << "Started RT HelloWorld" << endl;
    //namedWindow("Original");
    chrono::time_point<std::chrono::high_resolution_clock> start;
    Mat img(480, 640, CV_8UC1, Scalar(128,128,128));

    while(1) {
        start = std::chrono::high_resolution_clock::now();
        // If the frame is empty, break immediately
        /*** Timing and framerate ***/
        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;
        // Limit the playback to no faster than 30 fps
        if (elapsed.count() < 0.0333) {
            usleep(33333 - (elapsed.count() * 1000000)); // 1000000 = one second
            finish = std::chrono::high_resolution_clock::now();
            elapsed = finish - start;
        }
        float fps = 1.0/elapsed.count();

        //cout << "frame_dt " << elapsed.count() << endl;
        char str[40];
        sprintf(str, "FPS  %.4f", fps);
        cout << "fps " << str << endl;
        //putText(imgDisp, str, Point(10, 20), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
        //imshow( "Original", img );

        char c = (char)waitKey(1);
        if( c == 27 ) break;
    }

    //destroyWindow("Original");
    pthread_exit(NULL);
}

void *RTDebug( void *data) {
    cout << "Started RT Debug" << endl;
    //namedWindow("Original");
    chrono::time_point<std::chrono::high_resolution_clock> start;
    //Mat img(480, 640, CV_8UC1, Scalar(128,128,128));
    Mat img(480, 640, CV_8UC1);
    VideoCapture cap;
    int i = 0;
    while (!cap.open(i)) {
        i++;
        if ( i == 5 ) return NULL;
    }
    //cout << "FPS default: " << cap.get(CAP_PROP_FPS) << endl;
    //cap.set(CAP_PROP_FPS, 60);
    //cout << "FPS current: " << cap.get(CAP_PROP_FPS) << endl;
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J','P','G'));
    int frame_width = cap.get(CAP_PROP_FRAME_WIDTH);
    int frame_height = cap.get(CAP_PROP_FRAME_HEIGHT);
    int capfps = cap.get(CAP_PROP_FPS);
    cout << "Opening camera in " << frame_width << " x " << frame_height << " mode at " << capfps << " fps\n";

    while(1) {
        start = std::chrono::high_resolution_clock::now();
        if (!cap.read(img)) { // read a new frame from video
            cout << "Failed to get next frame." << endl;
            exit(EXIT_FAILURE);
        }
        // If the frame is empty, break immediately
        /*** Timing and framerate ***/
        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;
        // Limit the playback to no faster than 30 fps
        /*if (elapsed.count() < 0.0333) {
            usleep(33333 - (elapsed.count() * 1000000)); // 1000000 = one second
            finish = std::chrono::high_resolution_clock::now();
            elapsed = finish - start;
        }*/
        float fps = 1.0/elapsed.count();

        //cout << "frame_dt " << elapsed.count() << endl;
        char str[40];
        sprintf(str, "FPS  %.4f", fps);
        cout << "fps " << str << endl;
        //putText(imgDisp, str, Point(10, 20), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
        //imshow( "Original", img );

        char c = (char)waitKey(1);
        if( c == 27 ) break;
    }
    //destroyWindow("Original");
    pthread_exit(NULL);
}

void *RTCameraView( void *data ) {
    cout << "Started RT Camera View" << endl;
    namedWindow("Original");
    chrono::time_point<std::chrono::high_resolution_clock> start, finish;
    //Mat img(480, 640, CV_8UC1, Scalar(128,128,128));
    Mat img(480, 640, CV_8UC3);
    VideoCapture cap;
    int i = 0;
    while (!cap.open(i)) {
        i++;
        if ( i == 5 ) return NULL;
    }
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J','P','G'));
    int frame_width = cap.get(CAP_PROP_FRAME_WIDTH);
    int frame_height = cap.get(CAP_PROP_FRAME_HEIGHT);
    int capfps = cap.get(CAP_PROP_FPS);
    cout << "Opening camera in " << frame_width << " x " << frame_height << " mode at " << capfps << " fps\n";

    while(1) {
        start = std::chrono::high_resolution_clock::now();
        if (!cap.read(img)) { // read a new frame from video
            cout << "Failed to get next frame." << endl;
            exit(EXIT_FAILURE);
        }
        // If the frame is empty, break immediately
        /*** Timing and framerate ***/
        finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;
        // Limit the playback to no faster than 30 fps
        //cout << "frame_dt " << elapsed.count() << endl;
        //putText(imgDisp, str, Point(10, 20), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
        //imshow( "Original", img );
        //char c = (char)waitKey(1);
        //if( c == 27 ) break;
        if (elapsed.count() < 0.0333) {
            usleep(33333 - (elapsed.count() * 1000000)); // 1000000 = one second
            finish = std::chrono::high_resolution_clock::now();
            elapsed = finish - start;
        }
        float fps = 1.0/elapsed.count();
        char str[40];
        sprintf(str, "FPS  %.4f", fps);
        cout << "fps " << str << endl;
    }

    //destroyWindow("Original");
    pthread_exit(NULL);
}

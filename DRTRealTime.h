#ifndef __DRT_REALTIME_H__
#define __DRT_REALTIME_H__

#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <iostream>
#include <mutex>
#include <chrono>
#include <pthread.h>
#include "xmas_tree.h"
#include "patdet.h"
#include "wiringPi.h"

using namespace std;
using namespace cv;

struct drt_data {
    // metrics & parameters
    float fps = 0.0;
    int failctr = 0; 
    int mtxfailctr = 0; 
    int camfailctr = 0; 
    int framectr = 0; 
    int capw = 0;
    int caph = 0;
    int capfps = 0;
    int capbrt = 0;
    int capsat = 0;
    int capcon = 0;
    int setbrt = 0;
    int setsat = 0;
    int setcon = 0;

    // flags
    bool imgRdy; // flag for notifying parent thread that the img Mat has been updated
    bool running; // if this true, kill the thread
    bool kill; // if this true, kill the thread
    bool ps_detected; // true when prestage lamps detected
    bool rev_pol; // false is normal, true is reversed signal logic

    // objects
    VideoCapture* cap;
    XmasTree* tree;
  
    Mat *p_img_1 = new Mat(480, 640, CV_8UC3); // three-channel Mat for unfiltered RGB
    Mat *p_img_2 = new Mat(480, 640, CV_8UC3); // three-channel Mat for unfiltered RGB
    Mat *p_imgDisp = new Mat(480, 640, CV_8UC3); // three-channel Mat for unfiltered RGB

    // timers
    chrono::time_point<std::chrono::high_resolution_clock> start, 
                                                           finish, 
                                                           heat_start, 
                                                           heat_finish; // mark time at beginning and end of frame
    chrono::duration<double> elapsed,
                             elapsed_heat;

    // bump variables
    double bump_t; //bump duration in seconds
    bool bump;
};

mutex drtmtx;





/********************************
****     TCBLive       *******
********************************/
void *TCBLive( void *_data) {
    cout << "TCBLive" << endl;
    /* Cast _data into a meaningful pointer type that we can use */
    drt_data *data = ( drt_data *) _data;

    // local flags
    bool ps_detected = false; // true if prestage lamps detected

    // local variables
    int mtxfailctr = 0;
    int camfailctr = 0;
    int framectr = 0;
    int capw, caph, capfps, capbrt, capsat, capcon;

    Mat imgRaw(480, 640, CV_8UC3, Scalar(255,255,255));
    Mat imgProc(480, 640, CV_8UC3, Scalar(255,255,255));
    Mat imgThresh(480,640, CV_8UC1, Scalar(255));

    Mat imgDisp(480, 640, CV_8UC3);

    // Circle detection
    int morph_val, gb_val1, gb_val2;
    vector<Vec3f> rawcircles;
    // Stage/Prestage lamp detection
    bool detect_fail = true;
    //float dist[3] = { 2.0, 5.4, 2.0 }; // dist to second lamp, across the center post, and then between last two lamps. Multiples of radius.
    vector<Vec3f> invec;
    float fuzz, ps2x, ps3x, lmpwidth;
    int ps2x_pred, ps2x_act, ps3x_pred, ps3x_act;
    int psdeterror = 20; // number of pixels allowed to be off by
    bool ps2_found = false;
    bool ps3_found = false;
    std::vector<cv::Vec3f> pslamps, stlamps;

    enum Sequence { IDLE, SEARCHING, TRACKING } seq;
    seq = SEARCHING;


    /*** initialize the camera ***/
    VideoCapture cap;
    int i = 0;
    while (!cap.open(i)) {
        i++;
        if ( i == 5 ){
            cout << "ERROR - cv::VideoCapture failed to open the camera";
            pthread_exit(NULL);
        }
    }
    cout << "opened camera on " << i << endl;
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J','P','G'));
    cap.set(CAP_PROP_BRIGHTNESS, data->setbrt);  // -64 to 64
    cap.set(CAP_PROP_CONTRAST, data->setcon);  // 0-64
    cap.set(CAP_PROP_SATURATION, data->setsat);  // 0-128 

    capw   = cap.get(CAP_PROP_FRAME_WIDTH);
    caph   = cap.get(CAP_PROP_FRAME_HEIGHT);
    capfps = cap.get(CAP_PROP_FPS);
    capbrt = cap.get(CAP_PROP_BRIGHTNESS);
    capsat = cap.get(CAP_PROP_SATURATION);
    capcon = cap.get(CAP_PROP_CONTRAST);
    /****** camera complete *******/

    // local timers
    chrono::time_point<std::chrono::high_resolution_clock> frame_st, 
                                                           frame_end, 
                                                           heat_st, 
                                                           heat_end; 
    chrono::duration<double> frame_dt,
                             heat_dt;
    heat_dt = chrono::duration<double>(0.0);
    chrono::seconds timeout(60);

    // populate data struct, assign pointers, etc.
    data->imgRdy = false;
    //data->p_img_1 = &imgRaw;
    data->p_imgDisp = &imgDisp;

    data->capw   = capw;
    data->caph   = caph;
    data->capfps = capfps;
    data->capbrt = capbrt;
    data->capsat = capsat;
    data->capcon = capcon;
    data->cap    = &cap; // this tells DRTLive that the camera is opened

    // copy all of the tree values to the thread stack
    int wLowH        = data->tree->wLowH;
    int wHighH       = data->tree->wHighH;
    int wLowS        = data->tree->wLowS;
    int wHighS       = data->tree->wHighS;
    int wLowV        = data->tree->wLowV;
    int wHighV       = data->tree->wHighV;
    int yLowH        = data->tree->yLowH;
    int yHighH       = data->tree->yHighH;
    int yLowS        = data->tree->yLowS;
    int yHighS       = data->tree->yHighS;
    int yLowV        = data->tree->yLowV;
    int yHighV       = data->tree->yHighV;
  
    int dp           = data->tree->dp;
    int min_dist     = data->tree->min_dist;
    int canny_edge   = data->tree->canny_edge;
    int center_det   = data->tree->center_det;
    int min_rad      = data->tree->min_rad;
    int max_rad      = data->tree->max_rad; 

    float prestage_y = data->tree->prestage_y;
    float stage_y    = data->tree->stage_y;
    float y1_y       = data->tree->y1_y;
    float y2_y       = data->tree->y2_y;
    float y3_y       = data->tree->y3_y;
    float column1_x  = data->tree->column1_x;
    float column2_x  = data->tree->column2_x;
    float st2_x      = data->tree->st2_x;
    float st3_x      = data->tree->st3_x;
    cout << "loaded xmas tree\n";

    // start heat timer
    heat_st = chrono::high_resolution_clock::now();

    // entering critical path... do not allocate memory
    /****************************** CRITICAL PATH START ******************************/
    while(heat_dt < timeout && !data->kill){
        // start frame timer
        frame_st = chrono::high_resolution_clock::now();
        framectr++;
        // do algorithm here
        if (seq == SEARCHING) {
            if (cap.read(imgRaw)) {
                resize(imgRaw, imgRaw, Size(1067, 800));
                Rect2d cropTo(293, 0,  480, 800);
                imgRaw = Mat(imgRaw, cropTo);
                // convert to HSV format
                cvtColor(imgRaw, imgProc, COLOR_BGR2HSV); 
                // filter stage and prestage white lamps 
                inRange(imgProc, Scalar(wLowH, wLowS, wLowV), Scalar(wHighH, wHighS, wHighV), imgThresh); 
                // detect raw circles
                HoughCircles( imgThresh, rawcircles, HOUGH_GRADIENT, dp, min_dist, canny_edge, center_det, min_rad, max_rad );
                if(rawcircles.size() > 0) cout << "rawcircles - " << rawcircles.size() << endl;
                morph_val = 5;
                gb_val1 = 9;
                gb_val2 = 2;
                erode ( imgThresh, imgThresh, getStructuringElement(MORPH_ELLIPSE, Size(morph_val, morph_val)) );
                dilate( imgThresh, imgThresh, getStructuringElement(MORPH_ELLIPSE, Size(morph_val, morph_val)) );
                dilate( imgThresh, imgThresh, getStructuringElement(MORPH_ELLIPSE, Size(morph_val, morph_val)) );
                erode ( imgThresh, imgThresh, getStructuringElement(MORPH_ELLIPSE, Size(morph_val, morph_val)) );
                GaussianBlur( imgThresh,imgThresh, Size(gb_val1, gb_val1), gb_val2, gb_val2 );
                pslamps.clear();
                stlamps.clear();

                // b. Look for patterns and find the tree
                if (rawcircles.size() > 0) {
                    //cout << "rawcircles: " << frame.rawcircles.size() << endl;
                    vector<Vec3f> tmpvec = PatDet::sortByY(rawcircles);

                    // Detect Prestage lights 
                    pslamps = PatDet::WithinRadius_Y(tmpvec[0], tmpvec, 2.0, true);
                    pslamps = PatDet::sortByX(pslamps);

                    // isStageLamps - prestage
                    if (pslamps.size() == 4) {
                        Vec3f ps1 = invec[0];
                        Vec3f ps2 = invec[1];
                        Vec3f ps3 = invec[2];
                        Vec3f ps4 = invec[3];
                        lmpwidth = cvRound(ps4[0] - ps1[0]);
                        ps2x_pred = cvRound(ps1[0] + (ps2x * lmpwidth));
                        ps2x_act  = cvRound(ps2[0]);
                        ps3x_pred = cvRound(ps1[0] + (ps3x * lmpwidth));
                        ps3x_act  = cvRound(ps3[0]);
                        ps2_found = ((ps2[0] > (ps2x_pred - psdeterror)) && (ps2[0] < (ps2x_pred + psdeterror)));
                        ps3_found = ((ps3[0] > (ps3x_pred - psdeterror)) && (ps3[0] < (ps3x_pred + psdeterror)));
                        if (ps2_found && ps3_found) {
                            ps_detected = true;
                            cout << "Prestage detected";
                        }
                        
                    }
                }
            } else {
                camfailctr++;
            }
        }
        // update data struct
        if(drtmtx.try_lock()) {
            // choose what to display
            imgRaw.copyTo(imgDisp);
            //imgProc.copyTo(imgDisp);
            //imgThresh.copyTo(imgDisp);
            data->imgRdy = true;
            drtmtx.unlock();
        } else {
            mtxfailctr++;
        }

        /*** Timing and framerate ***/
        frame_end = chrono::high_resolution_clock::now();
        frame_dt = frame_end - frame_st;

        // thread-safe atomic operations on shared data struct
        data->ps_detected = ps_detected;
        data->fps = 1.0/frame_dt.count();
        data->elapsed = frame_dt;
        heat_dt += frame_dt;
    }
    // exiting critical path
    cout << "reached timeout. " << timeout.count() << " seconds elapsed, thread exiting" << endl;

    cout << "Heat duration by sum of frames: " << heat_dt.count() << endl;
    heat_end = chrono::high_resolution_clock::now();
    heat_dt = heat_end - heat_st;
    cout << "Heat duration by timer: " << heat_dt.count() << endl;
    data->elapsed_heat = heat_dt;
    data->framectr = framectr;
    data->mtxfailctr = mtxfailctr;
    data->camfailctr = camfailctr;

    data->running = false;
    pthread_exit(NULL);
};








/********************************
****     TCBCamTest       *******
********************************/
void *TCBCamTest( void *_data) {
    cout << "TCBCamTest called" << endl;

    // local variables
    int mtxfailctr = 0;
    int camfailctr = 0;
    int framectr = 0;
    int capw, caph, capfps, capbrt, capsat, capcon;

    Mat imgRaw(480, 640, CV_8UC3, Scalar(255,255,255));
    Mat imgProc(480, 640, CV_8UC3, Scalar(255,255,255));
    Mat imgDisp(480, 640, CV_8UC3);

    /*** initialize the camera ***/
    VideoCapture cap;
    int i = 0;
    while (!cap.open(i)) {
        i++;
        if ( i == 5 ){
            cout << "ERROR - cv::VideoCapture failed to open the camera";
            pthread_exit(NULL);
        }
    }
    cout << "opened camera on " << i << endl;
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J','P','G'));
    cap.set(CAP_PROP_BRIGHTNESS, 30);  // -64 to 64
    cap.set(CAP_PROP_CONTRAST, 32);  // 0-64
    cap.set(CAP_PROP_SATURATION, 60);  // 0-128 

    capw = cap.get(CAP_PROP_FRAME_WIDTH);
    caph = cap.get(CAP_PROP_FRAME_HEIGHT);
    capfps = cap.get(CAP_PROP_FPS);
    capbrt = cap.get(CAP_PROP_BRIGHTNESS);
    capsat = cap.get(CAP_PROP_SATURATION);
    capcon = cap.get(CAP_PROP_CONTRAST);
    /****** camera complete *******/

    // local timers
    chrono::time_point<std::chrono::high_resolution_clock> frame_st, 
                                                           frame_end, 
                                                           heat_st, 
                                                           heat_end; 
    chrono::duration<double> frame_dt,
                             heat_dt;
    heat_dt = chrono::duration<double>(0.0);
    chrono::seconds timeout(120);

    /* Cast _data into a meaningful pointer type that we can use */
    drt_data *data = ( drt_data *) _data;
    // populate data struct, assign pointers, etc.
    data->imgRdy = false;
    //data->p_img_1 = &imgRaw;
    data->p_imgDisp = &imgDisp;

    data->capw   = capw;
    data->caph   = caph;
    data->capfps = capfps;
    data->capbrt = capbrt;
    data->capsat = capsat;
    data->capcon = capcon;
    data->cap = &cap; // this tells DRTLive that the camera is opened

    // start heat timer
    heat_st = chrono::high_resolution_clock::now();

    // entering critical path... do not allocate memory
    while(heat_dt < timeout && !data->kill){
        // start frame timer
        frame_st = chrono::high_resolution_clock::now();
        framectr++;
        // do algorithm here
        if (cap.read(imgRaw)) {
            cvtColor(imgRaw, imgProc, COLOR_BGR2HSV); 
            inRange(imgProc, Scalar(0, 0, 245), Scalar(130, 130, 255), imgProc); 

            // update data struct
            if( !data->imgRdy && drtmtx.try_lock()) {
                imgProc.copyTo(imgDisp);
                //imgThresh.copyTo(imgDispC3);
                data->imgRdy = true;
                drtmtx.unlock();
            } else {
                mtxfailctr++;
            }
        } else {
            camfailctr++;
        }

        /*** Timing and framerate ***/
        frame_end = chrono::high_resolution_clock::now();
        frame_dt = frame_end - frame_st;
        data->fps = 1.0/frame_dt.count();
        data->elapsed = frame_dt;
        heat_dt += frame_dt;
    }

    // exiting critical path
    cout << "reached timeout. " << timeout.count() << " seconds elapsed, thread exiting" << endl;
    cout << "Heat duration by sum of frames: " << heat_dt.count() << endl;
    heat_end = chrono::high_resolution_clock::now();
    heat_dt = heat_end - heat_st;
    cout << "Heat duration by timer: " << heat_dt.count() << endl;
    data->elapsed_heat = heat_dt;
    data->framectr = framectr;
    data->mtxfailctr = mtxfailctr;
    data->camfailctr = camfailctr;

    data->running = false;
    pthread_exit(NULL);
};


/********************************
****     TCBMatTest       *******
********************************/
void *TCBMatTest( void *_data) {
    cout << "TCBMatTest called" << endl;

    // local variables
    int failctr = 0;
    int framectr = 0;
    Mat imgRaw(480, 640, CV_8UC3);
    imgRaw = Scalar(255,255,255);
    Mat img_2(480, 640, CV_8UC3);
    img_2 = Scalar(255,255,255);

    // local timers
    chrono::time_point<std::chrono::high_resolution_clock> frame_st, 
                                                           frame_end, 
                                                           heat_st, 
                                                           heat_end; 
    chrono::duration<double> frame_dt,
                             heat_dt;

    /* Cast _data into a meaningful pointer type that we can use */
    drt_data *data = ( drt_data *) _data;
    data->imgRdy = false;
    data->p_img_1 = &imgRaw;

    // initialize timers
    heat_dt = chrono::duration<double>(0.0);
    chrono::seconds timeout(5);
    heat_st = chrono::high_resolution_clock::now();

    // entering critical path... do not allocate memory
    while(heat_dt < timeout && !data->kill){
        // start the timer for the frame
        frame_st = chrono::high_resolution_clock::now();
        framectr++;

        if(drtmtx.try_lock()) {
            imgRaw = CVBLUE;
            data->imgRdy = true;
            drtmtx.unlock();
        } else {
            failctr++;
            //cout << "Failed to lock mutex  " << data->failctr << endl;
        }
        /*** Timing and framerate ***/
        frame_end = chrono::high_resolution_clock::now();
        frame_dt = frame_end - frame_st;
        // Limit the playback to no faster than 100 fps
        if (frame_dt.count() < 0.01) {
            usleep(10000 - (frame_dt.count() * 1000000)); // 1000000 = one second
            frame_end = chrono::high_resolution_clock::now();
            frame_dt = frame_end - frame_st;
        }
        data->fps = 1.0/frame_dt.count();
        data->elapsed = frame_dt;
        heat_dt += frame_dt;
    }
    // exiting critical path
    cout << "reached timeout. " << timeout.count() << " seconds elapsed, thread exiting" << endl;

    cout << "Heat duration by sum of frames: " << heat_dt.count() << endl;
    heat_end = chrono::high_resolution_clock::now();
    heat_dt = heat_end - heat_st;
    cout << "Heat duration by timer: " << heat_dt.count() << endl;
    data->elapsed_heat = heat_dt;
    data->framectr = framectr;
    data->failctr = failctr;

    data->running = false;
    pthread_exit(NULL);
};



/********************************
****  TCBClockShareFPS    *******
********************************/
void *TCBClockShareFPS( void *_data) {
    cout << "Started Real-time Share Data Test" << endl;
    chrono::time_point<std::chrono::high_resolution_clock> start;
    Mat img(480, 640, CV_8UC1, Scalar(128,128,128));

    /* Cast the args into a meaningful pointer type that we can use */
    drt_data *data = ( drt_data *) _data;
    data->imgRdy = false;
    //printf ("First: %d; Second: '%s'\n", args->first, args->second);
    std::chrono::duration<double> timeelapsed(0.0);
    std::chrono::seconds timeout(5);
    while(timeelapsed.count() < timeout.count()){
        start = std::chrono::high_resolution_clock::now();
        // If the frame is empty, break immediately
        /*** Timing and framerate ***/
        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;
        // Limit the playback to no faster than 30 fps
        //if (elapsed.count() < 0.0333) {
        // Limit the playback to no faster than 100 fps
        if (elapsed.count() < 0.01) {
            usleep(10000 - (elapsed.count() * 1000000)); // 1000000 = one second
            finish = std::chrono::high_resolution_clock::now();
            elapsed = finish - start;
        }
        float fps = 1.0/elapsed.count();
        timeelapsed += elapsed;
        if(drtmtx.try_lock()) {
            data->fps = fps;
            data->imgRdy = true;
            data->failctr++;
            drtmtx.unlock();
        }
        //char str[40];
        //sprintf(str, "FPS  %.4f", fps);
        //cout << "fps " << str << endl;
    }
    cout << "timeout elapsed, thread exiting" << endl;
    data->running = false;
    pthread_exit(NULL);
};


/********************************
*****   TCBClock  ***************
********************************/
void *TCBClock( void *data) {
    cout << "Started Real-time Clock Test" << endl;
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
        char str[40];
        sprintf(str, "FPS  %.4f", fps);
        cout << "fps " << str << endl;
    }
    pthread_exit(NULL);
};

#endif



/********************************
****  TCBBump    *******
********************************/
void *TCBBump( void *_data) {
    chrono::time_point<std::chrono::high_resolution_clock> start, finish;
    drt_data *data = ( drt_data *) _data;
    int us = (int)(data->bump_t * 1000000.0);
    int high = 1;
    int low  = 0;
    if(data->rev_pol) {
        high = 0;
        low  = 1;
    }
    start = std::chrono::high_resolution_clock::now();
    #ifndef VM
        digitalWrite (BRKREL, high) ;
    #else
        cout << "Brake signal engaged" << endl;
    #endif

    usleep(us); // 1000000 = one second

    #ifndef VM
        digitalWrite (BRKREL,  low) ;
    #else
        cout << "Brake signal disengaged" << endl;
    #endif

    finish = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = finish - start;
    cout << "commanded bump time " << data->bump_t << endl;
    cout << "actual bump time " << elapsed.count() << endl;
    pthread_exit(NULL);
};

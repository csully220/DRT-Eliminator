#include "DRTLive.h"
#include "DRTRealTime.h"

using namespace std;
using namespace cv;

/********************************
****      RTLive          *******
********************************/
int DRTLive::RTLive() {
    bool use_gui = true;
    if (NULL == getenv("DISPLAY")) use_gui = false;
    
    if(use_gui) {
        namedWindow("Live",  WINDOW_AUTOSIZE);
        moveWindow("Live", 0, 0);
    }

    Mat  mImgDisp(480, 640, CV_8UC3);

    FILE* pFile = fopen("log/drt.log", "w");
    Output2FILE::Stream() = pFile;
    FILELog::ReportingLevel() = LOG_LEVEL;
    FILE_LOG(logINFO) << "\n\nOpening log file.   OpenCV version  " << CV_VERSION;
    FILE_LOG(logINFO) << "DRTLive::RTLive called";
    FILE_LOG(logDEBUG) << "Using GUI " << use_gui;

    // Initialize local variables
    int ret = 0;

    // Initialize data struct
    drt_data *data = (drt_data*)malloc (sizeof (drt_data));
    data->ps_detected = false;
    treeprofile = s_track_dir + "tree.cfg";
    data->tree = new XmasTree(treeprofile);

    data->cap = NULL;

    INIReader reader( "./data/camera.cfg" );
    if (reader.ParseError() < 0) {
        FILE_LOG(logERROR) << "Failed to load camera.cfg file";
        data->setbrt = 30;
        data->setcon = 32;
        data->setsat = 60;
    }

    string section = "camera";
    data->setbrt = reader.GetInteger(  section, "BRIGHTNESS",  0);
    data->setcon = reader.GetInteger(  section, "CONTRAST",    0);
    data->setsat = reader.GetInteger(  section, "SATURATION",  0);

    // Initialize the realtime thread
    InitThread();
    ret = pthread_create(&thread, &attr, TCBLive, (void*)data);
    if (ret) {
        FILE_LOG(logERROR) << "create pthread failed";
        return ret;
    } else {
        FILE_LOG(logDEBUG) << "pthread created";
        data->running = true;
    }
    usleep(100000); // sleep for 100ms so the thread can start running
    // wait for the camera to initialize
    int timeout = 0;
    while (!data->cap && data->running && timeout < 60) { // 6 seconds. (100 * 100ms = 10 seconds)
        usleep(100000); // sleep for 100ms until the camera opens or we time out.
        timeout++;
    }
    FILE_LOG(logINFO) << "Camera settings:\n\
                                 resolution: " << data->capw << " x " << data->caph << " at " << data->capfps << " fps\n\
                                 brightness: " << data->capbrt << "\n\
                                 saturation: " << data->capsat << "\n\
                                 contrast: " << data->capcon;

    // LOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOP //   
    while(data->running) {
        if(data->imgRdy) {
            // lock the mutex
            drtmtx.lock();
                // FILE_LOG(logDEBUG) << "Mean value: " << mean(*data->p_img_2) << endl;
                if(use_gui) { 
                    data->p_imgDisp->copyTo(mImgDisp);
                    //imshow("Live", *data->p_imgDisp);
                }
                // sprintf(str, "FPS  %.4f", data->fps);
                // only use copyTo() because clone() will allocate new memory and change address!!!
            data->imgRdy = false;
            drtmtx.unlock();
            imshow("Live", mImgDisp);
        }

        int key_val = waitKeyEx(17);
        switch(key_val) {
            case 27:    // esc - exit program
                FILE_LOG(logINFO) << "User pressed escape. Exiting...";
                data->kill = true;
            break;
            default:
            break;
        }
    }


    if(use_gui) destroyWindow("Live");
    /* Join the thread and wait until it is done */
    ret = pthread_join(thread, NULL);
    if (ret) {
        FILE_LOG(logERROR) << "pthread did not join";
        ret = -1;
    }
    FILE_LOG(logDEBUG) << "pthread joined\n****************  HEAT STATS ***************";
    
    FILE_LOG(logDEBUG) << "pthread try_lock fails: " << data->mtxfailctr;
    FILE_LOG(logDEBUG) << "Camera read fails:      " << data->mtxfailctr;
    float avgframedt = data->elapsed_heat.count()/(float)data->framectr;
    FILE_LOG(logDEBUG) << "Number of frames:       " << data->framectr;
    FILE_LOG(logDEBUG) << "Average frame duration: " << avgframedt;
    FILE_LOG(logDEBUG) << "Average fps:            " << 1.0/avgframedt;
    FILE_LOG(logDEBUG) << "Staging lamps detected: " << data->ps_detected;;
    FILE_LOG(logINFO) << "---- END OF LOG ----";
    return ret;
}











/********************************
****      RTCameraTest    *******
********************************/
int DRTLive::RTCameraTest() {
    namedWindow("Test",  WINDOW_AUTOSIZE);
    moveWindow("Test", 0, 0);

    FILE* pFile = fopen("log/drt.log", "w");
    Output2FILE::Stream() = pFile;
    FILELog::ReportingLevel() = LOG_LEVEL;
    FILE_LOG(logINFO) << "\n\nOpening log file.   OpenCV version  " << CV_VERSION;
    FILE_LOG(logINFO) << "DRTLive::RTCameraTest called";

    // Initialize local variables
    int ret = 0;

    Mat mImgDisp(480, 640, CV_8UC3);

    // Initialize data struct
    drt_data *data = (drt_data*)malloc (sizeof (drt_data));

    data->cap = NULL;

    // Initialize the realtime thread
    InitThread();
    ret = pthread_create(&thread, &attr, TCBCamTest, (void*)data);
    if (ret) {
        FILE_LOG(logERROR) << "create pthread failed";
        return ret;
    } else {
        FILE_LOG(logDEBUG) << "pthread created";
        data->running = true;
    }

    usleep(100000); // sleep for 100ms so the thread can start running

    // wait for the camera to initialize

    int timeout = 0;
    while (!data->cap && data->running && timeout < 60) { // 6 seconds. (100 * 100ms = 10 seconds)
        usleep(100000); // sleep for 100ms until the camera opens or we time out.
        timeout++;
    }
    FILE_LOG(logINFO) << "Camera settings:\n\
                                 resolution: " << data->capw << " x " << data->caph << " at " << data->capfps << " fps\n\
                                 brightness: " << data->capbrt << "\n\
                                 saturation: " << data->capsat << "\n\
                                 contrast: " << data->capcon;
    while(data->running) {
        if(data->imgRdy) {
            // lock the mutex
            drtmtx.lock();
                // FILE_LOG(logDEBUG) << "Mean value: " << mean(*data->p_img_2) << endl;
                data->p_imgDisp->copyTo(mImgDisp);
                // only use copyTo() because clone() will allocate new memory and change address!!!
            data->imgRdy = false;
            drtmtx.unlock();
        }
        imshow("Test", *data->p_imgDisp);

        int key_val = waitKeyEx(33);
        switch(key_val) {
            case 27:    // esc - exit program
                FILE_LOG(logINFO) << "User pressed escape. Exiting...";
                data->kill = true;
            break;
            default:
            break;
        }
    }

    destroyWindow("Test");
    /* Join the thread and wait until it is done */
    ret = pthread_join(thread, NULL);
    if (ret) {
        FILE_LOG(logERROR) << "pthread did not join";
        ret = -1;
    }
    FILE_LOG(logDEBUG) << "pthread joined\n****************  HEAT STATS ***************";
    
    FILE_LOG(logDEBUG) << "pthread try_lock fails: " << data->mtxfailctr;
    FILE_LOG(logDEBUG) << "Camera read fails:      " << data->mtxfailctr;
    float avgframedt = data->elapsed_heat.count()/(float)data->framectr;
    FILE_LOG(logDEBUG) << "Number of frames:       " << data->framectr;
    FILE_LOG(logDEBUG) << "Average frame duration: " << avgframedt;
    FILE_LOG(logDEBUG) << "Average fps:            " << 1.0/avgframedt;
    FILE_LOG(logINFO) << "---- END OF LOG ----";
    return ret;
}









/********************************
****      RTMatTest       *******
********************************/
int DRTLive::RTMatTest() {
    namedWindow("Test",  WINDOW_AUTOSIZE);
    moveWindow("Test", 0, 0);

    FILE* pFile = fopen("log/drt.log", "w");
    Output2FILE::Stream() = pFile;
    FILELog::ReportingLevel() = LOG_LEVEL;
    FILE_LOG(logINFO) << "\n\nOpening log file.   OpenCV version  " << CV_VERSION;
    FILE_LOG(logINFO) << "DRTLive::RTMatTest called";


    // Local variables
    int ret = 0;
    //int bust_count = 0; // how many times did the frame bust
    //char buf[64];
    //Mat* p_img_1 = NULL; // new Mat(480, 640, CV_8UC3);
    Mat mImgRaw(480, 640, CV_8UC3);

    // Initialize the data struct
    drt_data *data = (drt_data*)malloc (sizeof (drt_data));
    data->fps = 0.0;
    data->failctr = 0;

    // Initialize the realtime thread
    ret = InitThread();
    if (ret) {
        FILE_LOG(logERROR) << "initialize pthread failed";
        return ret;
    }
    // Start the thread
    ret = pthread_create(&thread, &attr, TCBMatTest, (void*)data);
    if (ret) {
        FILE_LOG(logERROR) << "create pthread failed";
        return -1;
    } else {
        FILE_LOG(logDEBUG) << "pthread created";
        data->running = true;
    }

    usleep(100000); // sleep 100ms so the thread can start running 
    int start_timeout = 0;
    while(!data->p_img_1 && data->running) {
        usleep(100000); // sleep 100ms so the thread can start running 
        start_timeout++;
        if (start_timeout > 100) break;
    }

    /***************      main loop      ************************/
    while(data->running) {
        // something is ready
        if(data->imgRdy) {
            // lock the mutex
            drtmtx.lock();
            {
                data->p_img_1->copyTo(mImgRaw);
                //sprintf(buf, "FPS  %.4f", data->fps);
                //cout << "Parent: " << buf << endl;
                //Scalar avg = mean(*data->pImgRaw);
                //cout << "Mean value: " << avg << endl;
                data->imgRdy = false;
                drtmtx.unlock();
            }
        } 
        imshow("Test", mImgRaw);
        int key_val = waitKeyEx(33);
        switch(key_val) {
            case 27:    // esc - exit program
                FILE_LOG(logINFO) << "User pressed escape. Exiting...";
                data->kill = true;
            break;
        }
    }
    destroyWindow("Test");
    /* Join the thread and wait until it is done */
    ret = pthread_join(thread, NULL);
    if (ret) {
        FILE_LOG(logERROR) << "pthread did not join";
        ret = -1;
    }
    FILE_LOG(logDEBUG) << "pthread joined";
    FILE_LOG(logDEBUG) << "RT thread try_lock failures - " << data->failctr;
    float avgframedt = data->elapsed_heat.count()/(float)data->framectr;
    FILE_LOG(logDEBUG) << "Number of frames: " << data->framectr;
    FILE_LOG(logDEBUG) << "Average frame duration: " << avgframedt;
    return ret;
}
 

/********************************
****  RTClockShareData    *******
********************************/
int DRTLive::RTClockShareData() {
    int ret = 0;
    InitThread();
    drt_data *data = (drt_data*)malloc (sizeof (drt_data));
    data->fps = 0.0;
    //data->second = "Hello world";
    ret = pthread_create(&thread, &attr, TCBClockShareFPS, (void*)data);
    if (ret) {
        printf("create pthread failed %d\n", ret);
        exit(EXIT_FAILURE);
    }
    usleep(500000); // sleep one half second so the thread can start running 
    while(data->running) {
        if(data->imgRdy) {
            sprintf(str, "FPS  %.4f", data->fps);
            cout << "Parent: " << str << endl;
            data->imgRdy = false;
        } else usleep(16666); // 1000000 = one second
    }
    /* Join the thread and wait until it is done */
    ret = pthread_join(thread, NULL);
    if (ret) {
        printf("join pthread failed: %m\n");
        exit(EXIT_FAILURE);
    }
    cout << "Thread joined\n";
    return ret;
}


/********************************
****     RTClockTest      *******
********************************/
int DRTLive::RTClockTest() {
    int ret = 0;
    InitThread();
    ret = pthread_create(&thread, &attr, TCBClock, NULL);
    if (ret) {
        printf("create pthread failed %d\n", ret);
        exit(EXIT_FAILURE);
    }
    /* Join the thread and wait until it is done */
    ret = pthread_join(thread, NULL);
    if (ret) {
        printf("join pthread failed: %m\n");
        exit(EXIT_FAILURE);
    }
    return ret;
}


int DRTLive::InitThread() {
    #ifdef VM
      return InitThread(false, false);
    #else
      return InitThread(true, false);
    #endif
}

int DRTLive::InitThread(bool preempt, bool memlock) {
    int ret = 0;
    ret = pthread_attr_init(&attr);
    if (ret) {
        printf("init pthread attributes failed\n");
        exit(EXIT_FAILURE);
    }
    /* Lock memory */
    if (memlock) {
        if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
            printf("mlockall failed: %m\n");
            exit(EXIT_FAILURE);
        }
    }
    /* Set a specific stack size  */
    ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN * 2);
    if (ret) {
        printf("pthread setstacksize failed\n");
        exit(EXIT_FAILURE);
    }
    ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if (ret) {
        printf("pthread setschedpolicy failed\n");
        exit(EXIT_FAILURE);
    }
    if (preempt)
        param.sched_priority = 80;
    else
        param.sched_priority = 20;
    ret = pthread_attr_setschedparam(&attr, &param);
    if (ret) {
        printf("pthread setschedparam failed\n");
        exit(EXIT_FAILURE);
    }
    /* Use scheduling parameters of attr */
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (ret) {
        printf("pthread setinheritsched failed\n");
        exit(EXIT_FAILURE);
    }
    return ret;
}

DRTLive::DRTLive() : DRTLive("./data/track_profiles/track_0/") {}

DRTLive::DRTLive(string tracks_path) : s_track_dir(tracks_path) {
    //LoadConfig("./data/config.ini");
    //treeprofile = s_track_dir + "tree.cfg";
    //tree = XmasTree(treeprofile);
}

DRTLive::~DRTLive() {
    cap.release();
}




/********************************
****      RTBump          *******
********************************/
int DRTLive::RTBump(double bump_duration, bool rev_pol) {
    int ret = 0;
    InitThread();
    drt_data *data = (drt_data*)malloc (sizeof (drt_data));
    data->bump_t = bump_duration;
    data->rev_pol = rev_pol;

    ret = pthread_create(&thread, &attr, TCBBump, (void*)data);
    if (ret) {
        printf("create pthread failed %d\n", ret);
        exit(EXIT_FAILURE);
    }
    ret = pthread_join(thread, NULL);
    if (ret) {
        printf("join pthread failed: %m\n");
        exit(EXIT_FAILURE);
    }
    cout << "Thread joined\n";
    return ret;
}



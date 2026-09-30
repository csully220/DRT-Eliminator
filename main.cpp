//C++ STL
#include "defines.h"
#include <iostream>
//#include <pthread.h>
#include <sys/mman.h>
//C Standard Lib
#include <unistd.h>
//OpenCV
#include "opencv2/highgui.hpp"
//Utilities
#ifndef VM
#include <wiringPi.h>
#endif
//Custom
#include  "DRTUtils.h"
#include  "DRTCal.h"
#include  "DRTVid.h"
#include  "DRTLive.h"
#define CVUI_IMPLEMENTATION
#define CVUI_DISABLE_COMPILATION_NOTICES
#include "cvui.h"
#include "INIReader.h"

using namespace cv;
using namespace std;

/*************************************************
 MAIN MAIN MAIN MAIN MAIN MAIN MAIN MAIN MAIN MAIN
      *****************************************/
int main( int argc, char* argv[] ) {
    bool b_brk_eng = false; // brake signal is engaged
    bool b_rev_pol = false; // brake signal polarity reversed
    bool b_pause = false;
    int profile_num = 0;
    double bump_duration = 0.02;
    string s_trk_dir = "./data/track_profiles/track_" + to_string(profile_num) + "/";


    // Brake signal polarity
    // POLARITY = 0 - normal:   high engages the transbrake, low disengages
    // POLARITY = 1 - reversed: low engages the transbrake, high disengages
    int pol = 0;
    int high = 1;
    int low = 0;
    INIReader reader( "./data/setup.cfg" );
    if (reader.ParseError() < 0) {
        FILE_LOG(logERROR) << "Failed to read setup.cfg. Defaulting to normal signal polarity.";
    } else {
        string section = "signal";
        pol = reader.GetInteger(  section, "POLARITY",  0);
        if(pol) {
            b_rev_pol = true;
            high = 0;
            low = 1;
        }
    }

    #ifndef VM
        wiringPiSetup();
        pinMode (BRKREL, OUTPUT);
        digitalWrite (BRKREL, low); // signal is off. transbrake disengaged. 
    #endif
    short int option = 0;
    if(argc <= 1) {
        cout << "Starting GUI..." << endl;
    } else {
        // If character is followed by a colon, the option requires an argument, so getopt() places a pointer to the following text in the same argv-element, or the text of the following argv-element, in optarg. Two colons mean an option takes an optional arg
        while ((option = getopt(argc, argv,":inr:dv::p:")) != -1) {
            switch (option) {
                // Effects, smoothing, etc.
                case 'd':
                    {
                    //string s_cal_vid = s_trk_dir + "cal_vid.avi";
                    DRTVid drtvid(s_trk_dir);
                    drtvid.RunVideoDebug();
                    }
                    exit(EXIT_SUCCESS);
                break;
                case 'p':
                    if(optarg) {
                        int _pronum = atoi(optarg);
                        if (_pronum >= 0 && _pronum <= 10) profile_num = _pronum;
                    }
                break;
                case 'r':
                    // r - starts in real-time mode
                    if (optarg) {
                        {
                        struct sched_param param;
                        pthread_attr_t attr;
                        pthread_t thread;
                        int ret;
                        ret = pthread_attr_init(&attr);
                        if (ret) {
                            printf("init pthread attributes failed\n");
                            exit(EXIT_FAILURE);
                        }
                        /* Lock memory */
                        //if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
                        //    printf("mlockall failed: %m\n");
                        //    exit(EXIT_FAILURE);
                        //}

                        /* Set a specific stack size  */
                        ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
                        if (ret) {
                            printf("pthread setstacksize failed\n");
                            exit(EXIT_FAILURE);
                        }

                        ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
                        if (ret) {
                            printf("pthread setschedpolicy failed\n");
                            exit(EXIT_FAILURE);
                        }
                        param.sched_priority = 80;
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
                  
                        /* Create a pthread with specified attributes */
                        //ret = pthread_create(&thread, &attr, RTHelloWorld, (void *)ptr_drtvid);
                        switch (atoi(optarg)) {
                            case 1:
                                {
                                // Options r 1: Output FPS from the RT thread
                                DRTLive drtlive;
                                drtlive.RTClockTest();
                                }
                            break;
                            case 2:
                                {
                                // Options r 2: Set the FPS in RT thread, display it in the main thread
                                DRTLive drtlive;
                                drtlive.RTClockShareData();
                                }
                            break;
                            case 3:
                                {
                                // Options r 3: Set a Mat in RT thread, display it in the main thread
                                DRTLive drtlive;
                                drtlive.RTMatTest();
                                }
                            break;
                            case 4:
                                {
                                // Options r 4: Get frame from camera in RT thread, display it in the main thread
                                DRTLive drtlive;
                                drtlive.RTCameraTest();
                                }
                            break;
                            case 5:
                                {
                                // Options r 5: Run live detect mode
                                DRTLive drtlive(s_trk_dir);
                                drtlive.RTLive();
                                }
                            break;
                            default:
                                cout << "Invalid real-time mode option" << endl;
                                exit(EXIT_FAILURE);
                        }
                        }
                        exit(EXIT_SUCCESS);
                    } 
                break;
                case 'v':
                    {
                        if (optarg) {
                            // cout << "Using video file " << optarg << endl;
                            string vidfile = optarg;
                            DRTVid drtvid;
                            drtvid.RunVideoDebug(vidfile);
                        } else {
                            DRTVid drtvid("data/track_profiles/track_0/");
                            drtvid.RunVideoDebug();
                        }
                    }
                    exit(EXIT_SUCCESS);
                break;
                default: cout << "Invalid arguments\n";
                    exit(EXIT_FAILURE);
                break;
            }
        }
        if (optind < argc) { //non-option argument video file for playback
            cout << "not implemented\n";
            exit(EXIT_FAILURE);
        }
    }
    
    Mat imgMenuBg = imread(BGIMAGE);

    if (imgMenuBg.empty()) {
        cout << "Could not open or find the background image" << endl;
        exit(EXIT_FAILURE);
    }
    
    //cvui::init("Menu", 20, true);
    cvui::init("Menu");
    //setWindowProperty("Menu", WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
    //moveWindow("Menu", 0, 0);
    
    enum MENU_TITLE { MAINMENU, TRACKPROFILE } menu_level;
    menu_level = MAINMENU;

    /************* MAIN MENU **************/
    while (true) { 
        while (menu_level == MAINMENU) {
            // Column 1
            bool b_quit_menu = false;
            imgMenuBg = imread(string(BGIMAGE));
            cvui::init("Menu", 0, true);
            while (!b_quit_menu) {
                cvui::text(imgMenuBg, COL1, ROW1, "Camera", 0.6, CVUIGRAY);
                if (cvui::button(imgMenuBg, COL1, ROW2, BTN_WID, 20, string("View Camera"))) {
                    //cout << "Not implemented\n";
                    destroyWindow("Menu");
                    DRTUtils::ShowCameraFeed();
                    cvui::init("Menu", 0, true);
                }
                if (cvui::button(imgMenuBg, COL1, ROW3, BTN_WID, 20, string("Camera Settings"))) {
                    destroyWindow("Menu");
                    DRTCal drtcal(s_trk_dir);
                    drtcal.RunCameraSettings();
                    cvui::init("Menu", 0, true);
                }
                // Column 2
                cvui::text(imgMenuBg, COL2, ROW1, "Brake Control", 0.6, CVUIGRAY);
                if (cvui::button(imgMenuBg, COL2, ROW2, BTN_WID, 20, string("Reverse Logic"))) {
                    b_rev_pol = !b_rev_pol;
                    ofstream cfgfile;
                    cfgfile.open("./data/setup.cfg");
                    if (cfgfile) {
                        cfgfile << "[signal]"
                                << "\n; POLARITY = 0 - normal:   high engages the transbrake, low disengages\n;POLARITY = 1 - reversed: low engages the transbrake, high disengages"
                                << "\nPOLARITY = " << b_rev_pol << ";\n";
                    } else FILE_LOG(logERROR) << "Failed to save signal polarity";
                    cfgfile.close();
                    imgMenuBg = imread(string(BGIMAGE));
                }
                if(b_rev_pol) cvui::text(imgMenuBg, COL2, ROW3, "SIG HI : BRK OFF", 0.6, CVUIRED);
                else cvui::text(imgMenuBg, COL2, ROW3, "SIG HI : BRK ON", 0.6, CVUIRED);

                if(!b_brk_eng) {
                    if (cvui::button(imgMenuBg, COL2, ROW4, BTN_WID, 20, string("Engage Brake"))) {
                       b_brk_eng = true;
                       #ifndef VM
                            digitalWrite (BRKREL, high) ; 
                            imgMenuBg = imread(string(BGIMAGE));
                       #else
                            cout << "Brake signal engaged";
                       #endif
                    }
                } else if (b_brk_eng) {
                    if (cvui::button(imgMenuBg, COL2, ROW4, BTN_WID, 20, string("Release Brake"))) {
                       b_brk_eng = false;
                       #ifndef VM
                            digitalWrite (BRKREL,  low) ;
                            imgMenuBg = imread(string(BGIMAGE));
                       #else
                            cout << "Brake signal disengaged";
                       #endif
                    }
                }

                // Bump controls
                cvui::text(imgMenuBg, COL1, ROW6, "Bump to Stage", 0.6, CVUIGRAY);
                cvui::counter(imgMenuBg, COL1 + 150, ROW7, &bump_duration, 0.005, "%.3f");
                    if (bump_duration > 0.5) bump_duration = 0.5;
                    else if (bump_duration < 0.0) bump_duration = 0.0;
                cvui::text(imgMenuBg, COL1, ROW7, "Duration (sec)", 0.6, CVUIGRAY);
                
                if (cvui::button(imgMenuBg, COL1, ROW8, BTN_WID, 20, string("Bump"))) {
                    destroyWindow("Menu");
                    DRTLive drtlive(s_trk_dir);
                    drtlive.RTBump(bump_duration, b_rev_pol);
                    cvui::init("Menu", 0, true);
                }

                // Track profiles
                cvui::text(imgMenuBg, COL1, ROW10, "Load Profile", 0.6, CVUIGRAY);

                cvui::counter(imgMenuBg, COL1, ROW11, &profile_num, 1, "%d");
                    if (profile_num > 10) profile_num = 10;
                    else if (profile_num < 0) profile_num = 0;

                if (cvui::button(imgMenuBg, COL1, ROW12, 100, 20, string("Preview"))) {
                    try { destroyWindow("Preview"); } catch (...) {}
                    cvui::init("Preview", 0, true);
                    Mat imgPreview = imread(string("./data/track_profiles/track_" + to_string(profile_num) + "/cal_img.png"));
                    if (imgPreview.empty()) imgPreview = imread(string("./media/backgrounds/no_cal_img.png"));
                    resize(imgPreview, imgPreview, Size(240, 180));
                    moveWindow("Preview", 200, 560);
                    cvui::imshow("Preview", imgPreview);
                    b_quit_menu = true;
                }

                if (cvui::button(imgMenuBg, COL1 + 130, ROW11, 100, 20, string("Select"))) {
                    cout << "Loading profile " << profile_num << endl;
                    menu_level = TRACKPROFILE;
                    b_quit_menu = true;
                    break;
                }
                
                // Column 4, Quit
                if (cvui::button(imgMenuBg, COL1, ROW15, 100, 20, string("Quit"))) {
                    cout << "Exiting..." << endl;
                    exit(EXIT_SUCCESS);
                }
                cvui::imshow("Menu", imgMenuBg);

                int key_val = waitKeyEx(30);
                switch(key_val) {
                    case 27:    // esc - exit program
                        cout << "Exiting..." << endl;
                        exit(EXIT_SUCCESS);
                    break;
                }
            }
        }


        while (menu_level == TRACKPROFILE) {
            char buf[32];
            sprintf(buf, "Track profile %d", profile_num);
            imgMenuBg = imread(string(BGIMAGE));
            cvui::init("Menu", 0, true);
            bool b_quit_menu = false;
            s_trk_dir = "./data/track_profiles/track_" + to_string(profile_num) + "/";
            string s_cal_img = s_trk_dir + "cal_img.png";
            string s_cal_vid = s_trk_dir + "cal_vid.avi";

            while (!b_quit_menu) {
                cvui::text(imgMenuBg, COL1, ROW2, buf, 0.6, CVUIGRAY);
                // column 1
                cvui::text(imgMenuBg, COL1, ROW3, "Camera", 0.6, CVUIGRAY);
                if (cvui::button(imgMenuBg, COL1, ROW4, BTN_WID, 20, string("View Camera"))) {
                    destroyWindow("Menu");
                    DRTUtils::ShowCameraFeed();
                    cvui::init("Menu", 0, true);
                }
                if (cvui::button(imgMenuBg, COL1, ROW5, BTN_WID, 20, string("Record Cal Video"))) {
                    destroyWindow("Menu");
                    DRTCal drtcal(s_trk_dir);
                    drtcal.RunRecordCalVideo( s_cal_vid );
                    cvui::init("Menu", 0, true);
                }
                if (cvui::button(imgMenuBg, COL1, ROW6, BTN_WID, 20, string("Select Cal Image"))) {
                    destroyWindow("Menu");
                    DRTVid drtvid(s_trk_dir);
                    drtvid.RunSelectCalImage( s_cal_vid );
                    cvui::init("Menu", 0, true);
                }
                
                // column 2
                cvui::text(imgMenuBg, COL2, ROW3, "Calibration", 0.6, CVUIGRAY);
                if (cvui::button(imgMenuBg, COL2, ROW4, BTN_WID, 20, string("Calibrate Geom"))) {
                    destroyWindow("Menu");
                    DRTCal drtcal(s_trk_dir);
                    drtcal.RunImageCalibrate(true, false);
                    cvui::init("Menu", 0, true);
                }
                if (cvui::button(imgMenuBg, COL2, ROW5, BTN_WID, 20, string("Calibrate Filter"))) {
                    destroyWindow("Menu");
                    DRTCal drtcal(s_trk_dir);
                    drtcal.RunImageCalibrate(false, true);
                    cvui::init("Menu", 0, true);
                }
                if (cvui::button(imgMenuBg, COL2, ROW6, BTN_WID, 20, string("Calibrate All"))) {
                    destroyWindow("Menu");
                    DRTCal drtcal(s_trk_dir);
                    drtcal.RunImageCalibrate(true, true);
                    cvui::init("Menu", 0, true);
                }
                if (cvui::button(imgMenuBg, COL2, ROW7, BTN_WID, 20, string("View Cal Settings"))) {
                    destroyWindow("Menu");
                    DRTCal drtcal(s_trk_dir);
                    drtcal.RunViewCalSettings();
                    cvui::init("Menu", 0, true);
                }
                if (cvui::button(imgMenuBg, COL2, ROW8, BTN_WID, 20, string("Sensitivity"))) {
                    destroyWindow("Menu");
                    DRTCal drtcal(s_trk_dir);
                    drtcal.RunSetSensitivity();
                    cvui::init("Menu", 0, true);
                }

                if (cvui::button(imgMenuBg, COL1, ROW9, BTN_WID, 20, string("Video Test"))) {
                    destroyWindow("Menu");
                    DRTVid drtvid(s_trk_dir);
                    drtvid.RunVideoDebug( s_cal_vid );
                    cvui::init("Menu", 0, true);
                }

                if (cvui::button(imgMenuBg, COL2, ROW9, BTN_WID, 20, string("Go Live"))) {
                    destroyWindow("Menu");
                    DRTLive drtlive(s_trk_dir);
                    drtlive.RTLive();
                    cvui::init("Menu", 0, true);
                }

                if (cvui::button(imgMenuBg, COL1, ROW11, 100, 20, string("Back"))) {
                    b_quit_menu = true;
                    break;
                }
                cvui::imshow("Menu", imgMenuBg);

                int key_val = waitKeyEx(30);
		//if (key_val != -1) cout << key_val << endl;
                switch(key_val) {
                    case 27:    // esc - exit program
                        b_quit_menu = true;
                    break;
                }
            }
            cout << "Returning to Main Menu..." << endl;
            menu_level = MAINMENU;
        }
    }
}


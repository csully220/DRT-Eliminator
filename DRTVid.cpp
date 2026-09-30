#include "DRTVid.h"

//#define MASK_HEIGHT_FACTOR 1.5
//#define STG_LTS_ILLUM_THRESH 60
//#define YEL_LTS_ILLUM_THRESH 30

using namespace cv;
using namespace std;

typedef chrono::time_point<std::chrono::high_resolution_clock> hr_clock_t;

//namespace fs = std::experimental::filesystem;
//using namespace fs;

DRTVid::DRTVid() : DRTVid("./data/track_profiles/track_0/") {}

DRTVid::DRTVid(string tracks_path) : s_track_dir(tracks_path) {
    tracker = TrackerMOSSE::create();
    treeprofile = s_track_dir + "tree.cfg";
    //tree = XmasTree(treeprofile);
}

DRTVid::~DRTVid() {
    cap.release();
}

void DRTVid::DetModeCycle() {
    if (det_mode == NONE) {
        det_mode = MANUAL;
    } else if (det_mode == MANUAL) {
        det_mode = PATTERN;
    } else if (det_mode == PATTERN) { 
        det_mode = NONE;
        // det_mode = HAAR;
    }
    //} else if (det_mode == HAAR) { 
    //    det_mode = NONE;
    //}
}

string DRTVid::DetModeString() {
    string s_detmode = "det mode: ERROR";
    if (det_mode == NONE) {
        s_detmode = "det mode: NONE";
    } else if (det_mode == MANUAL) {
        s_detmode = "det mode: MANUAL";
    } else if (det_mode == PATTERN) {
        s_detmode = "det mode: PATTERN";
    } else if (det_mode == HAAR) {
        s_detmode = "det mode: HAAR";
    }
    return s_detmode; 
}

// Analyze pattern for consistency with stage/prestage lamps ***
// float fuzz: allowable tolerance in multiples of radius for bounding box
// ps2_x and ps3_x are ratios where ps1_x = 0 and ps4_x = 1
bool DRTVid::isStageLamps(vector<Vec3f> invec, float fuzz, float ps2x, float ps3x) {
    if (invec.size() == 4) {
        Vec3f ps1 = invec[0];
        Vec3f ps2 = invec[1];
        Vec3f ps3 = invec[2];
        Vec3f ps4 = invec[3];
        float lmpwidth = cvRound(ps4[0] - ps1[0]);
        //FILE_LOG(logDEBUG) << "invec size: " << invec.size() << "    center-to-center stage lamps width - " << lmpwidth;

        int ps2x_pred = cvRound(ps1[0] + (ps2x * lmpwidth));
        int ps2x_act  = cvRound(ps2[0]);
        int ps3x_pred = cvRound(ps1[0] + (ps3x * lmpwidth));
        int ps3x_act  = cvRound(ps3[0]);
        //char buf[2048];
        //sprintf(buf, "ps2x prediction/actual: %d / %f,  ps2x prediction/actual: %d / %f", ps2x_pred, ps2[0], ps3x_pred, ps3[0]);
        //FILE_LOG(logDEBUG) << buf;
        int error = 20; // number of pixels allowed to be off by
        bool ps2_found = ((ps2[0] > (ps2x_pred - error)) && (ps2[0] < (ps2x_pred + error)));
        bool ps3_found = ((ps3[0] > (ps3x_pred - error)) && (ps3[0] < (ps3x_pred + error)));

        if (ps2_found && ps3_found) return true;
  
        // TODO Try selecting a sub-Matrix and checking the average value. Make it lamp diameter high and like 5 diameters wide.
        
        /*
             2.0  5.4  2.0
          ___________________
          |  *  *  |  *  *  |
          |________|________|
          |  *  *  |  *  *  |
          |________|________|

        */
       
    } /* else if (invec.size() == 2) {
        Vec3f cir1 = invec[0];
        int c1_x = cir1[0];
        int c1_r = cir1[2];
        Vec3f cir2 = invec[1];
        int c2_x = cir2[0];
        int c2_r = cir2[2];
        float r_avg = (c1_r + c2_r) / 2.0;
        float allowed_error = r_avg * fuzz;
        //float dist_c2c1 = (float)cir2[0] - (float)cir1[0];
        int nxtlamp_x = c1_x + (c1_r * dist[0]);
        if ( !(c2_x > (nxtlamp_x - allowed_error) && c2_x < (nxtlamp_x + allowed_error)) ) { // lamp is not there.
            detect_fail = true;
        } //else cout << "Found a pair\n";
    } else detect_fail = true;*/

    return false;
}


/***       Run Video Debug Mode      ***/
/***************************************/
void DRTVid::on_trackbar( int pos, void* )
{
    cout << "Debug level: " << pos << endl;
}


void DRTVid::RunVideoDebug() {
    string s_vid_file = s_track_dir + "cal_vid.avi";
    RunVideoDebug(s_vid_file);
}

void DRTVid::RunVideoDebug( string s_vid_file ) {
    cout << "Using OpenCV version " << CV_VERSION << endl << endl;
    cout << "space - pause\n"
         << "q - restart the video\n"
         << "-> fast forward 1/2 second\n"
         << "<- rewind 1/2 second\n";

    namedWindow("Control", WINDOW_AUTOSIZE); // create a window called "Control"
    namedWindow("Original",  WINDOW_AUTOSIZE);
    setWindowProperty("Original", WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
    //moveWindow("Original", 0, 0);
    //namedWindow("Filtered",  WINDOW_AUTOSIZE);
    //Mat imgZone;
    // Create trackbars in "Control" window
    int m_debug_level = 0;
    createTrackbar("Debug level", "Control", &m_debug_level, 4, on_trackbar);
    int m_crop_roi_when_tracking = 0;
    createTrackbar("Crop ROI", "Control", &m_crop_roi_when_tracking, 1);
    int m_morph = 0;
    createTrackbar("Morph", "Control", &m_morph, 1);
    int m_blur = 1;
    createTrackbar("Blur", "Control", &m_blur, 1);
    int m_fps_disp = 1;
    createTrackbar("Show FPS", "Control", &m_fps_disp, 1);
    int m_frmctr = 0;
    createTrackbar("Frame counter", "Control", &m_frmctr, 1);
    int m_displamps = 1;
    createTrackbar("Highlight lamps", "Control", &m_displamps, 1); 
    int m_filtimg_disp = 0;
    createTrackbar("Show filtered", "Control", &m_filtimg_disp, 1);
    //int m_update_circles_radius = 60;
    //createTrackbar("Circle comp slop", "Control", &m_update_circles_radius, 100); // Muliplied by radius to get boundary distance when tracking circles across frames

    // Initialize logging - logERROR, logWARNING, logINFO, logDEBUG, logDEBUG1, logDEBUG2, logDEBUG3, logDEBUG
    FILE* pFile = fopen("log/drt.log", "w");
    Output2FILE::Stream() = pFile;
    FILELog::ReportingLevel() = LOG_LEVEL;
    FILE_LOG(logINFO) << "Opening log file.   OpenCV version  " << CV_VERSION << endl;

    // Initialize timing and framerate variables
    hr_clock_t start_t, end_t, total_t;
    // timestamps when lamps detected
    hr_clock_t prestage_t, stage_t, yel1_t, yel2_t, yel3_t, grn_t;
    // frames when lamp illumination detected
    //int stage_f, yel1_f, yel2_f, yel3_f, grn_f;
    // estimated times when lamps will illuminate
    hr_clock_t yel1_t_exp, yel2_t_exp, yel3_t_exp, grn_t_exp;
    // ?wait for green" loop timers
    hr_clock_t heat_start_t, heat_end_t, curr_t;

    double fps = 30.0;              // The actual framerate.
    unsigned int frame_counter = 0;
    //double time_sum = 0;
    //std::chrono::duration<double> time_sum;
    // double> frame_dt = 0;
    std::chrono::duration<double> frame_dt;

    // Initialize flags for playback options
    bool b_pause = false; // SET
    bool b_run = true;    // GET
    bool b_frame_advance = false;    // GET
    bool b_halt_tracker = false;    // GET
    
    //Initialize image processing flags
    bool b_filtimg_disp = true;
    bool b_morph = true;
    bool b_blur = true;
    bool b_displamps = false; // highlight only detected lamps by default, otherwise highlight all detected circles
    bool b_manroi = false;
    //int trkzoneidx = 0;
    bool b_trk_success = true;
    //bool b_autoroi = true;

    FILE_LOG(logINFO) << "    *****  Video Debug Mode  *****\n";
    FILE_LOG(logINFO) << "video file: " << s_vid_file;
    cap.open(s_vid_file);

    Mat imgRaw;
    Mat imgDisp;
    Mat imgProc;
    Mat imgTrk;
    Mat imgMask;
    Scalar maskIllum;

    enum ZONE { PRESTAGE, STAGE, YEL_1, YEL_2, YEL_3, GRN, RED } zone;
    zone = PRESTAGE;

    //VisFrame frame(cap);
    cap.set( CAP_PROP_POS_FRAMES, 90 );

    tree.init(treeprofile);
    FILE_LOG(logINFO) << "Loaded tree profile - " << treeprofile;
    HSVFilterVals hsv_wht(tree.wLowH, tree.wHighH, tree.wLowS, tree.wHighS, tree.wLowV, tree.wHighV);
    HSVFilterVals hsv_yel(tree.yLowH, tree.yHighH, tree.yLowS, tree.yHighS, tree.yLowV, tree.yHighV);

    //enum SEQUENCE { IDLE, SEARCHING, TRACKING };
    seq = SEARCHING;
    //enum DETECTION_MODE { NONE, MANUAL, PATTERN, HAAR };
    det_mode = PATTERN;

    cout << "Detection mode is " << DetModeString() << endl;
    FILE_LOG(logINFO) << "Detection mode set to " << DetModeString();

    heat_start_t = std::chrono::high_resolution_clock::now();

    while (b_run) {

        if ( !b_pause || b_frame_advance ) {
            start_t = std::chrono::high_resolution_clock::now();

            frame_counter = cap.get( CAP_PROP_POS_FRAMES );
            if ( m_frmctr ) { 
                cout << "Frame: " << frame_counter << endl;
                FILE_LOG(logDEBUG4) << "Frame: " << frame_counter;
            }

            FrameUtils::NextVidFrame(imgRaw, cap);
            FrameUtils::ResizeForTouchscreen(imgRaw);
            imgRaw.copyTo(imgDisp);
            imgProc = FrameUtils::CopyThresh(imgRaw, hsv_wht, hsv_yel);
            FrameUtils::Morph(imgProc);

            // 2. Process image, detect circles and/or select an ROI
            if (seq == SEARCHING) {

                // 3. Detect the Christmas Tree
                // 3.1 Select ROI manually
                if (det_mode == MANUAL && b_manroi) {
                    bool fromCenter = false;
                    roiPrimary = selectROI("Original", imgDisp, fromCenter);
                    if(roiPrimary.width > 0 && roiPrimary.height > 0) {
                        cout << "ROI selected\n";
                        if( tracker->init(imgProc, roiPrimary)) {
                            cout << "Manual ROI selection successful - tracker initialized" << endl;
                            FILE_LOG(logINFO) << "Manual ROI selection successful - tracker initialized";
                            seq = TRACKING;
                        } else FILE_LOG(logERROR) << "Manual ROI selection failed to initialize tracker\n";
                    }
                } else if (det_mode == PATTERN) {
                    // 3.2  Detect ROI automatically
                    pslamps.clear();
                    stlamps.clear();
                    // a. Detect circles (or more generically, features) and sort, optionally

                    vector<Vec3f> rawcircles = FrameUtils::DetectCircles( imgProc, false, tree.dp, tree.min_dist, tree.canny_edge, tree.center_det, tree.min_rad, tree.max_rad );

                    // b. Look for patterns and find the tree
                    if (rawcircles.size() > 0) {
                        //cout << "rawcircles: " << frame.rawcircles.size() << endl;
                        vector<Vec3f> tmpvec = PatDet::sortByY(rawcircles);

                        /*** Detect Prestage lights ***/
                        pslamps = PatDet::WithinRadius_Y(tmpvec[0], tmpvec, 2.0, true);
                        pslamps = PatDet::sortByX(pslamps);
                        if (isStageLamps(pslamps,  0.2, tree.st2_x, tree.st3_x) ) cout << "Prestage detected\n";
                        /*** Highlight Prestage lamps ***/
                        if (m_displamps) {
                            for( size_t i = 0; i < pslamps.size(); i++ ) {
                                Point center(cvRound(pslamps[i][0]), cvRound(pslamps[i][1]));
                                Point txtctr(cvRound(pslamps[i][0]) - 6, cvRound(pslamps[i][1]) + 6);
                                int radius = cvRound(pslamps[i][2]);
                                if (i==0) {
                                    putText(imgDisp, "PS", txtctr, FONT_HERSHEY_SIMPLEX, 0.4, CVBLUE, 2);
                                } else {
                                    putText(imgDisp, to_string(i+1), txtctr, FONT_HERSHEY_SIMPLEX, 0.4, CVBLUE, 2);
                                } 
                                circle( imgDisp, center, radius, CVGREEN, 3, 8, 0 ); // circle outline
                            }
                        }

                        /*** Detect Stage lights ***/
                        stlamps = PatDet::WithinRadius_Y(tmpvec[0], tmpvec, 2.0, true);
                        stlamps = PatDet::sortByX(stlamps);
                        if (isStageLamps(stlamps,  0.2, tree.st2_x, tree.st3_x) ) cout << "Stage detected\n";
                        /*** Highlight Stage lamps ***/
                        if (m_displamps) {
                            for( size_t i = 0; i < stlamps.size(); i++ ) {
                                Point center(cvRound(stlamps[i][0]), cvRound(stlamps[i][1]));
                                Point txtctr(cvRound(stlamps[i][0]) - 6, cvRound(stlamps[i][1]) + 6);
                                int radius = cvRound(stlamps[i][2]);
                                if (i==0) {
                                    putText(imgDisp, "ST", txtctr, FONT_HERSHEY_SIMPLEX, 0.4, CVBLUE, 2);
                                } else {
                                    circle( imgDisp, center, 3, CVBLUE, -1, 8, 0 ); // circle center
                                } 
                                circle( imgDisp, center, radius, CVBLUE, 3, 8, 0 ); // circle outline
                            }
                        }
                        // highlight any other lamps purple
                        if (m_displamps) {
                            for( size_t i = 0; i < tmpvec.size(); i++ ) {
                                Point center(cvRound(tmpvec[i][0]), cvRound(tmpvec[i][1]));
                                int radius = cvRound(tmpvec[i][2]);
                                if (i == 0) {
                                    Point txtcenter(center.x - 6, center.y + 6);
                                    putText(imgDisp, "T", txtcenter, FONT_HERSHEY_SIMPLEX, 0.4, CVPURPLE, 2);
                                } else {
                                    circle( imgDisp, center, 3, CVPURPLE, -1, 8, 0 );     // circle center
                                }
                                circle( imgDisp, center, radius - 3, CVPURPLE, 3, 8, 0 ); // circle outline
                            }
                        }
                    // c. Draw a Rect for ROI
                        // detected four lamps
                        vector<Vec3f> tmplamps = pslamps;
                        if ( isStageLamps(tmplamps, 0.2, tree.st2_x, tree.st3_x) ) {
                            FILE_LOG(logDEBUG) << "Detected lamps";
                            float width = 0;
                            Vec3f ps4 = tmplamps[3];
                            Vec3f ps1 = tmplamps[0];
                            width = ps4[0] - ps1[0];
                            if (width < 0) width *= -1;
                            int height = cvRound(width * tree.aspect_ratio);
                            // We now have width and height measured center-to-center between lamps. Need to encompass all outer lamps.
                            float radius = (tmplamps[0][2] + tmplamps[1][2] + tmplamps[2][2] + tmplamps[3][2]) / 4.0;
                            int tot_width = cvRound(width + (radius * 2.0));
                            int tot_height = cvRound(height + (radius * 2.0));

                            // And now offset up and left.
                            int tl_x = cvRound(ps1[0] - radius);
                            int tl_y = cvRound(ps1[1] - radius);

                            roiPrimary = Rect2d( tl_x, tl_y, tot_width, tot_height );
                            FILE_LOG(logDEBUG4) << "Pattern ROI top left: " << tl_x << ", " << tl_y << " - width: " << tot_width;
                    // d. Init the tracker
                            if (roiPrimary.width > 0 && roiPrimary.height > 0) {
                                if( tracker->init(imgRaw, roiPrimary) ) {
                                    //rectangle(imgDisp, roiPrimary, CVBLUE, 2 );
                                    cout << "Pattern ROI selection successful - tracker initialized" << endl;
                                    FILE_LOG(logINFO) << "Pattern ROI selection successful - tracker initialized";
                                    seq = TRACKING;
                                }
                            }    
                        }
                    }
                }
            }

            // Not "else" if (TRACKING) because variables used in display later in frame might not be set correctly on first frame after switch to TRACKING mode
            if (seq == TRACKING) {
                //imgProc = FrameUtils::CopyThresh(imgRaw, hsv_wht, hsv_yel);
                //FrameUtils::Morph(imgProc);
        
                //b_trk_success = tracker->update(frame.imgRaw, roiPrimary);
                b_trk_success = tracker->update( imgRaw, roiPrimary );
                if ( !b_trk_success || b_halt_tracker ) {
                    FILE_LOG(logDEBUG) << "Tracking stopped.  Update failure - " << !b_trk_success << " or Halted - "<< b_halt_tracker;
                    seq = SEARCHING;
                    roiPrimary = Rect(Point(0,0), Point(0,0));
                    tracker = TrackerMOSSE::create();
                    cout << "Tracking stopped\n";
                    imgTrk = Mat(imgProc, roiPrimary);
                } else if (b_trk_success) {
                    imgTrk = Mat(imgProc, roiPrimary);

            // 5. Dimensions, color values, aspect ratio, etc. are properties of the tree.
            //    DRT checks if area is white or whatever. Tree should be alerted when the 
            //    lights start illuminating. That starts a counter, then call member functions on the tree
            //    with new illuminations with timestamp.
            
                // Get the mean value of each zone
                int _rows = imgTrk.rows;  // height of filtered Mat
                int startrow = 0;
                int endrow = 0;
                int half_height = cvRound((float)tree.nominal_radius * tree.mask_ht);
                int ctr_y = 0; 
                // Prestage
                    switch (zone) {
                        case PRESTAGE:
                            {
                            startrow = 0;
                            endrow = half_height * 2;
                            if (endrow > _rows) endrow = _rows;
                            imgMask = imgTrk.rowRange(startrow, endrow);
                            maskIllum = mean(imgMask);
                            Rect roiMask(roiPrimary.x, roiPrimary.y, roiPrimary.width, endrow);
                            rectangle(imgProc, roiMask, CVCYAN, 2);
                            if (maskIllum[0] > tree.thresh_stg) {
                                prestage_t = std::chrono::high_resolution_clock::now();
                                std::chrono::duration<double> elapsed = prestage_t - heat_start_t;
                                FILE_LOG(logDEBUG) << "Frame " << frame_counter << ", " << elapsed.count() << " seconds - PRESTAGE lamps detected illuminated: " << maskIllum;
                                zone = STAGE;
                            }
                            }
                        break;
                        case STAGE:
                            {
                            ctr_y = cvRound((float)_rows * tree.stage_y);
                            startrow = ctr_y - half_height;
                            if (startrow < 0) startrow = 0;
                            endrow = ctr_y + half_height;
                            if (endrow > _rows) endrow = _rows;
                            imgMask = imgTrk.rowRange(startrow, endrow);
                            maskIllum = mean(imgMask);
                            Rect roiMask(roiPrimary.x, roiPrimary.y + startrow, roiPrimary.width, endrow - startrow);
                            rectangle(imgProc, roiMask, CVCYAN, 2);
                            if (maskIllum[0] > tree.thresh_stg) {
                                //stage_t = clock()/CLOCKS_PER_SEC;
                                stage_t = std::chrono::high_resolution_clock::now();
                                std::chrono::duration<double> elapsed = stage_t - heat_start_t;
                                //stage_f = frame_counter;
                                yel1_t_exp = stage_t + std::chrono::milliseconds(500);
                                FILE_LOG(logDEBUG) << "Frame " << frame_counter << ", " << elapsed.count() << " seconds - STAGE lamps detected illuminated: " << maskIllum;
                                zone = YEL_1;
                            }
                            }
                        break;
                        case YEL_1:
                            {
                            ctr_y = cvRound((float)_rows * tree.y1_y);
                            startrow = ctr_y - half_height;
                            if (startrow < 0) startrow = 0;
                            endrow = ctr_y + half_height;
                            if (endrow > _rows) endrow = _rows;
                            imgMask = imgTrk.rowRange(startrow, endrow);
                            maskIllum = mean(imgMask);
                            Rect roiMask(roiPrimary.x, roiPrimary.y + startrow, roiPrimary.width, endrow - startrow);
                            rectangle(imgProc, roiMask, CVCYAN, 2);
                            if (maskIllum[0] > tree.thresh_yel) {
                                //yel1_t = clock()/CLOCKS_PER_SEC;
                                yel1_t = std::chrono::high_resolution_clock::now();
                                std::chrono::duration<double> elapsed = yel1_t - heat_start_t;
                                yel2_t_exp = yel1_t + std::chrono::milliseconds(500);
                                //yel1_f = frame_counter;
                                FILE_LOG(logDEBUG) << "Frame " << frame_counter << ", " << elapsed.count() << " seconds - YEL_1 lamps detected illuminated: " << maskIllum;
                                zone = YEL_2;
                            }
                            }
                        break;
                        case YEL_2:
                            {
                            ctr_y = cvRound((float)_rows * tree.y2_y);
                            startrow = ctr_y - half_height;
                            if (startrow < 0) startrow = 0;
                            endrow = ctr_y + half_height;
                            if (endrow > _rows) endrow = _rows;
                            imgMask = imgTrk.rowRange(startrow, endrow);
                            maskIllum = mean(imgMask);
                            Rect roiMask(roiPrimary.x, roiPrimary.y + startrow, roiPrimary.width, endrow - startrow);
                            rectangle(imgProc, roiMask, CVCYAN, 2);
                            if (maskIllum[0] > tree.thresh_yel) {
                                //yel2_t = clock()/CLOCKS_PER_SEC;
                                yel2_t = std::chrono::high_resolution_clock::now();
                                std::chrono::duration<double> elapsed = yel2_t - heat_start_t;
                                yel3_t_exp = yel2_t + std::chrono::milliseconds(500);
                                //yel2_f = frame_counter;
                                FILE_LOG(logDEBUG) << "Frame " << frame_counter << ", " << elapsed.count() << " seconds - YEL_2 lamps detected illuminated: " << maskIllum;
                                zone = YEL_3;
                            }
                            }
                        break;
                        case YEL_3:
                            {
                            ctr_y = cvRound((float)_rows * tree.y3_y);
                            startrow = ctr_y - half_height;
                            if (startrow < 0) startrow = 0;
                            endrow = ctr_y + half_height;
                            if (endrow > _rows) endrow = _rows;
                            imgMask = imgTrk.rowRange(startrow, endrow);
                            maskIllum = mean(imgMask);
                            Rect roiMask(roiPrimary.x, roiPrimary.y + startrow, roiPrimary.width, endrow - startrow);
                            rectangle(imgProc, roiMask, CVCYAN, 2);
                            if (maskIllum[0] > tree.thresh_yel) {
                                //yel3_t = clock()/CLOCKS_PER_SEC;
                                yel3_t = std::chrono::high_resolution_clock::now();
                                std::chrono::duration<double> elapsed = yel3_t - heat_start_t;
                                grn_t_exp = yel3_t + std::chrono::milliseconds(500);
                                //yel3_f = frame_counter;
                                FILE_LOG(logDEBUG) << "Frame " << frame_counter << ", " << elapsed.count() << " seconds - YEL_3 lamps detected illuminated: " << maskIllum;
                                //FILE_LOG(logDEBUG) << "Launching at " << grn_t_exp;
                                zone = GRN;
                            }
                            }
                        break;
                        case GRN:
                            // don't actually have to wait to see the green lamp illuminate. just wait out the time and launch
                            {
                            //wait_grn_t = clock()/CLOCKS_PER_SEC;
                            while(1) {
                                curr_t = std::chrono::high_resolution_clock::now();
                                if (curr_t >= grn_t_exp) {
                                    #ifndef VM
                                    digitalWrite (BRKREL, HIGH) ;
                                    delay (2000) ;
                                    digitalWrite (BRKREL,  LOW) ;
                                    #endif
                                    FILE_LOG(logDEBUG) << " LAUNCH!!!  frame: " << frame_counter;
                                    break;
                                }
                            }
                            }
                        default:
                        break; 
                    }
                }
            }

            // 6. When tree is notified first YELLOW light illuminates, start a timer.
            //    Use timestamp updates to correct the estimate of when the GREEN light will illuminate.
            //    After the last yellow light illuminates and DRT calls that function on the tree,
            //    the tree will release the brake after the calculated delay.
            


        }

        // Reset variables, such as one-frame flags.
        b_manroi = false;
        b_halt_tracker = false; 
        b_frame_advance = false;

        /*** Timing and framerate ***/
        //end_t = clock()/CLOCKS_PER_SEC;
        end_t = std::chrono::high_resolution_clock::now();
        //frame_dt = (double)(end_t - start_t);
        frame_dt = end_t - start_t;
        //time_sum += frame_dt;

        // Limit the playback to no faster than 30 fps
        // if (frame_dt < 0.0333) usleep(33333 - (frame_dt * 1000000)); // 1000000 = one second

        /*** Placeholder - do stuff at timed intervals here ***/
        //   if (time_sum > 0.5) do something;
        /******************************************************/
        //if (time_sum.count() > 0.25) time_sum = std::chrono::duration<double>::zero();

        if (frame_dt.count() < 0.0333) {
            usleep(33333 - (frame_dt.count() * 1000000)); // 1000000 = one second
            end_t = std::chrono::high_resolution_clock::now();
            frame_dt = end_t - start_t;
        }
        fps = 1.0/frame_dt.count();


        /*** Do stuff at frame intervals here ***/
        //if (!(frame_counter % 5)) { // Only update the framerate every 5th frame
        //    if (m_fps_disp && !b_pause) {
        //        fps = 1.0/frame_dt;
        //    }
        //}

        // Framerate, frame counter display stuff
        if (m_fps_disp && !b_pause) {
            //if (!(frame_counter % 5)) fps = 1.0/frame_dt;
            char str[40];
            sprintf(str, "FPS  %.2f", fps);  
            putText(imgDisp, str, Point(10, 20), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
        } else if (b_pause) {
            putText(imgDisp, to_string(frame_counter), Point(10, 20), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
        }

        // Display overlay
        // Detection Mode onscreen indicator
        if (seq == SEARCHING) {
            putText(imgDisp, DetModeString(), Point(10, 50), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
            if (!m_filtimg_disp) imshow("Original", imgDisp);
            else imshow("Original", imgProc);
        } else if (seq == TRACKING) {
            //FILE_LOG(logDEBUG) << "roiPrimary X: " << roiPrimary.x;
            if (b_trk_success) {
                if (m_crop_roi_when_tracking) {
                    imshow("Original", imgMask);
                } else {
                    //putText(imgDisp, "tracking", Point(10, 50), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
                    if (m_filtimg_disp) {
                        rectangle(imgProc, roiPrimary, CVBLUE, 2);
                        imshow("Original", imgProc);
                    } else {
                        rectangle(imgDisp, roiPrimary, CVBLUE, 2);
                        imshow("Original", imgDisp);
                    }
                    //if (m_filtimg_disp) imshow("Filtered", imgZone);
                }
            } else {
                imshow("Original", imread("media/backgrounds/no_cal_img.png"));

            }
        }

        int key_val = waitKeyEx(10);

        switch(key_val) {
            case 27:    // esc - exit program
                cout << "escape key pressed" << endl;
                b_run = false;
                break;
            case 32:    // space bar - pause/play video
                b_pause = !b_pause;
                b_frame_advance = true;
                break;
            case 97:    // a - auto detect roi
                // NONE, MANUAL, PATTERN, HAAR
                DetModeCycle();
                cout << "Detection mode changed to " << DetModeString() << endl;
                FILE_LOG(logINFO) << "Detection mode changed to " << DetModeString();
                break;
            //case 122:   // z - iterate trackzone
            //    trkzoneidx++;
            //    if (trkzoneidx > 6) trkzoneidx = 0;
            //    break;
            case 113:   // q - restart the video
                cap.set( CAP_PROP_POS_AVI_RATIO, 0 );
                break;
            case 114:   // r - set roi
                if (seq == TRACKING) {
                    b_halt_tracker = true;
                } else if (det_mode == MANUAL) {
                    b_manroi = true;
                }
                break;
            case 65363: // -> fast forward 1 second or 1 frame if paused
                if (b_pause) {
                    b_frame_advance = true;
                } else {
                    frame_counter += 30;
                }
                cap.set( CAP_PROP_POS_FRAMES, frame_counter );
                break;
            case 65361: // <- rewind 1 second or 1 frame if paused
                if (b_pause) {
                    frame_counter -= 2;
                    b_frame_advance = true;
                } else {
                    frame_counter -= 30;
                }
                cap.set( CAP_PROP_POS_FRAMES, frame_counter );
                break;
            default:
                break;
        } // switch
    }

    destroyWindow("Original");
    //destroyWindow("Filtered");
    destroyWindow("Control");
    return;
}


/*** mouseEvent function outputs the x,y coordinates of mouse click ***/
void DRTVid::mouseEvent(int evt, int x, int y, int flags, void* param) 
{
    //Mat* rgb = (Mat*) param;
    if (evt == EVENT_LBUTTONDOWN) 
    {
        Vec3i* p_data = (Vec3i*)param;
        
        (*p_data)[0] = x;
        (*p_data)[1] = y;
        (*p_data)[2] = 1; // used to signal a click
 
        //printf("%d %d: %d, %d, %d\n", 
        //x, y, 
        //(int)(*rgb).at<Vec3b>(y, x)[0], 
        //(int)(*rgb).at<Vec3b>(y, x)[1], 
        //(int)(*rgb).at<Vec3b>(y, x)[2]); 
    }
}

void DRTVid::ExecSimplePlayback() {
    string vid_path = s_track_dir + "cal_vid.avi";
    ExecSimplePlayback(vid_path);
}

void DRTVid::ExecSimplePlayback(string s_vid_file) {
    cout << "Using OpenCV version " << CV_VERSION << endl << endl;
    cout << "space - pause\n"
         << "q - restart the video\n"
         << "-> fast forward 1/2 second\n"
         << "<- rewind 1/2 second\n";

    namedWindow("Original",  WINDOW_AUTOSIZE);
    moveWindow("Original", 0, 0);
    setWindowProperty("Original", WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);

    // Initialize logging - logERROR, logWARNING, logINFO, logDEBUG, logDEBUG1, logDEBUG2, logDEBUG3, logDEBUG
    FILE* pFile = fopen("log/drt.log", "w");
    Output2FILE::Stream() = pFile;
    FILELog::ReportingLevel() = LOG_LEVEL;
    FILE_LOG(logINFO) << "Opening log file.   OpenCV version  " << CV_VERSION << endl;

    // Initialize timing and framerate variables
    //double start_t, end_t, total_t;
    double fps = 30.0;              // The actual framerate.
    unsigned int frame_counter = 0;
    double frame_dt = 0;

    // Initialize flags for playback options
    bool b_pause = false; // SET
    bool b_fps_disp = true; // SET
    bool b_run = true;    // GET
    bool b_frame_advance = false;    // GET
    
    FILE_LOG(logINFO) << "    *****  Video Playback Mode  *****\n";
    FILE_LOG(logINFO) << "video file: " << s_vid_file;
    cap.open(s_vid_file);

    VisFrame frame( cap );
    chrono::time_point<std::chrono::high_resolution_clock> start;
    while ( b_run ) {
        /*** Refresh imgRaw and imgResized ***/
        if ( !b_pause || b_frame_advance ) {
            start = std::chrono::high_resolution_clock::now();

            // 1. VisFrame refresh, crop, rotate, resize
            frame.NextVidFrame(true, false);
            frame_counter = cap.get( CAP_PROP_POS_FRAMES );
        }

        // Reset variables, such as one-frame flags.
        b_frame_advance = false;

        /*** Timing and framerate ***/
        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;

        // Limit the playback to no faster than 30 fps
        if (elapsed.count() < 0.0333) { 
            usleep(33333 - (elapsed.count() * 1000000)); // 1000000 = one second
            finish = std::chrono::high_resolution_clock::now();
            elapsed = finish - start;
        }
        fps = 1.0/elapsed.count();

        // Framerate, frame counter display stuff
        if (b_fps_disp && !b_pause) {
            char str[40];
            sprintf(str, "FPS  %.4f", fps); 
            putText(frame.imgDisp, str, Point(10, 20), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
        } else if (b_pause) {
            putText(frame.imgDisp, to_string(frame_counter), Point(10, 20), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
        }

        // Display overlay
        imshow("Original", frame.imgDisp);

        int key_val = waitKeyEx(WAITKEYMILLIS);

        switch(key_val) {
            case 27:    // esc - exit program
                cout << "escape key pressed" << endl;
                b_run = false;
                break;
            case 32:    // space bar - pause/play video
                b_pause = !b_pause;
                b_frame_advance = true;
                break;
            case 113:   // q - restart the video
                cap.set( CAP_PROP_POS_AVI_RATIO, 0 );
                break;
            case 65363: // -> fast forward 1 second or 1 frame if paused
                if (b_pause) {
                    b_frame_advance = true;
                } else {
                    frame_counter += 30;
                }
                cap.set( CAP_PROP_POS_FRAMES, frame_counter );
                break;
            case 65361: // <- rewind 1 second or 1 frame if paused
                if (b_pause) {
                    frame_counter -= 2;
                    b_frame_advance = true;
                } else {
                    frame_counter -= 30;
                }
                cap.set( CAP_PROP_POS_FRAMES, frame_counter );
                break;
            default:
                break;
        } // switch
    }
    destroyWindow("Original");
    return;
}




void DRTVid::RunSelectCalImage(string s_vid_file) {
    cout << "Using OpenCV version " << CV_VERSION << endl << endl;
    cout << "space - pause\n"
         << "q - restart the video\n"
         << "-> fast forward 1/2 second\n"
         << "<- rewind 1/2 second\n";

    namedWindow("Original",  WINDOW_AUTOSIZE);
    moveWindow("Original", 0, 0);
    setWindowProperty("Original", WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);

    // Initialize logging - logERROR, logWARNING, logINFO, logDEBUG, logDEBUG1, logDEBUG2, logDEBUG3, logDEBUG
    FILE* pFile = fopen("log/drt.log", "w");
    Output2FILE::Stream() = pFile;
    FILELog::ReportingLevel() = LOG_LEVEL;
    FILE_LOG(logINFO) << "Opening log file.   OpenCV version  " << CV_VERSION << endl;

    // Initialize timing and framerate variables
    clock_t start_t, end_t, total_t;
    double fps = 30.0;              // The actual framerate.
    unsigned int frame_counter = 0;
    double time_sum = 0;
    double frame_dt = 0;

    // Initialize flags for playback options
    bool b_pause = false; // SET
    bool b_alt_disp = false; // SET
    bool b_fps_disp = true; // SET
    bool b_run = true;    // GET
    bool b_frame_advance = false;    // GET
    
    FILE_LOG(logINFO) << "    *****  Video Playback Mode  *****\n";
    FILE_LOG(logINFO) << "video file: " << s_vid_file;
    cap.open(s_vid_file);

    VisFrame frame( cap );

    while ( b_run ) {

        /*** Refresh imgRaw and imgResized ***/
        if ( !b_pause || b_frame_advance ) {
            start_t = clock();

            // 1. VisFrame refresh, crop, rotate, resize
            frame.NextVidFrame(true, false);
            frame_counter = cap.get( CAP_PROP_POS_FRAMES );
        }

        // Reset variables, such as one-frame flags.
        b_frame_advance = false;

        /*** Timing and framerate ***/
        end_t = clock();
        frame_dt = (double)(end_t - start_t) / (double)CLOCKS_PER_SEC;
        time_sum += frame_dt;

        // Limit the playback to no faster than 30 fps
        if (frame_dt < 0.0333) usleep(33333 - (frame_dt * 1000000)); // 1000000 = one second

        /*** Placeholder - do stuff at timed intervals here ***/
        //   if (time_sum > 0.5) do something;
        /******************************************************/
        if (time_sum > 0.25) time_sum = 0;

        /*** Do stuff at frame intervals here ***/
        if (!(frame_counter % 5)) { // Only output the framerate every 5th frame
            if (b_fps_disp && !b_pause) {
                end_t = clock();
                frame_dt = (double)(end_t - start_t) / (double)CLOCKS_PER_SEC;
                fps = 1.0/frame_dt;
            }
        }

        // Framerate, frame counter display stuff
        if (b_fps_disp && !b_pause) {
            if (!(frame_counter % 5)) fps = 1.0/frame_dt;
            char str[40];
            sprintf(str, "FPS  %.2f", fps);  
            putText(frame.imgDisp, str, Point(10, 20), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
        } else if (b_pause) {
            putText(frame.imgDisp, to_string(frame_counter), Point(10, 20), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
        }

        // Display overlay
        // Detection Mode onscreen indicator
        if (!b_alt_disp) imshow("Original", frame.imgDisp);
        //else imshow("Original", frame.imgThresh);

        int key_val = waitKeyEx(WAITKEYMILLIS);

        switch(key_val) {
            case 27:    // esc - exit program
                cout << "escape key pressed" << endl;
                b_run = false;
                break;
            case 32:    // space bar - pause/play video
                b_pause = !b_pause;
                b_frame_advance = true;
                break;
            case 100:    // d - toggle alt disp
                b_alt_disp = !b_alt_disp;
                break;
            case 113:   // q - restart the video
                cap.set( CAP_PROP_POS_AVI_RATIO, 0 );
                break;
            case 115:   // s - save the image
                if (b_pause) {
                    string save_path(s_track_dir + "cal_img.png");
                    imwrite(save_path, frame.imgRaw);
                    FILE_LOG(logINFO) << "Saving to file " << save_path;
                    putText(frame.imgDisp, "image saved", Point(10, 20), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
                    imshow("Original", frame.imgDisp);
                }
                break;
            case 65363: // -> fast forward 1 second or 1 frame if paused
                if (b_pause) {
                    b_frame_advance = true;
                } else {
                    frame_counter += 30;
                }
                cap.set( CAP_PROP_POS_FRAMES, frame_counter );
                break;
            case 65361: // <- rewind 1 second or 1 frame if paused
                if (b_pause) {
                    frame_counter -= 2;
                    b_frame_advance = true;
                } else {
                    frame_counter -= 30;
                }
                cap.set( CAP_PROP_POS_FRAMES, frame_counter );
                break;
            default:
                break;
        } // switch
    }
    destroyWindow("Original");
    return;
}

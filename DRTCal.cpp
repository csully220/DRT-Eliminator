#include "DRTCal.h"
#define CAMERA_MAX_RETRIES 5

using namespace cv;
using namespace std;

DRTCal::DRTCal(string trk_path) : s_track_dir(trk_path) {
    treeprofile = s_track_dir + "tree.cfg";
    FILE* pFile = fopen("log/drt.log", "w");
    Output2FILE::Stream() = pFile;
    FILELog::ReportingLevel() = LOG_LEVEL;
    FILE_LOG(logINFO) << "DRTCal Opening log file. Using OpenCV version  " << CV_VERSION << endl;
}

/***************************************/
void DRTCal::on_trackbar( int pos, void* )
{
    cout << "Debug level: " << pos << endl;
}

/*** mouseEvent function outputs the x,y coordinates of mouse click ***/
void DRTCal::mouseEvent(int evt, int x, int y, int flags, void* param) 
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


void DRTCal::RunCameraSettings() {
    FILE_LOG(logINFO) << "DRTCal::CameraSettings";
    /************  Main object instantiation  **********************/

    INIReader reader( "./data/camera.cfg" );
    if (reader.ParseError() < 0) {
        FILE_LOG(logERROR) << "Failed to load camera.cfg file";
        return;
    }

    int brt = 0;
    int con = 0;
    int sat = 0;

    string section = "camera";
    brt = reader.GetInteger(  section, "BRIGHTNESS",  0);
    con = reader.GetInteger(  section, "CONTRAST",    0);
    sat = reader.GetInteger(  section, "SATURATION",  0);

    int brtprv = brt;
    int conprv = con;
    int satprv = sat;

    int brtmax = 64;
    int brtmin = -64;
    int conmax = 64;
    int conmin = 0;
    int satmax = 128;
    int satmin = 0;

    VideoCapture cap;
    int i = 0;
    while (!cap.open(i)) {
        i++;
        if ( i == 5 ){
            FILE_LOG(logERROR) << "Failed to open the camera";
            pthread_exit(NULL);
        }
    }
    // Create the window
    cvui::init("Camera");
    moveWindow("Camera", 0, 0);

    bool b_run = true;
    bool b_refcamset = false; // refresh camera settings


    Mat imgRaw;

    while (b_run) {
        if (cap.read(imgRaw)) {

            cvui::text(imgRaw, COL2 - 100, ROW1, "Brightness", 0.6, CVUIGRAY); //Higher is more sensitive
            cvui::counter(imgRaw, COL1, ROW1, &brt, 1, "%d");
                if (brt > brtmax) brt = brtmax;
                if (brt < brtmin ) brt = brtmin;

            cvui::text(imgRaw, COL2 - 100, ROW2, "Contrast", 0.6, CVUIGRAY); //Higher is more sensitive
            cvui::counter(imgRaw, COL1, ROW2, &con, 1, "%d");
                if (con > conmax) con = brtmax;
                if (con < brtmin ) con = brtmin;

            cvui::text(imgRaw, COL2 - 100, ROW3, "Saturation", 0.6, CVUIGRAY); //Higher is more sensitive
            cvui::counter(imgRaw, COL1, ROW3, &sat, 1, "%d");
                if (sat > satmax) sat = satmax;
                if (sat < satmin) sat = satmin;

            if (brt != brtprv || con != conprv || sat != satprv) {
                cap.set(CAP_PROP_BRIGHTNESS, brt);  // -64 to 64
                cap.set(CAP_PROP_CONTRAST, con);  // 0-64
                cap.set(CAP_PROP_SATURATION, sat);  // 0-128
                brtprv = brt;
                conprv = con;
                satprv = sat;
            }

            if (cvui::button(imgRaw, COL1, ROW12, 100, 20, string("Back"))) {
                 b_run = false;
                 break;
            }
            if (cvui::button(imgRaw, COL2, ROW12, 100, 20, string("Save"))) {
                ofstream camcfgfile;
                camcfgfile.open("./data/camera.cfg");
                if (camcfgfile) {
                    camcfgfile << "[camera]"
                             << "\nBRIGHTNESS = " << brt
                             << ";\nCONTRAST = " << con
                             << ";\nSATURATION = " << sat << ";\n";
                    camcfgfile.close();
                    FILE_LOG(logINFO) << "Saved camera.cfg file";
                    b_run = false;
                    break;
                } else {
                    FILE_LOG(logERROR) << "Error saving camera.cfg file";
                }

            }

            cvui::imshow("Camera", imgRaw);
            int key_val = waitKeyEx(30);
            switch(key_val) {
                case 27:    // esc - exit program
                    b_run = false;
                    break;
                default:
                    break;
            } 
        } else {
            FILE_LOG(logERROR) << "Failed to get image from camera";
            b_run = false;
        }
    }
    destroyWindow("Camera");
}


void DRTCal::RunSetSensitivity() {
    string s_svfile = s_track_dir + "tree.cfg";
    RunSetSensitivity(s_svfile);
}

void DRTCal::RunSetSensitivity(string s_svfile) {
    FILE_LOG(logINFO) << "DRTCal::RunSetSensitivity";
    /************  Main object instantiation  **********************/
    VisFrame frame;
    XmasTree cal_tree;

    bool b_run = true;
    bool b_first_cal = false;
    if (!cal_tree.init(treeprofile)) {
        cal_tree.name = "Calibrated";
        cal_tree.location = "Default";
        b_first_cal = true;
    }

    // Create the window
    cvui::init("Calibrate");
    moveWindow("Calibrate", 0, 0);

    if(b_first_cal) {
        cal_tree.dp         = 1;
        cal_tree.min_dist   = 20;
        cal_tree.canny_edge = 200;
        cal_tree.center_det = 20;
        cal_tree.min_rad    = 4;
        cal_tree.max_rad    = 20;
    }

    int dp             = cal_tree.dp;
    int min_dist       = cal_tree.min_dist;
    int canny_edge     = cal_tree.canny_edge;
    int ac             = cal_tree.center_det;
    int min_rad        = cal_tree.min_rad;
    int max_rad        = cal_tree.max_rad;
    int rad_pm         = (max_rad - min_rad)/2;
    int nominal_radius = cal_tree.nominal_radius;
    int thresh_stg     = cal_tree.thresh_stg;
    int thresh_yel     = cal_tree.thresh_yel;
    double mask_ht      = cal_tree.mask_ht;

    Mat imgMenuBg = imread(string(BGIMAGE));

    while (b_run) {
        imgMenuBg = imread(string(BGIMAGE));
        cvui::text(imgMenuBg, COL1, ROW1, "Lamp Detection Sensitivity", 1.0, CVUIGRAY);
        
        cvui::text(imgMenuBg, COL2 - 100, ROW3, "Min distance between circles", 0.6, CVUIGRAY); //Higher is more sensitive
        cvui::counter(imgMenuBg, COL1, ROW3, &min_dist, 1, "%d");
            if (min_dist > 40) min_dist = 40;
            if (min_dist < 2 ) min_dist = 2;

        cvui::text(imgMenuBg, COL2 - 100, ROW4, "Circle radius range", 0.6, CVUIGRAY); //Higher is more sensitive
        cvui::counter(imgMenuBg, COL1, ROW4, &rad_pm, 1, "%d");
            if (rad_pm > 20) rad_pm = 40;
            if (rad_pm < 0 ) rad_pm = 0;

        cvui::text(imgMenuBg, COL2 - 100, ROW5, "Accum resolution", 0.6, CVUIGRAY); //Higher is more sensitive
        cvui::counter(imgMenuBg, COL1, ROW5, &dp, 1, "%d");
            if (dp > 5) dp = 5;
            if (dp < 0) dp = 0;

        cvui::text(imgMenuBg, COL2 - 100, ROW6, "Canny edge", 0.6, CVUIGRAY); //Lower is more sensitive
        cvui::counter(imgMenuBg, COL1, ROW6, &canny_edge, 1, "%d");
            if (canny_edge > 400)  canny_edge = 400;
            if (canny_edge < 50) canny_edge = 50;

        cvui::text(imgMenuBg, COL2 - 100, ROW7, "Accum threshold", 0.6, CVUIGRAY); //Lower is more sensitive
        cvui::counter(imgMenuBg, COL1, ROW7, &ac, 1, "%d");
            if (ac > 50) ac = 50;
            if (ac < 5 ) ac = 5;
        
        cvui::text(imgMenuBg, COL2 - 100, ROW8, "Staging lamp threshold", 0.6, CVUIGRAY); //Lower is more sensitive
        cvui::counter(imgMenuBg, COL1, ROW8, &thresh_stg, 1, "%d");
            if (thresh_stg > 60) thresh_stg = 60;
            if (thresh_stg < 5 ) thresh_stg = 5;

        cvui::text(imgMenuBg, COL2 - 100, ROW9, "Yellow lamp threshold", 0.6, CVUIGRAY); //Lower is more sensitive
        cvui::counter(imgMenuBg, COL1, ROW9, &thresh_yel, 1, "%d");
            if (thresh_yel > 60) thresh_yel = 60;
            if (thresh_yel < 5 ) thresh_yel = 5;

        cvui::text(imgMenuBg, COL2 - 100, ROW10, "Mask height adjustment", 0.6, CVUIGRAY); //Lower is more sensitive
        cvui::counter(imgMenuBg, COL1, ROW10, &mask_ht, 0.1, "%.1f");
            if (mask_ht > 3.0) mask_ht = 3.0;
            if (mask_ht < 0.1 ) mask_ht = 0.1;

        if (cvui::button(imgMenuBg, COL1, ROW12, 100, 20, string("Back"))) {
             b_run = false;
             break;
        }
        if (cvui::button(imgMenuBg, COL2, ROW12, 100, 20, string("Save"))) {
             cal_tree.dp = dp;
             cal_tree.min_dist = min_dist;
             cal_tree.canny_edge = canny_edge;
             cal_tree.center_det = ac;
             cal_tree.min_rad = nominal_radius - rad_pm;
             cal_tree.max_rad = nominal_radius + rad_pm;
             cal_tree.thresh_stg = thresh_stg;
             cal_tree.thresh_yel = thresh_yel;
             cal_tree.mask_ht    = (float)mask_ht;
             cal_tree.saveToFile(s_svfile);
             b_run = false;
             break;
        }


        cvui::imshow("Calibrate", imgMenuBg);

        int key_val = waitKeyEx(30);

        switch(key_val) {
            case 27:    // esc - exit program
                cout << "esc key is pressed by user" << endl;
                b_run = false;
                break;
            default:
                break;
        } //switch
    }
    destroyWindow("Calibrate");
}


void DRTCal::RunViewCalSettings() {
    RunViewCalSettings(s_track_dir + "cal_img.png");
}

void DRTCal::RunViewCalSettings(string s_imgfile) {
    FILE_LOG(logINFO) << "DRTCal->RunViewCalSettings";
    if (s_imgfile.empty()) {
        FILE_LOG(logERROR) << "RunViewCalSettings called without specifying image";
        return;
    }
    FILE_LOG(logINFO) << "loading image " << s_imgfile;

    /************  Main object instantiation  **********************/
    VisFrame frame;

    if (!tree.init(treeprofile)) {
        FILE_LOG(logERROR) << treeprofile << " not found." << s_track_dir; 
        return;
    }

    cvui::init("Calibrate");
    //setWindowProperty("Calibrate", WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
    /********* Windows *******************************************/

    Point LP_top_left;
    int LP_width;

    enum { LAMPS, FILTER } cal_view;
    cal_view = LAMPS;

    bool b_run = true;
    while (b_run) {
        frame.LoadImage(s_imgfile, true, true);
        
        if (cal_view == LAMPS) {
            if (cvui::button(frame.imgDisp, 370, 10, 100, 20, string("Filter"))) {
                cal_view = FILTER;
            }
            if (cvui::button(frame.imgDisp, 10, 10, 100, 20, string("Back"))) {
                b_run = false;
                break;
            }
            putText(frame.imgDisp, "view cal settings", Point(10, 60), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
            Point LP_top_left(tree.calimg_tl_x, tree.calimg_tl_y);
            vector<Point> lamp_points = tree.getLampPoints(LP_top_left, tree.calimg_w);

            //FILE_LOG(logDEBUG4) << " nominal radius " << tree.nominal_radius; 
            //FILE_LOG(logDEBUG4) << " aspect ratio   " << tree.aspect_ratio;
            
            for( vector<Point>::iterator it = lamp_points.begin(); it != lamp_points.end(); ++it ) {
                circle( frame.imgDisp, *it, tree.nominal_radius, CVPURPLE, 3, 8, 0 ); // circle outline
            }
            cvui::imshow("Calibrate", frame.imgDisp);
        } else if (cal_view == FILTER) {
            HSVFilterVals hsv_wht(tree.wLowH, tree.wHighH, tree.wLowS, tree.wHighS, tree.wLowV, tree.wHighV);
            HSVFilterVals hsv_yel(tree.yLowH, tree.yHighH, tree.yLowS, tree.yHighS, tree.yLowV, tree.yHighV);
            frame.Threshold(true, true, hsv_wht, hsv_yel);
            if (cvui::button(frame.imgThresh, 370, 10, 100, 20, string("Geometry"))) {
                cal_view = LAMPS;
            }
            if (cvui::button(frame.imgDisp, 10, 10, 100, 20, string("Back"))) {
                b_run = false;
                break;
            }
            cvui::imshow("Calibrate", frame.imgThresh);
        }
        int key_val = waitKeyEx(WAITKEYMILLIS);
        // if(key_val != -1) cout << key_val << endl;

        switch(key_val) {
            case 27:    // esc - exit program
                cout << "esc key is pressed by user" << endl;
                b_run = false;
                break;
        }

    }
    destroyWindow("Calibrate");
}


void DRTCal::RunImageCalibrate( bool b_do_cal_geometry, bool b_do_cal_filter ) {
    RunImageCalibrate(b_do_cal_geometry, b_do_cal_filter, s_track_dir + "cal_img.png"); 
}

void DRTCal::RunImageCalibrate( bool b_do_cal_geometry, bool b_do_cal_filter, string s_imgfile ) {
    RunImageCalibrate(b_do_cal_geometry, b_do_cal_filter, s_imgfile, s_track_dir + "tree.cfg"); 
}

void DRTCal::RunImageCalibrate( bool b_do_cal_geometry, bool b_do_cal_filter, string s_imgfile, string s_svfile ) {
    FILE_LOG(logINFO) << "DRTCal->RunImageCalibrate";
    FILE_LOG(logINFO) << "loading image " << s_imgfile;

    if (s_imgfile.empty()) {
        FILE_LOG(logERROR) << "RunImageCalibrate called without specifying image";
        return;
    }

    /************  Main object instantiation  **********************/
    VisFrame frame;
    XmasTree cal_tree;

    bool b_first_cal = false; 
    if (!cal_tree.init(treeprofile)) {
        cal_tree.name = "Calibrated";
        cal_tree.location = "Default";
        b_first_cal = true;
    }
    /************  Main object instantiation  **********************/

    bool b_run = true;
    bool b_filter = false;
    bool b_morph = true;
    bool b_blur = true;

    // Create trackbars in "Control" window
    int wLowH = 0;
    int wHighH = 38;
    int wLowS = 0;
    int wHighS = 30;
    int wLowV = 240;
    int wHighV = 255;

    int yLowH = 0;
    int yHighH = 38;
    int yLowS = 0;
    int yHighS = 30;
    int yLowV = 240;
    int yHighV = 255;

    int iRadius = 12;

    int wHue = 0;
    int wSat = 0;
    int wVal = 0;

    int yHue = 0;
    int ySat = 0;
    int yVal = 0;

    enum LAMP_POINT { PS1, PS2, PS3, PS4, ST1, ST2, ST3, ST4, Y1L, Y1R, Y2L, Y2R, Y3L, Y3R, GL, GR, RL, RR} lamp_point;
    enum CAL_STEP { GEOMETRY, NORMALIZE, RADIUS, FILTER, DONE } cal_step;
    enum FILTER_COLOR { WHITE, YELLOW, REVIEW } filter_color;
    lamp_point = PS1;
 
    if (b_do_cal_geometry || b_first_cal) cal_step = GEOMETRY;
    else cal_step = FILTER;

    filter_color = WHITE;

    Point ps[4];
    Point st[4];
    Point y1[2];
    Point y2[2];
    Point y3[2];
    Point grn[2];
    Point red[2];

    string s_cal_step = "prestage 1";

    /********* Windows *******************************************/
    //namedWindow("Calibrate", WINDOW_AUTOSIZE);
    cvui::init("Calibrate");
    //setWindowProperty("Calibrate", WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
    /********* Windows *******************************************/

    Point LP_top_left;
    int LP_width;

    while (b_run) {
        frame.LoadImage(s_imgfile, true, true);
       
        // click top left prestage
        if (cal_step == GEOMETRY) { 
            if (cvui::button(frame.imgDisp, 370, 10, 100, 20, string("Restart"))) {
                 lamp_point = PS1;
	         s_cal_step = "prestage 1";
            }
            if (cvui::button(frame.imgDisp, 10, 10, 100, 20, string("Back"))) {
                 b_run = false;
                 break;
            }
            if (cvui::mouse(cvui::LEFT_BUTTON, cvui::CLICK)) {
                Point cursor = cvui::mouse();
                int x = cursor.x;
                int y = cursor.y;
                int hue = frame.imgRaw.at<Vec3b>(y, x)[0]; 
                int sat = frame.imgRaw.at<Vec3b>(y, x)[1]; 
                int val = frame.imgRaw.at<Vec3b>(y, x)[2];
                FILE_LOG(logDEBUG) << "Calibration click: [" << x << ", " << y << "]" << "HSV: " << hue << ", " << sat << ", " << val;
                // Get click positions for each lamp
                switch (lamp_point) {
                    case PS1:
	                ps[0] = Point(x, y);
                        LP_top_left = Point(x,y);
	                lamp_point = PS2;
	                s_cal_step = "prestage 2";
                        break;
                    case PS2:
	                ps[1] = Point(x, y);
	                lamp_point = PS3;
	                s_cal_step = "prestage 3";
                        break;
                    case PS3:
	                ps[2] = Point(x, y);
	                lamp_point = PS4;
	                s_cal_step = "prestage 4";
                        break;
                    case PS4:
	                ps[3] = Point(x, y);
	                lamp_point = ST1;
	                s_cal_step = "stage 1";
                        break;
                    case ST1:
	                st[0] = Point(x, y);
	                lamp_point = ST2;
	                s_cal_step = "stage 2";
                        break;
                    case ST2:
	                st[1] = Point(x, y);
	                lamp_point = ST3;
	                s_cal_step = "stage 3";
                        break;
                    case ST3:
	                st[2] = Point(x, y);
	                lamp_point = ST4;
	                s_cal_step = "stage 4";
                        break;
                    case ST4:
	                st[3] = Point(x, y);
	                lamp_point = Y1L;
	                s_cal_step = "y1 left";
                        break;
                    case Y1L:
	                y1[0] = Point(x, y);
	                lamp_point = Y1R;
	                s_cal_step = "y1 right";
                        break;
                    case Y1R:
	                y1[1] = Point(x, y);
	                lamp_point = Y2L;
	                s_cal_step = "y2 left";
                        break;
                    case Y2L:
	                y2[0] = Point(x, y);
	                lamp_point = Y2R;
	                s_cal_step = "y2 right";
                        break;
                    case Y2R:
	                y2[1] = Point(x, y);
	                lamp_point = Y3L;
	                s_cal_step = "y3 left";
                        break;
                    case Y3L:
	                y3[0] = Point(x, y);
	                lamp_point = Y3R;
	                s_cal_step = "y3 right";
                        break;
                    case Y3R:
	                y3[1] = Point(x, y);
	                lamp_point = GL;
	                s_cal_step = "grn left";
                        break;
	            case GL:
	                grn[0] = Point(x, y);
	                lamp_point = GR;
	                s_cal_step = "grn right";
                        break;
	            case GR:
	                grn[1] = Point(x, y);
	                lamp_point = RL;
	                s_cal_step = "red left";
                        break;
	            case RL:
	                red[0] = Point(x, y);
	                lamp_point = RR;
	                s_cal_step = "red right";
                        break;
	            case RR:
	                red[1] = Point(x, y);
	                s_cal_step = "normalizing";
                        cal_step = NORMALIZE;
                        break;
                }
            } 

            putText(frame.imgDisp, s_cal_step, Point(10, 60), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
            cvui::imshow("Calibrate", frame.imgDisp);
        }
        if (cal_step == NORMALIZE) {
            putText(frame.imgDisp, "normalizing", Point(10, 60), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
            // Calculate averages for columns and rows, scale
            int ps_y_avg = 0;
            int st_y_avg = 0;
            int y1_y_avg = 0;
            int y2_y_avg = 0;
            int y3_y_avg = 0;
            int grn_y_avg = 0;
            int red_y_avg = 0;

            int st1_x_avg = 0;
            int st2_x_avg = 0;
            int st3_x_avg = 0;
            int st4_x_avg = 0;
            int left_col_x_avg = 0;
            int right_col_x_avg = 0;

            int ps_y_sum = 0;
            for (int i = 0; i < 4; ++i) ps_y_sum += ps[i].y;

            int st_y_sum = 0;
            for (int i = 0; i < 4; ++i) st_y_sum += st[i].y;

            ps_y_avg = cvRound((float)ps_y_sum/4);
            FILE_LOG(logDEBUG4) << "PS_Y_AVG         " << ps_y_avg; 
            st_y_avg = cvRound((float)st_y_sum/4);
            FILE_LOG(logDEBUG4) << "ST_Y_AVG         " << st_y_avg; 

            y1_y_avg = cvRound((float)(y1[0].y + y1[1].y)/2.0);
            FILE_LOG(logDEBUG4) << "Y1_Y_AVG         " << y1_y_avg; 
            y2_y_avg = cvRound((float)(y2[0].y + y2[1].y)/2.0);
            FILE_LOG(logDEBUG4) << "Y2_Y_AVG         " << y2_y_avg; 
            y3_y_avg = cvRound((float)(y3[0].y + y3[1].y)/2.0);
            FILE_LOG(logDEBUG4) << "Y3_Y_AVG         " << y3_y_avg; 
            grn_y_avg = cvRound((float)(grn[0].y + grn[1].y)/2.0);
            FILE_LOG(logDEBUG4) << "GRN_Y_AVG        " << grn_y_avg; 
            red_y_avg = cvRound((float)(red[0].y + red[1].y)/2.0);
            FILE_LOG(logDEBUG4) << "RED_Y_AVG        " << red_y_avg; 
            
            st1_x_avg = cvRound((float)(ps[0].x + st[0].x)/2.0);
            FILE_LOG(logDEBUG4) << "ST1_X_AVG        " << st1_x_avg; 
            st2_x_avg = cvRound((float)(ps[1].x + st[1].x)/2.0);
            FILE_LOG(logDEBUG4) << "ST2_X_AVG        " << st2_x_avg; 
            st3_x_avg = cvRound((float)(ps[2].x + st[2].x)/2.0);
            FILE_LOG(logDEBUG4) << "ST3_X_AVG        " << st3_x_avg; 
            st4_x_avg = cvRound((float)(ps[3].x + st[3].x)/2.0);
            FILE_LOG(logDEBUG4) << "ST4_X_AVG        " << st4_x_avg; 
            
            left_col_x_avg  = cvRound((float)(y1[0].x + y2[0].x + y3[0].x + grn[0].x + red[0].x)/5.0);
            FILE_LOG(logDEBUG4) << "LEFT_COL_X_AVG   " << left_col_x_avg; 
            right_col_x_avg = cvRound((float)(y1[1].x + y2[1].x + y3[1].x + grn[1].x + red[1].x)/5.0);
            FILE_LOG(logDEBUG4) << "RIGHT_COL_X_AVG  " << right_col_x_avg;

            float width  = st4_x_avg - st1_x_avg;
            LP_width = width;
            float height = red_y_avg - ps_y_avg;
            float aspect_ratio = height/width;
            cal_tree.aspect_ratio = aspect_ratio;
            float _ps_y = 0.0;
            cal_tree.prestage_y = _ps_y;
            FILE_LOG(logDEBUG) << "PRESTAGE_Y " << cal_tree.prestage_y; 
            float _st_y = (float)(st_y_avg - ps_y_avg)/height;
            cal_tree.stage_y = _st_y;
            FILE_LOG(logDEBUG) << "STAGE_Y    " << cal_tree.stage_y; 
            float _y1_y = (float)(y1_y_avg - ps_y_avg)/height;
            cal_tree.y1_y = _y1_y;
            FILE_LOG(logDEBUG) << "Y1_Y       " << cal_tree.y1_y; 
            float _y2_y = (float)(y2_y_avg - ps_y_avg)/height;
            cal_tree.y2_y = _y2_y;
            FILE_LOG(logDEBUG) << "Y2_Y       " << cal_tree.y2_y; 
            float _y3_y = (float)(y3_y_avg - ps_y_avg)/height;
            cal_tree.y3_y = _y3_y;
            FILE_LOG(logDEBUG) << "Y3_Y       " << cal_tree.y3_y; 
            float _grn_y = (float)(grn_y_avg - ps_y_avg)/height;
            cal_tree.grn_y = _grn_y;
            FILE_LOG(logDEBUG) << "GRN_Y      " << cal_tree.grn_y; 
            float _red_y = (float)(red_y_avg - ps_y_avg)/height;
            cal_tree.red_y = _red_y;
            FILE_LOG(logDEBUG) << "RED_Y      " << cal_tree.red_y; 

            float _st1_x = 0.0;
            cal_tree.st1_x = _st1_x;
            FILE_LOG(logDEBUG) << "ST1_X      " << cal_tree.st1_x; 
            float _st2_x = (float)(st2_x_avg - st1_x_avg)/width;
            cal_tree.st2_x = _st2_x;
            FILE_LOG(logDEBUG) << "ST2_X      " << cal_tree.st2_x; 
            float _st3_x = (float)(st3_x_avg - st1_x_avg)/width;
            cal_tree.st3_x = _st3_x;
            FILE_LOG(logDEBUG) << "ST3_X      " << cal_tree.st3_x; 
            float _st4_x = (float)(st4_x_avg - st1_x_avg)/width;
            cal_tree.st4_x = _st4_x;
            FILE_LOG(logDEBUG) << "ST4_X      " << cal_tree.st4_x; 
            float _left_col_x  = (float)(left_col_x_avg - st1_x_avg)/width;
            cal_tree.column1_x = _left_col_x;
            FILE_LOG(logDEBUG) << "COLUMN1_X  " << cal_tree.column1_x; 
            float _right_col_x = (float)(right_col_x_avg - st1_x_avg)/width;
            cal_tree.column2_x = _right_col_x;
            FILE_LOG(logDEBUG) << "COLUMN2_X  " << cal_tree.column2_x; 

            cal_step = RADIUS;
        }

        if (cal_step == RADIUS) {
            if (cvui::button(frame.imgDisp, 370, 10, 100, 20, string("Next"))) {
                 cal_tree.nominal_radius = iRadius;
                 cal_tree.relative_radius = (float)iRadius/(float)LP_width;
                 if (b_do_cal_filter || b_first_cal) cal_step = FILTER;
                 else cal_step = DONE;
            }
            if (cvui::button(frame.imgDisp, 10, 10, 100, 20, string("Back"))) {
                 b_run = false;
                 break;
            }
            putText(frame.imgDisp, "radius", Point(10, 60), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
            cvui::counter(frame.imgDisp, 10, 70, &iRadius, 1, "%d");
                if (iRadius > 30) iRadius = 30;
                if (iRadius < 6 ) iRadius = 6;
            vector<Point> lamp_points = cal_tree.getLampPoints(LP_top_left, LP_width);
            for( vector<Point>::iterator it = lamp_points.begin(); it != lamp_points.end(); ++it ) {
                circle( frame.imgDisp, *it, iRadius, CVPURPLE, 3, 8, 0 ); // circle outline
            }
            cvui::imshow("Calibrate", frame.imgDisp);
        }

        if (cal_step == FILTER) {
            switch (filter_color) {
                case WHITE:
                    if (cvui::button(frame.imgDisp, 10, 10, 100, 20, string("Back"))) {
                         b_run = false;
                         break;
                    }
                    putText(frame.imgDisp, "filter", Point(10, 60), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
                    putText(frame.imgDisp, "click white", Point(10, 100), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);

                    if (cvui::mouse(cvui::LEFT_BUTTON, cvui::CLICK)) {
                        Point cursor = cvui::mouse();
                        int x = cursor.x;
                        int y = cursor.y;
                        int hue = frame.imgRaw.at<Vec3b>(y, x)[0];
                        int sat = frame.imgRaw.at<Vec3b>(y, x)[1];
                        int val = frame.imgRaw.at<Vec3b>(y, x)[2];
                        //FILE_LOG(logDEBUG) << "Calibration click: [" << x << ", " << y << "]" << "HSV: " << hue << ", " << sat << ", " << val;
                        /***   White hue between 0 and 40   ***/
                        wLowH = 0;
                        wHighH = hue + 5;
                        /***   White sat between 0 and 40   ***/
                        wLowS = 0;
                        wHighS = sat + 5;
                        /***   White value  between 250 and 255   ***/
                        wLowV = val - 10;
                            if (wLowV < 0) wLowV = 0;
                        wHighV = 255;
                        //wHighV = wVal + 10;
                        //if (wHighV > 255) wHighV = 255;
                        filter_color = YELLOW;
                    }
                    cvui::imshow("Calibrate", frame.imgDisp);
                    break;
                case YELLOW:
                    if (cvui::button(frame.imgDisp, 10, 10, 100, 20, string("Back"))) {
                         b_run = false;
                         break;
                    }
                    putText(frame.imgDisp, "filter", Point(10, 60), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
                    putText(frame.imgDisp, "click yellow", Point(10, 100), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);

                    if (cvui::mouse(cvui::LEFT_BUTTON, cvui::CLICK)) {
                        Point cursor = cvui::mouse();
                        int x = cursor.x;
                        int y = cursor.y;
                        int hue = frame.imgRaw.at<Vec3b>(y, x)[0];
                        int sat = frame.imgRaw.at<Vec3b>(y, x)[1];
                        int val = frame.imgRaw.at<Vec3b>(y, x)[2];
                        //FILE_LOG(logDEBUG) << "Calibration click: [" << x << ", " << y << "]" << "HSV: " << hue << ", " << sat << ", " << val;
                        /***   White hue between 0 and 40   ***/
                        yLowH = hue - 5;
                        yHighH = hue + 5;
                        /***   White sat between 0 and 40   ***/
                        yLowS = 0;
                        yHighS = 255;
                        /***   White value  between 250 and 255   ***/
                        yLowV = val - 10;
                            if (yLowV < 0) yLowV = 0;
                        yHighV = 255;
                        //wHighV = wVal + 10;
                        //if (wHighV > 255) wHighV = 255;
                        filter_color = REVIEW;
                    }
                    cvui::imshow("Calibrate", frame.imgDisp);
                    break;
                case REVIEW:
                    HSVFilterVals hsv_wht(wLowH, wHighH, wLowS, wHighS, wLowV, wHighV);
                    HSVFilterVals hsv_yel(yLowH, yHighH, yLowS, yHighS, yLowV, yHighV);
                    frame.Threshold(true, true, hsv_wht, hsv_yel);

                    if (cvui::button(frame.imgThresh, 370, 10, 100, 20, string("Next"))) {
                         cal_tree.wLowH = wLowH;
                         cal_tree.wHighH = wHighH;
                         cal_tree.wLowS = wLowS;
                         cal_tree.wHighS = wHighS;
                         cal_tree.wLowV = wLowV;
                         cal_tree.wHighV = wHighV;

                         cal_tree.yLowH = yLowH;
                         cal_tree.yHighH = yHighH;
                         cal_tree.yLowS = yLowS;
                         cal_tree.yHighS = yHighS;
                         cal_tree.yLowV = yLowV;
                         cal_tree.yHighV = yHighV;

                         cal_step = DONE;
                    }
                    if (cvui::button(frame.imgThresh, 370, 40, 100, 20, string("Redo"))) {
                         filter_color = WHITE;
                    }
                    if (cvui::button(frame.imgThresh, 10, 10, 100, 20, string("Back"))) {
                         b_run = false;
                         break;
                    }
                    putText(frame.imgThresh, "filter", Point(10, 60), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
                    putText(frame.imgThresh, "accept?", Point(10, 100), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
                   
                    putText(frame.imgThresh, "white", Point(10, 400), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
                    cvui::counter(frame.imgThresh, 10, 440, &wLowH, 1, "%d");
                        if (wLowH > 255) wLowH = 255;
                        if (wLowH < 0 ) wLowH = 0;
                    cvui::counter(frame.imgThresh, 10, 470, &wHighH, 1, "%d");
                        if (wHighH > 255) wHighH = 255;
                        if (wHighH < 0 ) wHighH = 0;
                    cvui::counter(frame.imgThresh, 10, 500, &wLowS, 1, "%d");
                        if (wLowS > 255) wLowS = 255;
                        if (wLowS < 0 ) wLowS = 0;
                    cvui::counter(frame.imgThresh, 10, 530, &wHighS, 1, "%d");
                        if (wHighS > 255) wHighS = 255;
                        if (wHighS < 0 ) wHighS = 0;
                    cvui::counter(frame.imgThresh, 10, 560, &wLowV, 1, "%d");
                        if (wLowV > 255) wLowV = 255;
                        if (wLowV < 0 ) wLowV = 0;
                    cvui::counter(frame.imgThresh, 10, 590, &wHighV, 1, "%d");
                        if (wHighV > 255) wHighV = 255;
                        if (wHighV < 0 ) wHighV = 0;

                    putText(frame.imgThresh, "yellow", Point(370, 400), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
                    cvui::counter(frame.imgThresh, 370, 440, &yLowH, 1, "%d");
                        if (yLowH > 255) yLowH = 255;
                        if (yLowH < 0 ) yLowH = 0;
                    cvui::counter(frame.imgThresh, 370, 470, &yHighH, 1, "%d");
                        if (yHighH > 255) yHighH = 255;
                        if (yHighH < 0 ) yHighH = 0;
                    cvui::counter(frame.imgThresh, 370, 500, &yLowS, 1, "%d");
                        if (yLowS > 255) yLowS = 255;
                        if (yLowS < 0 ) yLowS = 0;
                    cvui::counter(frame.imgThresh, 370, 530, &yHighS, 1, "%d");
                        if (yHighS > 255) yHighS = 255;
                        if (yHighS < 0 ) yHighS = 0;
                    cvui::counter(frame.imgThresh, 370, 560, &yLowV, 1, "%d");
                        if (yLowV > 255) yLowV = 255;
                        if (yLowV < 0 ) yLowV = 0;
                    cvui::counter(frame.imgThresh, 370, 590, &yHighV, 1, "%d");
                        if (yHighV > 255) yHighV = 255;
                        if (yHighV < 0 ) yHighV = 0;

                    putText(frame.imgThresh, "Hue", Point(200, 465), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
                    putText(frame.imgThresh, "Sat", Point(200, 525), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);
                    putText(frame.imgThresh, "Brt", Point(200, 585), FONT_HERSHEY_SIMPLEX, 0.6, CVWHITE, 2);

                    cvui::imshow("Calibrate", frame.imgThresh);
                    break;
            }
        }
        
        if (cal_step == DONE) {
            if (cvui::button(frame.imgDisp, 10, 10, 100, 20, string("Back"))) {
                cout << "exiting image calibration\n";
                b_run = false;
                break;
            }
            if (cvui::button(frame.imgDisp, 370, 10, 100, 20, string("Save"))) {
                 if(b_first_cal) {
                     cal_tree.dp = 1;
                     cal_tree.canny_edge = 200;
                     cal_tree.center_det = 20;
                     cal_tree.thresh_stg = 60;
                     cal_tree.thresh_yel = 60;
                     cal_tree.mask_ht    = 1.5;
                 }
                 cal_tree.min_dist = iRadius;
                 cal_tree.min_rad  = iRadius - 8;
                 cal_tree.max_rad  = iRadius + 8;
                 // placeholders for green and red values - don't matter
                 cal_tree.gLowH    = 80;
                 cal_tree.gHighH   = 95;
                 cal_tree.gLowS    = 0;
                 cal_tree.gHighS   = 255;
                 cal_tree.gLowV    = 205;
                 cal_tree.gHighV   = 255;
                 cal_tree.rLowH    = 200;
                 cal_tree.rHighH   = 215;
                 cal_tree.rLowS    = 225;
                 cal_tree.rHighS   = 255;
                 cal_tree.rLowV    = 225;
                 cal_tree.rHighV   = 255;
                 
                 if (b_do_cal_geometry || b_first_cal) {
                     cal_tree.calimg_tl_x = LP_top_left.x;
                     cal_tree.calimg_tl_y = LP_top_left.y;
                     cal_tree.calimg_w = LP_width;
                 }

                 cal_tree.saveToFile(s_svfile);
                 b_run = false;
                 break;
            }
            cvui::imshow("Calibrate", frame.imgDisp);
        }


        int key_val = waitKeyEx(WAITKEYMILLIS);
        // if(key_val != -1) cout << key_val << endl;
    
        switch(key_val) {
            case 27:    // esc - exit program
                cout << "esc key is pressed by user" << endl;
                b_run = false;
                break;
            case 98:    // b - blur
                b_blur = !b_blur;
                break;
            case 102:      // f - filter threshold HSV
                b_filter = !b_filter;
                break;
            case 109:      // m - morphological processing
                b_morph = !b_morph;
                break;
            case 115:      // s - save tree config to file
                {
                string save_path(s_track_dir + "tree.cfg");
                tree.saveToFile(save_path);
                FILE_LOG(logINFO) << "Saved tree profile to " << save_path;
                }
                break;
            default:
                break;
    	} //switch
    }
    destroyWindow("Calibrate");
}


/*void DRTCal::LoadConfig(string filename) {
    INIReader reader(filename);
}*/

void DRTCal::RunSelectCalImage(string s_vid_file) {
    FILE_LOG(logINFO) << "DRTCal->RunSelectCalImage";
    FILE_LOG(logINFO) << "loading video " << s_vid_file;

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

void DRTCal::RunRecordCalVideo ( string filename ) {
    FILE_LOG(logINFO) << "DRTCal->RunRecordCalVideo";
    FILE_LOG(logINFO) << "saving video " << filename;
    // Initialize logging - logERROR, logWARNING, logINFO, logDEBUG, logDEBUG1, logDEBUG2, logDEBUG3, logDEBUG
    FILE* pFile = fopen("log/drt.log", "w");
    Output2FILE::Stream() = pFile;
    FILELog::ReportingLevel() = LOG_LEVEL;

    namedWindow("Original",  WINDOW_AUTOSIZE);

    int i = 0;
    while (!cap.open(i)) {
        i++;
        if ( i == CAMERA_MAX_RETRIES ) return;
        FILE_LOG(logERROR) << "Couldn't open camera";
    }

    int frame_width = cap.get(CAP_PROP_FRAME_WIDTH);
    int frame_height = cap.get(CAP_PROP_FRAME_HEIGHT);
    FILE_LOG(logINFO) << "Camera opened on device " << i << " in " << frame_width << " x " << frame_height << " mode";

    VideoWriter video(filename, VideoWriter::fourcc('M','J','P','G'), 30, Size(frame_width,frame_height)); 

    while(1) {
        Mat frame;
        cap >> frame;

        // If the frame is empty, break immediately
        if (frame.empty())
            break;

        video.write(frame);

        putText( frame, "recording...", Point(5,20), FONT_HERSHEY_DUPLEX, 1, CVYELLOW, 2);
        imshow( "Original", frame );

        // Press  ESC on keyboard to  exit
        char c = (char)waitKey(1);
        if( c == 27 )
            break;
    }

    // When everything done, release the video capture and write object
    cap.release();
    video.release();

    destroyWindow("Original");
}


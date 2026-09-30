#include "xmas_tree.h"

#define illum_thresh 12.0

using namespace cv;
using namespace std;

XmasTree::XmasTree( ) {};

XmasTree::XmasTree( string filepath ) {
    init( filepath );
}

bool XmasTree::init( string filepath ) {

    FILE* pFile = fopen("log/drt.log", "w");
    Output2FILE::Stream() = pFile;
    FILELog::ReportingLevel() = LOG_LEVEL;
    INIReader reader( filepath );
    if (reader.ParseError() < 0) {
        cout << "Couldn't load " << filepath << endl;
        return false;
    }

    string section = "general";
    name         = reader.Get       (  section, "NAME",        "Unknown");
    location     = reader.Get       (  section, "LOCATION",    "Unknown");
    cascadefile  = reader.Get       (  section, "CASCADEFILE", "Unknown");

    section = "lamps";
    prestage_y   = reader.GetReal(  section, "PRESTAGE_Y", 0 );
    stage_y      = reader.GetReal(  section, "STAGE_Y",    0 );
    y1_y         = reader.GetReal(  section, "Y1_Y",       0 );
    y2_y         = reader.GetReal(  section, "Y2_Y",       0 );
    y3_y         = reader.GetReal(  section, "Y3_Y",       0 );
    grn_y        = reader.GetReal(  section, "GRN_Y",      0 );
    red_y        = reader.GetReal(  section, "RED_Y",      0 );

    st1_x        = reader.GetReal(  section, "ST1_X",      0 );
    st2_x        = reader.GetReal(  section, "ST2_X",      0 );
    st3_x        = reader.GetReal(  section, "ST3_X",      0 );
    st4_x        = reader.GetReal(  section, "ST4_X",      0 );
    column1_x    = reader.GetReal(  section, "COLUMN1_X",  0 );
    column2_x    = reader.GetReal(  section, "COLUMN2_X",  0 );

    aspect_ratio   = reader.GetReal(  section, "ASPECT_RATIO", 0 );
    relative_radius = reader.GetReal(  section, "RELATIVE_RADIUS", 0 );
    nominal_radius = reader.GetInteger(  section, "NOMINAL_RADIUS", 0 );

    if (aspect_ratio == 0 || relative_radius == 0 || nominal_radius == 0) FILE_LOG(logWARNING) << "Problem with [lamps] section in tree.cfg";

    section = "HSV_thresholds";
    wLowH  = reader.GetInteger(  section, "WHITE_HUE_LOW",   0 );
    wHighH = reader.GetInteger(  section, "WHITE_HUE_HIGH",  0 );
    wLowS  = reader.GetInteger(  section, "WHITE_SAT_LOW",   0 );
    wHighS = reader.GetInteger(  section, "WHITE_SAT_HIGH",  0 );
    wLowV  = reader.GetInteger(  section, "WHITE_VAL_LOW",   0 );
    wHighV = reader.GetInteger(  section, "WHITE_VAL_HIGH",  0 );

    yLowH  = reader.GetInteger(  section, "YELLOW_HUE_LOW",  0 );
    yHighH = reader.GetInteger(  section, "YELLOW_HUE_HIGH", 0 );
    yLowS  = reader.GetInteger(  section, "YELLOW_SAT_LOW",  0 );
    yHighS = reader.GetInteger(  section, "YELLOW_SAT_HIGH", 0 );
    yLowV  = reader.GetInteger(  section, "YELLOW_VAL_LOW",  0 );
    yHighV = reader.GetInteger(  section, "YELLOW_VAL_HIGH", 0 );

    gLowH  = reader.GetInteger(  section, "GREEN_HUE_LOW",   0 );
    gHighH = reader.GetInteger(  section, "GREEN_HUE_HIGH",  0 );
    gLowS  = reader.GetInteger(  section, "GREEN_SAT_LOW",   0 );
    gHighS = reader.GetInteger(  section, "GREEN_SAT_HIGH",  0 );
    gLowV  = reader.GetInteger(  section, "GREEN_VAL_LOW",   0 );
    gHighV = reader.GetInteger(  section, "GREEN_VAL_HIGH",  0 );

    rLowH  = reader.GetInteger(  section, "RED_HUE_LOW",     0 );
    rHighH = reader.GetInteger(  section, "RED_HUE_HIGH",    0 );
    rLowS  = reader.GetInteger(  section, "RED_SAT_LOW",     0 );
    rHighS = reader.GetInteger(  section, "RED_SAT_HIGH",    0 );
    rLowV  = reader.GetInteger(  section, "RED_VAL_LOW",     0 );
    rHighV = reader.GetInteger(  section, "RED_VAL_HIGH",    0 );

    thresh_stg = reader.GetInteger(  section, "STG_THRESHOLD",    0 );
    thresh_yel = reader.GetInteger(  section, "YEL_THRESHOLD",    0 );
    mask_ht    = reader.GetReal( section, "MASK_HEIGHT"  ,    0.0 );

    section = "circles";
    dp         = reader.GetInteger( section, "DP",         0 ); // dp - "accumulator threshold": Basically, the higher it is the less circles you get. And these circles have a higher probability of being correct. The best value is different for every image.
    min_dist   = reader.GetInteger( section, "MIN_DIST",   0 );
    canny_edge = reader.GetInteger( section, "CANNY_EDGE", 0 );
    center_det = reader.GetInteger( section, "CENTER_DET", 0 );
    min_rad    = reader.GetInteger( section, "MIN_RADIUS", 0 );
    max_rad    = reader.GetInteger( section, "MAX_RADIUS", 0 );
    if (dp == 0 || min_dist == 0 || canny_edge == 0 || center_det == 0 || min_rad == 0 || max_rad == 0) FILE_LOG(logWARNING) << "Problem with [circles] section in tree.cfg";

    // This section used only for displaying overlay on calibration image.
    section = "cal_image";
    calimg_tl_x = reader.GetInteger( section, "TOP_LEFT_X",   0 );
    calimg_tl_y = reader.GetInteger( section, "TOP_LEFT_Y",   0 );
    calimg_w    = reader.GetInteger( section, "WIDTH",   0 );
    
    if (calimg_tl_x == 0 || calimg_tl_y == 0 || calimg_w == 0) FILE_LOG(logWARNING) << "Problem with [cal_image] section in tree.cfg";

    return true;
}

bool XmasTree::saveToFile(string filepath) {
    ofstream treefile;
    treefile.open(filepath);
    if (treefile) {
        treefile << "[general]"
                 << "\nNAME = " << name 
                 << ";\nLOCATION = " << location 
                 << ";\nCASCADEFILE = " << cascadefile << ";\n"; 

        treefile << "[cal_image]"
                 << "\nTOP_LEFT_X = " << calimg_tl_x
                 << ";\nTOP_LEFT_Y = " << calimg_tl_y
                 << ";\nWIDTH = " << calimg_w << ";\n";
         

        treefile << "[lamps]" 
                 << "\nPRESTAGE_Y = " << prestage_y 
                 << ";\nSTAGE_Y = " << stage_y 
                 << ";\nY1_Y = " << y1_y 
                 << ";\nY2_Y = " << y2_y 
                 << ";\nY3_Y = " << y3_y 
                 << ";\nGRN_Y = " << grn_y 
                 << ";\nRED_Y = " << red_y 
                 << ";\nST1_X = " << st1_x 
                 << ";\nST2_X = " << st2_x 
                 << ";\nST3_X = " << st3_x 
                 << ";\nST4_X = " << st4_x 
                 << ";\nCOLUMN1_X = " << column1_x 
                 << ";\nCOLUMN2_X = " << column2_x 
                 << ";\nASPECT_RATIO = " << aspect_ratio 
                 << "\n;RELATIVE_RADIUS: ratio nominal_radius / width (width is measured center-to-center between outermost lights)" 
                 << ";\nRELATIVE_RADIUS = " << relative_radius
                 << "\n;NOMINAL_RADIUS: radius in pixels as set by user during calibration. Needs to capture the center of the lamps for Pattern Detection Mode" 
                 << ";\nNOMINAL_RADIUS = " << nominal_radius << ";\n";

        treefile << "[HSV_thresholds]" 
                 << "\n;Saturation and value May vary with lighting conditions but hue should be relatively static" 
                 << ";\nWHITE_HUE_LOW = " << wLowH 
                 << ";\nWHITE_HUE_HIGH = " << wHighH 
                 << ";\nWHITE_SAT_LOW = " << wLowS  
                 << ";\nWHITE_SAT_HIGH = " << wHighH 
                 << ";\nWHITE_VAL_LOW  = " << wLowV  
                 << ";\nWHITE_VAL_HIGH = " << wHighV 
                 << ";\nYELLOW_HUE_LOW = " << yLowH  
                 << ";\nYELLOW_HUE_HIGH = " << yHighH 
                 << ";\nYELLOW_SAT_LOW  = " << yLowS 
                 << ";\nYELLOW_SAT_HIGH = " << yHighS 
                 << ";\nYELLOW_VAL_LOW  = " << yLowV  
                 << ";\nYELLOW_VAL_HIGH = " << yHighV 
                 << ";\nGREEN_HUE_LOW   = " << gLowH   
                 << ";\nGREEN_HUE_HIGH  = " << gHighH  
                 << ";\nGREEN_SAT_LOW   = " << gLowS   
                 << ";\nGREEN_SAT_HIGH  = " << gHighS  
                 << ";\nGREEN_VAL_LOW   = " << gLowV 
                 << ";\nGREEN_VAL_HIGH  = " << gHighV  
                 << ";\nRED_HUE_LOW     = " << rLowH 
                 << ";\nRED_HUE_HIGH    = " << rHighH    
                 << ";\nRED_SAT_LOW     = " << rLowS     
                 << ";\nRED_SAT_HIGH    = " << rHighS    
                 << ";\nRED_VAL_LOW     = " << rLowV     
                 << ";\nRED_VAL_HIGH    = " << rHighV
                 << ";\nSTG_THRESHOLD   = " << thresh_stg
                 << ";\nYEL_THRESHOLD   = " << thresh_yel
                 << ";\nMASK_HEIGHT     = " << mask_ht << ";\n";

        treefile << "[circles]" 
                 << "\n;DP: Resolution of accumulator array. Set too small and only perfect circles are found, set too high and noise collaborates to vote for non-circles."
                 << "\nDP = " << dp 
                 << ";\nMIN_DIST = " << min_dist 
                 << ";\n;CANNY_EDGE: Too low only detects perfect circles, too high will get false positives."
                 << ";\nCANNY_EDGE = " << canny_edge 
                 << ";\n;CENTER_DET: This is actually the accumulator threshold."
                 << ";\nCENTER_DET = " << center_det 
                 << ";\nMIN_RADIUS = " << min_rad 
                 << ";\nMAX_RADIUS = " << max_rad << ";\n";

        treefile.close();
        return true;
    } else return false;
}


vector<Point> XmasTree::getLampPoints() {
    vector<Point> outvec;
    outvec.push_back(Point(st1_x, prestage_y));
    outvec.push_back(Point(st2_x, prestage_y));
    outvec.push_back(Point(st3_x, prestage_y));
    outvec.push_back(Point(st4_x, prestage_y));
    outvec.push_back(Point(st1_x, stage_y   ));
    outvec.push_back(Point(st2_x, stage_y   ));
    outvec.push_back(Point(st3_x, stage_y   ));
    outvec.push_back(Point(st4_x, stage_y   ));
    outvec.push_back(Point(column1_x, y1_y  ));
    outvec.push_back(Point(column2_x, y1_y  ));
    outvec.push_back(Point(column1_x, y2_y  ));
    outvec.push_back(Point(column2_x, y2_y  ));
    outvec.push_back(Point(column1_x, y3_y  ));
    outvec.push_back(Point(column2_x, y3_y  ));
    outvec.push_back(Point(column1_x, grn_y  ));
    outvec.push_back(Point(column2_x, grn_y  ));
    outvec.push_back(Point(column1_x, red_y  ));
    outvec.push_back(Point(column2_x, red_y  ));
    return outvec;
}

vector<Point> XmasTree::getLampPoints(Point top_left, float width) {
    vector<Point> outvec;
    float height = width * aspect_ratio;

    int _ps_y = top_left.y;
    int _st_y = top_left.y + cvRound(stage_y * height);
    int _y1_y = top_left.y + cvRound(y1_y * height);
    int _y2_y = top_left.y + cvRound(y2_y * height);
    int _y3_y = top_left.y + cvRound(y3_y * height);
    int _grn_y = top_left.y + cvRound(grn_y * height);
    int _red_y = top_left.y + cvRound(red_y * height);


    int _st1_x = top_left.x;
    int _st2_x = top_left.x + cvRound(st2_x * width);
    int _st3_x = top_left.x + cvRound(st3_x * width);
    int _st4_x = top_left.x + cvRound(width);

    int _col1_x = top_left.x + cvRound(column1_x * width);
    int _col2_x = top_left.x + cvRound(column2_x * width);

    outvec.push_back(top_left);
    outvec.push_back(Point( _st2_x, _ps_y  ));
    outvec.push_back(Point( _st3_x, _ps_y  ));
    outvec.push_back(Point( _st4_x, _ps_y  ));
    outvec.push_back(Point( _st1_x, _st_y  ));
    outvec.push_back(Point( _st2_x, _st_y  ));
    outvec.push_back(Point( _st3_x, _st_y  ));
    outvec.push_back(Point( _st4_x, _st_y  ));
    outvec.push_back(Point( _col1_x, _y1_y  ));
    outvec.push_back(Point( _col2_x, _y1_y  ));
    outvec.push_back(Point( _col1_x, _y2_y  ));
    outvec.push_back(Point( _col2_x, _y2_y  ));
    outvec.push_back(Point( _col1_x, _y3_y  ));
    outvec.push_back(Point( _col2_x, _y3_y  ));
    outvec.push_back(Point( _col1_x, _grn_y ));
    outvec.push_back(Point( _col2_x, _grn_y ));
    outvec.push_back(Point( _col1_x, _red_y ));
    outvec.push_back(Point( _col2_x, _red_y ));
    return outvec;
}


// function: get_zone
// given a rectangle, return the portion of that rectangle that encompasses the specified zone.
Rect XmasTree::getZone(ZONE zone, Rect roi) {
    int _x1, _y1, _x2, _y2;
    Point _p1, _p2;
    Rect _rect;

    Point tl = roi.tl(); // top left corner
    int _xlt = roi.x;
    int _xrt = _xlt + roi.width; // roi right boundary
    switch(zone) {
        case PRESTAGE:
            _p1 = tl;
            _x2 = _xrt;
            _y2 = roi.y + ((roi.height * prestage_bot)/100);
            _p2 = Point(_x2, _y2);
            _rect = Rect(_p1, _p2);
        break;
        case STAGE:
            _x1 = _xlt;
            _y1 = roi.y + ((roi.height * stage_top)/100);
            _p1 = Point(_x1, _y1);
            _x2 = _xrt;
            _y2 = roi.y + ((roi.height * stage_bot)/100);
            _p2 = Point(_x2, _y2);
            _rect = Rect(_p1, _p2);
        break;
        case ONE:
            _x1 = _xlt;
            _y1 = roi.y + ((roi.height * yel1_top)/100);
            _p1 = Point(_x1, _y1);
            _x2 = _xrt;
            _y2 = roi.y + ((roi.height * yel1_bot)/100);
            _p2 = Point(_x2, _y2);
            _rect = Rect(_p1, _p2);
        break;
        case TWO:
            _x1 = _xlt;
            _y1 = roi.y + ((roi.height * yel2_top)/100);
            _p1 = Point(_x1, _y1);
            _x2 = _xrt;
            _y2 = roi.y + ((roi.height * yel2_bot)/100);
            _p2 = Point(_x2, _y2);
            _rect = Rect(_p1, _p2);
        break;
        case THREE:
            _x1 = _xlt;
            _y1 = roi.y + ((roi.height * yel3_top)/100);
            _p1 = Point(_x1, _y1);
            _x2 = _xrt;
            _y2 = roi.y + ((roi.height * yel3_bot)/100);
            _p2 = Point(_x2, _y2);
            _rect = Rect(_p1, _p2);
        break;
        case GO:
            _x1 = _xlt;
            _y1 = roi.y + ((roi.height * grn_top)/100);
            _p1 = Point(_x1, _y1);
            _x2 = _xrt;
            _y2 = roi.y + ((roi.height * grn_bot)/100);
            _p2 = Point(_x2, _y2);
            _rect = Rect(_p1, _p2);
        break;
        case FOUL:
            _x1 = _xlt;
            _y1 = roi.y + ((roi.height * red_top)/100);
            _p1 = Point(_x1, _y1);
            _x2 = _xrt;
            _y2 = roi.y + ((roi.height * red_bot)/100);
            _p2 = Point(_x2, _y2);
            _rect = Rect(_p1, _p2);
        break;
        default:
            _rect = Rect(roi);
        break;
    }
    return _rect;
}

Mat XmasTree::getZone(ZONE zone, Mat matRoi) {
    int _rows = matRoi.rows;  // height of filtered Mat
    int startrow = 0;
    int endrow = 0;
    int radius = ((st1_x - st2_x)/2) * _rows;

    switch(zone) {
        case PRESTAGE:
            //startrow = (_rows * prestage_top)/100;
            //endrow = (_rows * prestage_bot)/100;
            startrow = (_rows * prestage_y) - radius;
            endrow = (_rows * prestage_y) + radius;
            break;
        case STAGE:
            //startrow = (_rows * stage_top)/100;
            //endrow = (_rows * stage_bot)/100;
            startrow = (_rows * stage_y) - radius;
            endrow = (_rows * stage_y) + radius;
            break;
        case ONE:
            //startrow = (_rows * yel1_top)/100;
            //endrow = (_rows * yel1_bot)/100;
            startrow = (_rows * y1_y) - radius;
            endrow = (_rows * y1_y) + radius;
            break;
        case TWO:
            //startrow = (_rows * yel2_top)/100;
            //endrow = (_rows * yel2_bot)/100;
            startrow = (_rows * y2_y) - radius;
            endrow = (_rows * y2_y) + radius;
            break;
        case THREE:
            //startrow = (_rows * yel3_top)/100;
            //endrow = (_rows * yel3_bot)/100;
            startrow = (_rows * y3_y) - radius;
            endrow = (_rows * y3_y) + radius;
            break;
        case GO:
            //startrow = (_rows * grn_top)/100;
            //endrow = (_rows * grn_bot)/100;
            startrow = (_rows * grn_y) - radius;
            endrow = (_rows * grn_y) + radius;
            break;
        case FOUL:
            //startrow = (_rows * red_top)/100;
            //endrow = (_rows * red_bot)/100;
            startrow = (_rows * red_y) - radius;
            endrow = (_rows * red_y) + radius;
            break;
    }
    Mat matZone = matRoi.rowRange(startrow,endrow);
    Scalar _avg = mean(matZone);
    if (_avg[0] > illum_thresh) setIllum(zone, true);
    else setIllum(zone, false);
    return matRoi.rowRange(startrow,endrow);
}

void XmasTree::setIllum(ZONE zone, bool val) {
    switch(zone) {
        case PRESTAGE:
            ps_illum = val;
            break;
        case STAGE:
            st_illum = val;
            break;
        case ONE:
            y1_illum = val;
            break;
        case TWO:
            y2_illum = val;
            break;
        case THREE:
            y3_illum = val;
            break;
        case GO:
            gr_illum = val;
            break;
        case FOUL:
            rd_illum = val;
            break;
    }
}


bool XmasTree::getIllum(ZONE zone) {
    switch(zone) {
        case PRESTAGE:
            return ps_illum;
            break;
        case STAGE:
            return st_illum;
            break;
        case ONE:
            return y1_illum;
            break;
        case TWO:
            return y2_illum;
            break;
        case THREE:
            return y3_illum;
            break;
        case GO:
            return gr_illum;
            break;
        case FOUL:
            return rd_illum;
            break;
    }
    return false;
}

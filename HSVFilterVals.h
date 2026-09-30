#ifndef __HSVFILTERVALS_H__
#define __HSVFILTERVALS_H__

struct HSVFilterVals {

    int iLowH  = 0;
    int iHighH = 40;
    int iLowS  = 0;
    int iHighS = 40;
    int iLowV  = 250;
    int iHighV = 254;

    HSVFilterVals(int _loH, int _hiH, int _loS, int _hiS, int _loV, int _hiV) :iLowH(_loH), iHighH(_hiH), iLowS(_loS), iHighS(_hiS), iLowV(_loV), iHighV(_hiV) { };

};

#endif

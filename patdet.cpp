#include "patdet.h"

using namespace std;
using namespace cv;

/*** pop Minimum X ***/
Vec3f PatDet::popMinX(vector<Vec3f>& vec) {
    Vec3f min_x(0,0,0);
    // This should throw an exception
    if (vec.empty()) {
        return min_x;
    }
    min_x = *(vec.begin());
    if (vec.size() == 1) {
        vec.pop_back();
        return min_x;
    }
    vector<Vec3f>::iterator it_del = vec.begin();
    for ( vector<Vec3f>::iterator it = (vec.begin() + 1); it != vec.end(); ++it ) {
        Vec3f v3ftmp = *(it);
        if ( v3ftmp[0] <= min_x[0] ) {
            min_x = v3ftmp;
            it_del = it;
        }
    }
    vec.erase(it_del);
    return min_x;
}

/*** pop Minimum Y ***/
Vec3f PatDet::popMinY(vector<Vec3f>& vec) {
    Vec3f min_y(0,0,0);
    if (vec.empty()) {
        return min_y;
    }
    min_y = *(vec.begin());
    if (vec.size() == 1) {
        vec.pop_back();
        return min_y;
    }
    vector<Vec3f>::iterator it_del = vec.begin();
    for ( vector<Vec3f>::iterator it = (vec.begin() + 1); it != vec.end(); ++it ) {
        Vec3f v3ftmp = *(it);
        if ( v3ftmp[1] <= min_y[1] ) {
            min_y = v3ftmp;
            it_del = it;
        }
    }
    vec.erase(it_del);
    return min_y;
}

/*** pop Maximum Y ***/
Vec3f PatDet::popMaxY(vector<Vec3f>& vec) {
    Vec3f min_y(0,0,0);
    if (vec.empty()) {
        return min_y;
    }
    min_y = *(vec.begin());
    if (vec.size() == 1) {
        vec.pop_back();
        return min_y;
    }
    vector<Vec3f>::iterator it_del = vec.begin();
    for ( vector<Vec3f>::iterator it = (vec.begin() + 1); it != vec.end(); ++it ) {
        Vec3f v3ftmp = *(it);
        if ( v3ftmp[1] >= min_y[1] ) {
            min_y = v3ftmp;
            it_del = it;
        }
    }
    vec.erase(it_del);
    return min_y;
}

/*** Sort By X coord ***/
vector<Vec3f> PatDet::sortByX(vector<Vec3f> invec) {
    if( invec.size() > 1 ) {
        /*int i = 0; for (vector<Vec3f>::iterator it = invec.begin(); it != invec.end(); ++it) { cout << "Idx: " << i << "   " << (*it) << endl; i++; } cout << endl; */
        vector<Vec3f> outvec;
        while (!invec.empty()) {
            outvec.push_back(popMinX(invec));
        }
        return outvec;
        /* i = 0; for (vector<Vec3f>::iterator it = outvec.begin(); it != outvec.end(); ++it) { cout << "Idx: " << i << "   " << (*it) << endl; i++; } */
    } else {
        return invec;
    }
}

/*** Sort By Y coord ***/
vector<Vec3f> PatDet::sortByY(vector<Vec3f> invec) {
    if( invec.size() > 1 ) {
        vector<Vec3f> outvec;
        while (!invec.empty()) {
            outvec.push_back(popMinY(invec));
        }
        return outvec;
    } else {
        return invec;
    }
}

vector<Vec3f> PatDet::WithinRadius_X( Vec3f cir1, vector<Vec3f> invec, float coeff ) {
    vector<Vec3f> outvec;
    int c1_x = cir1[0];
    int c1_y = cir1[1];
    for ( vector<Vec3f>::iterator it = (invec.begin()); it != invec.end(); ++it ) {
        Vec3f cir2 = *(it);
        int c2_x = cir2[0];
        int c2_y = cir2[1];
        float range = cir1[2] * coeff; // within one radius. TODO multiply by scaling factor - maybe 0.5
        if (c2_x > (c1_x - range) && c2_x < (c1_x + range)) {
            //if ( !(c2_y > (c1_y - range) && c2_y < (c1_y + range)) ) {
            if ( !(c2_y == c1_y || c2_y == c1_y) ) { // don't add self
                outvec.push_back(cir2);
            }
        }
    }
    return outvec;
}


/*** return vector of Vec3f's within multiple of radius (coeff) and optionally remove from input vector ***/
vector<Vec3f> PatDet::WithinRadius_Y( Vec3f cir1, vector<Vec3f>& invec, float coeff, bool b_rmv ) {
    vector<Vec3f> outvec;
    vector<Vec3f> outsidevec;
    // TODO not efficient to make vector every time
    int c1_y = cir1[1];
    for ( vector<Vec3f>::iterator it = invec.begin(); it != invec.end(); ++it ) {
        Vec3f cir2 = *it;
        int c2_y = cir2[1];
        float range = cir1[2] * coeff; // within one radius
        if (c2_y >= (c1_y - range) && c2_y <= (c1_y + range)) {
            outvec.push_back(cir2);
            //if ( !(c2_x == c1_x || c2_x == c1_x) ) outvec.push_back(cir2); // Don't add self
        } else outsidevec.push_back(cir2);
    }
    // remove elements from the vector
    if (b_rmv) invec = outsidevec;

    return outvec;
}

int PatDet::LampsCenterline( bool b_draw ) {
    for( size_t i = 0; i < rawcircles.size(); i++ ) {
        cir_x.push_back(rawcircles[i][0]);
        if (cir_x.size() > 30) cir_x.erase(cir_x.begin());
    }
    int sum = 0;
    for ( vector<int>::iterator it = cir_x.begin(); it != cir_x.end(); ++it ) {
        int x = *(it);
        sum += x;
    }
    float avgf = (float)sum / (float)cir_x.size();
    int avg = (int)round(avgf);
    return avg;
}

bool PatDet::CompareCircles( Vec3f cir1, Vec3f cir2, float rad_coeff ) {
    float radius = ((float)cir1[2] + (float)cir2[2]) / 2.0;
    //cout << "Allowable radius: " << radius << endl;
    // cout << "radius: " << radius << "   rad_coeff: " << rad_coeff << endl;
    //cout << "radius plus minus: " << radius * rad_coeff << endl;
    int upr_bnd_x = cir2[0] + (radius * rad_coeff);
    int lwr_bnd_x = cir2[0] - (radius * rad_coeff);
    int upr_bnd_y = cir2[0] + (radius * rad_coeff);
    int lwr_bnd_y = cir2[0] - (radius * rad_coeff);
    if ( (cir1[0] < upr_bnd_x && cir1[0] > lwr_bnd_x) && (cir1[1] < upr_bnd_y && cir1[1] > lwr_bnd_y) ) {
        //cout << "BOUNDS    X: " << lwr_bnd_x << " - " << upr_bnd_x  << "    Y: " << lwr_bnd_y << " - " << upr_bnd_y << "   Actual:  " << cir1[0] << ", " << cir1[1] << endl;
        //cout << "cir1 x: " << cir1[0] << ", cir2 x: " << cir2[0] << "    cir1 y: " << cir1[1] << ", cir2 y: " << cir2[1] << endl;
        return true;
    }
    else return false;
}

bool PatDet::SearchCircles( Vec3f cir1, vector<Vec3f> invec, float rad_coeff ) {
    bool match = false;
    for ( vector<Vec3f>::iterator it = invec.begin(); it != invec.end(); ++it ) {
        Vec3f cir2 = *it;
        if (CompareCircles( cir1, cir2, rad_coeff )) {
            match = true;
            break;
        }
    }
    return match;
}

// rad_coeff - multiply by radius to get size of bounding box to consider circle the same from last frame.
void PatDet::RemoveDupeCircles( float rad_coeff ) {
    // got new circles
    //cout << "Circle Frame Start\n";
    if (!rawcircles.empty()) {
        // and have old circles
        if (!circles.empty()) {
            vector<Vec3f> allcircles;
            for ( vector<Vec3f>::iterator it_r = rawcircles.begin(); it_r != rawcircles.end(); ++it_r ) {
                Vec3f cir1 = *it_r;
                allcircles.push_back(cir1);
                if (!SearchCircles( cir1, circles, rad_coeff )) {
                    allcircles.push_back(cir1);
                }/* else {
                    cout << "removed a dupe\n";
                }*/
            }
            circles.clear();
            circles = allcircles;
        } else {
            circles = rawcircles;
        }
    }
}

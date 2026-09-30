#include <vector>
#include <iostream>
#include <string>
#include "/usr/local/include/opencv4/opencv2/imgproc/imgproc.hpp"

using namespace cv;
using namespace std;

Vec3f get_min_x(vector<Vec3f>& vec) {
    Vec3f min_x(0,0,0);
    // This should throw an exception
    if (vec.empty()) {
        cout << "Empty vector\n";
        return min_x;
    }
    min_x = *(vec.begin());
    if (vec.size() == 1) {
        //cout << "Vector size = 1\n";
        vec.pop_back();
        return min_x;
    }
    //cout << "Vector size " << vec.size() << endl;
    vector<Vec3f>::iterator it_del = vec.begin();
    for ( vector<Vec3f>::iterator it = (vec.begin() + 1); it != vec.end(); ++it ) {
        Vec3f v3ftmp = *(it);
        //cout << v3ftmp[0] << "   COMPARING TO   " << min_x[0] << endl;
        if ( v3ftmp[0] < min_x[0] ) {
            //cout << v3ftmp[0] << "  LOWER THAN  " << min_x[0] << endl;
            min_x = v3ftmp;
            it_del = it;
        }
    }
    //cout << "Removing " << *it_del << endl;
    vec.erase(it_del);
    //cout << "Found minimum x value:  " << min_x[0] << endl;
    return min_x;
}

int main() {
    // define some points
    Vec3f el1(255,30,35);
    Vec3f el2(100,15,20);
    Vec3f el3(75,15,20);
    Vec3f el4(460,45,50);
    Vec3f el5(20,45,50);
    Vec3f el6(423,45,50);
    // populate input vector
    vector<Vec3f> invec{ el1, el2, el3, el4, el5, el6 };
    cout << "Input vector size: " << invec.size() << endl;

    // print the input vector
    int i = 0;
    for (vector<Vec3f>::iterator it = invec.begin(); it != invec.end(); ++it) {
        cout << "Idx: " << i << "   " << (*it) << endl;
        i++;
    }
    cout << endl;

    // declare the output vector
    vector<Vec3f> outvec;
    size_t vecsztmp = invec.size();
    //while (vecsztmp != 0) {
    while (!invec.empty()) {
        outvec.push_back(get_min_x(invec));
        cout << "push_back " << outvec.back() << endl;
        vecsztmp--;
        //cout << "Vec_size_counter " << vecsztmp << endl;
        cout << "invec.size() :   " << invec.size() << endl;
    }



    i = 0;
    for (vector<Vec3f>::iterator it = outvec.begin(); it != outvec.end(); ++it) {
        cout << "Idx: " << i << "   " << (*it) << endl;
        i++;
    }
    exit(EXIT_SUCCESS);
}


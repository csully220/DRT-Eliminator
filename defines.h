#ifndef __DRT_DEFINES_H__
#define __DRT_DEFINES_H__

// If compiling in a VM
//#define VM

#define LOG_LEVEL logDEBUG4

#define WAITKEYMILLIS 10

// OpenCV Colors
#define CVWHITE  Scalar(255,255,255)
#define CVBLUE   Scalar(255,0,0)
#define CVGREEN  Scalar(0,255,0)
#define CVRED    Scalar(0,0,255)
#define CVYELLOW Scalar(0,255,255)
#define CVCYAN   Scalar(255,255,0)
#define CVPURPLE Scalar(128,0,128)

#define CVUIWHITE  0xFFFFFF
#define CVUIBLUE   0x0000FF
#define CVUIGREEN  0x00FF00
#define CVUIRED    0xFF0909
#define CVUIYELLOW 0xFFFF00
#define CVUICYAN   0x00FFFF
#define CVUIPURPLE 0xFF00FF
#define CVUIGRAY   0x090909

// Raspberry Pi I/O pins
#define BRKREL 25

#define BGIMAGE "./data/backgrounds/track_from_air.jpg"
#define COL1 50
#define COL2 250  // column 2 for profile menu

#define ROW_PAD 30
#define ROW1 110
#define ROW2 ROW1 + ROW_PAD  // 50
#define ROW3 ROW2 + ROW_PAD  // 80
#define ROW4 ROW3 + ROW_PAD  // 110
#define ROW5 ROW4 + ROW_PAD  // 140
#define ROW6 ROW5 + ROW_PAD  // 170
#define ROW7 ROW6 + ROW_PAD  // 200
#define ROW8 ROW7 + ROW_PAD  // 230
#define ROW9 ROW8 + ROW_PAD  // 260
#define ROW10 ROW9 + ROW_PAD  // 290
#define ROW11 ROW10 + ROW_PAD  // 320
#define ROW12 ROW11 + ROW_PAD  // 350
#define ROW13 ROW12 + ROW_PAD  // 380
#define ROW14 ROW13 + ROW_PAD  // 410
#define ROW15 ROW14 + ROW_PAD  // 410
#define ROW16 ROW15 + ROW_PAD  // 410
#define ROW17 ROW16 + ROW_PAD  // 410

#define BTN_WID 180

#endif

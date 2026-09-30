CC = g++
INC = -I/usr/local/include/opencv4 -I../lib
LINKS = -L/opt/vc/lib
#LINKS += -L/usr/lib/arm-linux-gnueabihf/
LIBS = -lmmal_core -lmmal -lmmal_util -lvcos -lbcm_host
LIBS += -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_tracking -lopencv_objdetect
CFLAGS = -Wall -g -std=c++11
#OBJS = obj/ini.o obj/inireader.o obj/tree.o obj/DRT.o obj/main.o

min_x: min_x.cpp
	@$(CC) $(CFLAGS) $(INC) min_x.cpp -o run_minx $(LINKS) $(LIBS)


CC = g++
INC = -I/usr/local/include/opencv4 -Ilib 
LINKS = -L/opt/vc/lib 
LIBS = -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_tracking -lopencv_objdetect -lstdc++fs -lpthread -lwiringPi
VMLIBS = -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_tracking -lopencv_objdetect -lstdc++fs -lpthread
CFLAGS = -Wall -Wno-unknown-pragmas -g -std=gnu++17
OBJS = obj/ini.o obj/inireader.o obj/visframe.o obj/frameutils.o obj/patdet.o obj/xmas_tree.o obj/DRTVid.o obj/DRTUtils.o obj/DRTCal.o obj/DRTLive.o obj/main.o
#OBJS = $(wildcard objs/*.o)

all: drt_eliminator

vm: drt_vm

drt_vm: $(OBJS)
	@echo Linking for target VM...
	@$(CC) $(CFLAGS) $(OBJS) -o drt_eliminator $(LINKS) $(VMLIBS)
	@echo Linking complete

drt_eliminator: $(OBJS)
	@echo Linking for target Raspi...
	@$(CC) $(CFLAGS) $(OBJS) -o drt_eliminator $(LINKS) $(LIBS)
	@echo Linking complete

obj/main.o: main.cpp defines.h
	@$(CC) $(CFLAGS) $(INC) -c main.cpp -o obj/main.o
	@echo Made main

obj/DRTLive.o: DRTLive.cpp DRTLive.h DRTRealTime.h
	@$(CC) $(CFLAGS) $(INC) -c DRTLive.cpp -o obj/DRTLive.o
	@echo Made DRTLive

obj/DRTCal.o: DRTCal.cpp DRTCal.h
	@$(CC) $(CFLAGS) $(INC) -c DRTCal.cpp -o obj/DRTCal.o
	@echo Made DRTCal

obj/DRTUtils.o: DRTUtils.cpp DRTUtils.h
	@$(CC) $(CFLAGS) $(INC) -c DRTUtils.cpp -o obj/DRTUtils.o
	@echo Made DRTUtils

obj/DRTVid.o: DRTVid.cpp DRTVid.h
	@$(CC) $(CFLAGS) $(INC) -c DRTVid.cpp -o obj/DRTVid.o
	@echo Made DRTVid

obj/frameutils.o: frameutils.cpp frameutils.h
	@$(CC) $(CFLAGS) $(INC) -c frameutils.cpp -o obj/frameutils.o
	@echo Made frameutils
   
obj/visframe.o: visframe.cpp visframe.h
	@$(CC) $(CFLAGS) $(INC) -c visframe.cpp -o obj/visframe.o
	@echo Made visframe
   
obj/patdet.o: patdet.cpp patdet.h
	@$(CC) $(CFLAGS) $(INC) -c patdet.cpp -o obj/patdet.o
	@echo Made patdet
   
obj/xmas_tree.o: xmas_tree.cpp xmas_tree.h
	@$(CC) $(CFLAGS) $(INC) -c xmas_tree.cpp -o obj/xmas_tree.o
	@echo Made xmas_tree

obj/inireader.o: lib/INIReader.cpp lib/INIReader.h
	@$(CC) $(CFLAGS) $(INC) -c lib/INIReader.cpp -o obj/inireader.o
	@echo Made inireader

obj/ini.o: lib/ini.c lib/ini.h
	@$(CC) $(CFLAGS) $(INC) -c lib/ini.c -o obj/ini.o
	@echo Made ini

.PHONY: clean
clean:
	@echo Cleaning...
	$(RM) *.o
	$(RM) obj/*.o
#	rm xmas_detect
#	@rm camtest


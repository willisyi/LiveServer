LIVE_DIR = ../

#PUBLIC_INCLUDE_DIR = ../interface/include
#APP_LIB_DIR = ../interface/lib
EXEC = wis-streamer



all: $(EXEC)

CC = arm-none-linux-gnueabi-gcc
CPLUSPLUS = arm-none-linux-gnueabi-g++

#CC=gcc
#CPLUSPLUS=g++

INCLUDES = -I . \
	-I$(LIVE_DIR)/BasicUsageEnvironment/include \
	-I$(LIVE_DIR)/UsageEnvironment/include \
	-I$(LIVE_DIR)/groupsock/include \
	-I$(LIVE_DIR)/liveMedia/include


CFLAGS = $(INCLUDES) -D_LINUX -g -Wall -rdynamic


LIBS = 	-L$(LIVE_DIR)/liveMedia -lliveMedia \
	-L$(LIVE_DIR)/BasicUsageEnvironment -lBasicUsageEnvironment \
	-L$(LIVE_DIR)/UsageEnvironment -lUsageEnvironment \
	-L$(LIVE_DIR)/groupsock -lgroupsock 


OBJS = wis-streamer.o  Err.o WISInput.o \
       share_mem.o \
       semaphore.o \
       WISServerMediaSubsession.o \
       WISH264VideoServerMediaSubsession.o \
       WISPCMAudioServerMediaSubsession.o \
       WISMPEG1or2VideoServerMediaSubsession.o

wis-streamer: $(OBJS) 
	$(CPLUSPLUS) $(CFLAGS) -o $(EXEC) $(OBJS) $(LIBS)



wis-streamer.cpp:			Err.hh 

Err.cpp:				Err.hh

#WISInput.cpp:				WISInput.hh  Err.hh
WISInput.cpp:				WISInput.hh share_mem.h semaphore.h

share_mem.c:                            share_mem.h

semaphore.c:                            semaphore.h

WISServerMediaSubsession.cpp:		WISServerMediaSubsession.hh

WISServerMediaSubsession.hh:            WISInput.hh

WISH264VideoServerMediaSubsession.hh:	WISServerMediaSubsession.hh

WISH264VideoServerMediaSubsession.cpp:	WISH264VideoServerMediaSubsession.hh

WISMPEG1or2VideoServerMediaSubsession.hh: WISServerMediaSubsession.hh

WISMPEG1or2VideoServerMediaSubsession.cpp: WISMPEG1or2VideoServerMediaSubsession.hh

WISPCMAudioServerMediaSubsession.cpp:	WISPCMAudioServerMediaSubsession.hh


.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

.cpp.o:
	$(CPLUSPLUS) -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o *~
	rm -f $(EXEC)	

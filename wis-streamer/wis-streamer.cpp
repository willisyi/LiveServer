/*
 * Copyright (C) 2005-2006 WIS Technologies International Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and the associated README documentation file (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
// An application that streams audio/video captured by a WIS GO7007,
// using a built-in RTSP server.
// main program

extern "C"
{
#include "share_mem.h"
#include "semaphore.h"
}

#include <signal.h>
#include <BasicUsageEnvironment.hh>
#include <getopt.h>
#include <liveMedia.hh>
#include "Err.hh"
#include "WISH264VideoServerMediaSubsession.hh"
#include "WISPCMAudioServerMediaSubsession.hh"
#include <sys/time.h>
#include <sys/resource.h>
#include <GroupsockHelper.hh>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <execinfo.h>


#define SIZE 1000
void *buffer[SIZE];
extern int gSEMV;
extern int gSEMVE;
//¼ì²é¶Î´íÎóµÄº¯Êý
void fault_trap(int n,struct siginfo *siginfo,void *myact)
{
        int i, num;
        char **calls;
        printf("Fault address:%X\n",siginfo->si_addr);   
        num = backtrace(buffer, SIZE);
        calls = backtrace_symbols(buffer, num);
        for (i = 0; i < num; i++)
                printf("%s\n", calls[i]);
        exit(1);
}

void setuptrap()
{
    struct sigaction act;
        sigemptyset(&act.sa_mask);   
        act.sa_flags=SA_SIGINFO;    
        act.sa_sigaction=fault_trap;
        sigaction(SIGSEGV,&act,NULL);
}

enum  StreamingMode
{
	STREAMING_UNICAST,
	STREAMING_UNICAST_THROUGH_DARWIN,
	STREAMING_MULTICAST_ASM,
	STREAMING_MULTICAST_SSM
};

enum{
	AUDIO_G711 = 0,
	AUDIO_AAC
};


portNumBits rtspServerPortNum = 8557;
char videoType = -1;
char const* H264StreamName = "H.264";
char const* streamDescription = "RTSP/RTP stream from IPNC";
int H264VideoBitrate = 8000000;
int audioOutputBitrate = 64000;
unsigned audioSamplingFrequency = 8000;
unsigned audioNumChannels = 1;
int audio_enable = 1;
unsigned audioType = AUDIO_G711;
char watchVariable = 0;


void sighandler(int signumber)
{
	printf("program is exiting...\n");
	exit(0);
}


int main(int argc, char** argv) {
 setuptrap();
 if(signal(SIGINT,&sighandler)==SIG_ERR)
 { 
         printf("register SIGINT handler error...\n");
	 exit(0);
 }
 int mid;
 int cnt = 0;
 gSEMV = 1221;
 gSEMVE=1220;
   StreamingMode streamingMode = STREAMING_UNICAST;
  portNumBits videoRTPPortNum = 0;
  portNumBits audioRTPPortNum = 0;
   int IsSilence = 0;
   int port=8557,shm=SHM_KEY;
   int bitrate = 8000000;
 for( cnt = 1; cnt < argc ;cnt++ )
  {
	if( strcmp( argv[cnt],"-m" )== 0  )
	{
		streamingMode = STREAMING_MULTICAST_SSM;
	}

	if( strcmp( argv[cnt],"-s" )== 0  )
	{
		IsSilence = 1;
	}

	if( strcmp( argv[cnt],"-a" )== 0  )
	{
		audioType = AUDIO_AAC;
	}
	if(strcmp(argv[cnt],"-p")==0)
	{
	 	port = atoi(argv[cnt+1]);
		cnt++;
		continue;
	}
	if(strcmp(argv[cnt],"-shm")==0)
	{
	 	shm = atoi(argv[cnt+1]);
		cnt++;
		continue;
	}
	if(strcmp(argv[cnt],"-r")==0)//bitrate
	{
	 	bitrate = atoi(argv[cnt+1]);
		cnt++;
		continue;
	}
	if(strcmp(argv[cnt],"-sem")==0)//
	{
	 	gSEMV = atoi(argv[cnt+1]);
		gSEMVE = atoi(argv[cnt+2]);
		cnt +=2;
		continue;
	}	
	
  }

 
 mid = ShareMemInit(shm);//33


  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);
  *env <<"port="<<port<<",shm="<<shm<<",gSEMV="<<gSEMV<<",gSEMVE="<<gSEMVE<<"\n";
  int video_type;
  WISInput* H264InputDevice = NULL;
   static pid_t child[1] = {
	-1
  };
  //  child[0] = fork();

  //  if( child[0] == 0 )
  //   {
	 /* parent, success */
	 video_type = VIDEO_TYPE_H264;
	 rtspServerPortNum = port;//8557
	 H264VideoBitrate = bitrate;//8000000
	 //videoRTPPortNum = 6012;
	 //audioRTPPortNum = 6014;
	 videoRTPPortNum = port-2000;
	 audioRTPPortNum = port-1998;
	 //   }

  videoType = video_type;
  
  *env << "Initializing...\n";

  // Initialize the WIS input device:

  if( video_type == VIDEO_TYPE_H264)
  {
       H264InputDevice= WISInput::createNew(*env,video_type);
  	if (H264InputDevice == NULL) {
    	err(*env) << "Failed to create H264 input device\n";
    	exit(1);
  	}
  }
 else
 {
	 err(*env) << "unsupport videotype.....\n";
 }

 
  // Create the RTSP server:
    RTSPServer* rtspServer = NULL;
    // Normal case: Streaming from a built-in RTSP server:
    rtspServer = RTSPServer::createNew(*env, rtspServerPortNum, NULL);
    if (rtspServer == NULL) {
      *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
        exit(1);
    }

    *env << "...done initializing\n";
     

    // Create a record describing the media to be streamed:

    if( video_type == VIDEO_TYPE_H264)
    {
     ServerMediaSession* sms
       = ServerMediaSession::createNew(*env,  H264StreamName, H264StreamName, streamDescription,
				       streamingMode == STREAMING_MULTICAST_SSM);   
   
     sms->addSubsession(WISH264VideoServerMediaSubsession
				  ::createNew(sms->envir(), *H264InputDevice, H264VideoBitrate));
     if(IsSilence==0)
     {
       sms->addSubsession(WISPCMAudioServerMediaSubsession::createNew(sms->envir(), *H264InputDevice));
     }
    
   
     rtspServer->addServerMediaSession(sms);
	
    char *url = rtspServer->rtspURL(sms);
    *env << "Play this stream using the URL:\n\t" << url << "\n";
    delete[] url;

   }

  // Begin the LIVE555 event loop:
  env->taskScheduler().doEventLoop(); // does not return


  Medium::close(rtspServer); // will also reclaim "sms" and its "ServerMediaSubsession"s
  if( H264InputDevice != NULL )
  {
       Medium::close(H264InputDevice);
  }

   if(shmctl(mid,IPC_RMID,0)==-1)
   {
       printf("shmctl(IPC_RMID) failed\n");
       exit(1);
   }


  env->reclaim();
  
  delete scheduler;

  return 0; // only to prevent compiler warning
}

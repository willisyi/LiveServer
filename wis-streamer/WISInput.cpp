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
// An interface to the WIS GO7007 capture device.
// Implementation


extern "C"
{
#include "share_mem.h"
#include "semaphore.h"
}
#include "WISInput.hh"
#include "Base64.hh"
#include "Err.hh"
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/shm.h>
#include <linux/soundcard.h>

int gSEMV;
int gSEMVE;

extern unsigned audioSamplingFrequency;
extern unsigned audioNumChannels;
extern int audio_enable;

#define TIME_GET_WAY 1

int semEmpty;
int semFull;
int semEmpty_audio;
int semFull_audio;

int I_start = 0;
//FILE *fp;


////////// WISOpenFileSource definition //////////

// A common "FramedSource" subclass, used for reading from an open file:

class WISOpenFileSource: public FramedSource {
public:
   int  uSecsToDelay;
   int  uSecsToDelayMax;
   int  srcType;

protected:
  WISOpenFileSource(UsageEnvironment& env, WISInput& input);
  virtual ~WISOpenFileSource();

  virtual int readFromFile() = 0;

private: // redefined virtual functions:
  virtual void doGetNextFrame();

private:
  static void incomingDataHandler(WISOpenFileSource* source);
  void incomingDataHandler1();

protected:
  WISInput& fInput;
};


////////// WISVideoOpenFileSource definition //////////

class WISVideoOpenFileSource: public WISOpenFileSource {
public:
  WISVideoOpenFileSource(UsageEnvironment& env, WISInput& input);
  virtual ~WISVideoOpenFileSource();


protected: // redefined virtual functions:
  virtual int readFromFile();
  int nal_state;
  int startI;

};



////////// WISAudioOpenFileSource definition //////////

class WISAudioOpenFileSource: public WISOpenFileSource {
public:
  WISAudioOpenFileSource(UsageEnvironment& env, WISInput& input);
  virtual ~WISAudioOpenFileSource();


protected: // redefined virtual functions:
  virtual int readFromFile();
  
   int getAudioData();

   struct timeval fPresentationTimePre;
   int IsStart;
   int audiobook;
   int offset;
/*
  unsigned int AudioBook;
  unsigned int AudioLock; 

  */
};




////////// WISInput implementation //////////

WISInput* WISInput::createNew(UsageEnvironment& env ,int vType) {
  return new WISInput(env,vType);
}

FramedSource* WISInput::videoSource() {
  if (fOurVideoSource == NULL) {
    fOurVideoSource = new WISVideoOpenFileSource(envir(), *this);
  }
  return fOurVideoSource;
}


FramedSource* WISInput::audioSource() {
  if (fOurAudioSource == NULL) {
    fOurAudioSource = new WISAudioOpenFileSource(envir(), *this);
  }
  return fOurAudioSource;
}



WISInput::WISInput(UsageEnvironment& env,int vType)
  : Medium(env),videoType(vType),fOurVideoSource(NULL),fOurAudioSource(NULL){
}

WISInput::~WISInput() {
	if(fOurVideoSource)
 	{
		delete (WISVideoOpenFileSource *)fOurVideoSource;
		fOurVideoSource = NULL;
 	}

	 if( fOurAudioSource )
 	{
 		delete (WISAudioOpenFileSource *)fOurAudioSource;
 		fOurAudioSource = NULL;
 	}
 
}


long timevaldiff(struct timeval *starttime, struct timeval *finishtime)
{
  long msec;
  msec = (finishtime->tv_sec-starttime->tv_sec)*1000;
  msec+=(finishtime->tv_usec-starttime->tv_usec)/1000;
  return msec;
}


static void printErr(UsageEnvironment& env, char const* str = NULL) {
  if (str != NULL) err(env) << str;
  env << ": " << strerror(env.getErrno()) << "\n";
}


////////// WISOpenFileSource implementation //////////

WISOpenFileSource
::WISOpenFileSource(UsageEnvironment& env, WISInput& input)
  : FramedSource(env),
    fInput(input){
}

WISOpenFileSource::~WISOpenFileSource() {
//  envir().taskScheduler().turnOffBackgroundReadHandling(fFileNo);
    I_start = 0;
}

void WISOpenFileSource::doGetNextFrame() {
  // Await the next incoming data :
  incomingDataHandler(this);
  
}


void WISOpenFileSource
::incomingDataHandler(WISOpenFileSource* source) {
  source->incomingDataHandler1();
}

void WISOpenFileSource::incomingDataHandler1() {
  	int ret;

        if (!isCurrentlyAwaitingData()) return;         

  // Read the data from share_memory
  	ret = readFromFile();

  	if (ret < 0) 
  	{
		handleClosure(this);
		fprintf(stderr,"In Grab Image, the source stops being readable!!!!\n");
 	}
 	else if (ret == 0)
 	{

		if( uSecsToDelay >= uSecsToDelayMax )
		{
			uSecsToDelay = uSecsToDelayMax;
		}else{
			uSecsToDelay *= 2;
		}
		nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
		      (TaskFunc*)incomingDataHandler, this);
 	}
	else {
		nextTask() = envir().taskScheduler().scheduleDelayedTask(0, (TaskFunc*)afterGetting, this); 
	}

}


////////// WISVideoOpenFileSource implementation //////////

WISVideoOpenFileSource
::WISVideoOpenFileSource(UsageEnvironment& env, WISInput& input)
  : WISOpenFileSource(env, input),nal_state(0),startI(0) {

  uSecsToDelay = 5000;
  //  uSecsToDelayMax = 33000/4;
  uSecsToDelayMax = 16700/4;
  //  fp = fopen("/mnt/video","wb");
  srcType = 0;
}

WISVideoOpenFileSource::~WISVideoOpenFileSource() {
  fInput.fOurVideoSource = NULL;
  //   del_semvalue(semEmpty);
  //   del_semvalue(semFull);

}

int WISVideoOpenFileSource::readFromFile()
{       
        char buffer[500000];
        char *ptr = buffer;
        AV_DATA av_data;
        unsigned int i, sps = 0, sps_len = 0,pps = 0;

        if (!semaphore_p(semFull)) 
        {
        	printf("semaphore_p() video error ! \n");
	       exit (1);
        }
	
       ShareMemRead(&av_data,buffer);  


	if(av_data.isIframe)
	{
	     if(nal_state == 0)
	     {
		    for(i=0; i<av_data.size; i++)
		      {
		      
				if(ptr[i] == 0 && ptr[i+1] == 0 && ptr[i+2] == 0 && ptr[i+3] == 1 && (ptr[i+4]&0x1f) == 7)
		          	sps = i+4; 
	              	if (sps) {
					if (ptr[i] == 0 && ptr[i+1] == 0 && ptr[i+2] == 0 && ptr[i+3] == 1 && (ptr[i+4]&0x1f) == 8)
					{
						sps_len = i-sps;
						break;
					}
				}
		    	}
	           if (sps_len) 
		    { 
			    fFrameSize = sps_len;
			    if (fFrameSize > fMaxSize) {
						fNumTruncatedBytes = fFrameSize - fMaxSize;
						fFrameSize = fMaxSize;
					}
			   else {
					   fNumTruncatedBytes = 0;
				    }
				    memcpy(fTo, ptr+sps, fFrameSize);
                            
		    }
		     else {
				     printf("SPS not found\n");
				     return -1;
				}
	     	}else if(nal_state == 1)
	     		{
	     		      for (i = 0; i < av_data.size; i++)
				{
					if (ptr[i] == 0 && ptr[i+1] == 0 && ptr[i+2] == 0 && ptr[i+3] == 1 && (ptr[i+4]&0x1f) == 8)
					{
						pps = i+4;
						break;
					}
				}
				if (pps) {
					fFrameSize = 5;
					if (fFrameSize > fMaxSize) {
					printf("Frame Truncated\n");
					fNumTruncatedBytes = fFrameSize - fMaxSize;
					fFrameSize = fMaxSize;
					}
					else 
					{
					     fNumTruncatedBytes = 0;
					}
					memcpy(fTo, ptr+pps, fFrameSize);
                             
				}

			}
			else
				{
				     for (i = 0; i < av_data.size; i++)
				    {
					    if (ptr[i] == 0 && ptr[i+1] == 0 && ptr[i+2] == 0 && ptr[i+3] == 1 && (ptr[i+4]&0x1f) == 5)
					   {	
	                                   i = i+4;
						  startI = 1;
                                          I_start = 1;
						  break;
					   }
				    } 
	                        fFrameSize = av_data.size-i;
				    if (fFrameSize > fMaxSize) {
	                                 fNumTruncatedBytes = fFrameSize - fMaxSize;
	                                 fFrameSize = fMaxSize;
	                            } else {
	                                  fNumTruncatedBytes = 0;
	                            }
				    memcpy(fTo, ptr+i, fFrameSize);
          	
				}
			
	           if(nal_state < 2)
	           	{
			      if (!semaphore_v(semFull))
		            {
		                 printf("semaphore_v() video error ! \n");
			           exit(1);
	  	            }
				
	           	}
		    else{
				if (!semaphore_v(semEmpty))
				{
		                    printf("semaphore_v() video error ! \n");
			              exit(1);
	  	             } 
			}
		    #if TIME_GET_WAY
		    gettimeofday(&fPresentationTime, NULL);
		    #else
		    fPresentationTime.tv_sec = av_data.timestamp/1000;
		    fPresentationTime.tv_usec = (av_data.timestamp%1000)*1000;
		    #endif
		    nal_state++;
		    //        fwrite(fTo,fFrameSize,1,fp);
		    return 1;
			    		     
	}

          	
	 if(!startI)
	   {
	        if (!semaphore_v(semEmpty))
	       {
	               printf("semaphore_v() video error ! \n");
		        exit(1);
  	       } 
 	
	    	 return 0;
	   }
	
	    fFrameSize = av_data.size-4;
           if (fFrameSize > fMaxSize) {
           fNumTruncatedBytes = fFrameSize - fMaxSize;
           fFrameSize = fMaxSize;
           } else {
           fNumTruncatedBytes = 0;
           }
          
	   memmove(fTo,buffer+4,fFrameSize);

          if (!semaphore_v(semEmpty))
	   {
	         printf("semaphore_v() video error ! \n");
		  exit(1);
  	   } 	

       #if TIME_GET_WAY	
	gettimeofday(&fPresentationTime, NULL);
	#else
	fPresentationTime.tv_sec = av_data.timestamp/1000;
	fPresentationTime.tv_usec = (av_data.timestamp%1000)*1000;
	#endif

	//	   fwrite(fTo,fFrameSize,1,fp);
	//	      fDurationInMicroseconds = 16700;
        return 1;
  }



////////// WISAudioOpenFileSource implementation //////////


WISAudioOpenFileSource
::WISAudioOpenFileSource(UsageEnvironment& env, WISInput& input)
  : WISOpenFileSource(env, input), IsStart(1),audiobook(0){
  uSecsToDelay = 5000;
  uSecsToDelayMax = 125000;
  srcType = 1;
  // fp = fopen("/mnt/audio","wb");
  //   semEmpty_audio= semget((key_t)1222, 1, 0666 | IPC_CREAT);
  //   semFull_audio= semget((key_t)1223, 1, 0666 | IPC_CREAT);
 
}

WISAudioOpenFileSource::~WISAudioOpenFileSource() {
  fInput.fOurAudioSource = NULL;
  //   del_semvalue(semEmpty_audio);
  //  del_semvalue(semFull_audio);
}


int WISAudioOpenFileSource::getAudioData() {

    char buffer[5000];
    AV_DATA av_data;
    
   
       semEmpty_audio = semget(SEMAE, 1, 0666 | IPC_CREAT);

       semFull_audio = semget(SEMA, 1, 0666 | IPC_CREAT);


        if (!semaphore_p(semFull_audio)) 
       {
	     printf("semaphore_p() audio error ! \n");
	     exit (1);
       }
	
	 AudioShareMemRead(&av_data,buffer);  
 	
        memmove(fTo,buffer,av_data.size);
	//	fwrite(fTo,1,av_data.size,fp);
	

       if (!semaphore_v(semEmpty_audio))
       {
            printf("semaphore_v() audio error ! \n");
            exit(1);
       }  
	      
  
#if TIME_GET_WAY
	   
		gettimeofday(&fPresentationTime, NULL);
#else
		fPresentationTime.tv_sec = av_data.timestamp/1000;
		fPresentationTime.tv_usec = (av_data.timestamp%1000)*1000;
#endif

 
 	return av_data.size;	
	
}
	



int WISAudioOpenFileSource::readFromFile() {
  // Read available audio data:
  int timeinc;
  int ret = 0;
       
  
   if( IsStart||I_start)
   {
  
         ret = getAudioData();
         IsStart = 0;
		 
   }
  if (ret <= 0) return 0;
  if (ret < 0) ret = 0;
  fFrameSize = (unsigned)ret;
  fNumTruncatedBytes = 0;

#if (TIME_GET_WAY)
  /* PR#2665 fix from Robin
   * Assuming audio format = AFMT_S16_LE
   * Get the current time
   * Substract the time increment of the audio oss buffer, which is equal to
   * buffer_size / channel_number / sample_rate / sample_size ==> 400+ millisec
   */


   timeinc = fFrameSize * 1000 / audioNumChannels / (audioSamplingFrequency/1000) ;
 
  while (fPresentationTime.tv_usec < timeinc)
  {
       fPresentationTime.tv_sec -= 1;
       timeinc -= 1000000;
  }
  fPresentationTime.tv_usec -= timeinc;
  

#else


  timeinc = fFrameSize*1000 / audioNumChannels / (audioSamplingFrequency/1000);
  if( IsStart )
  {
  	IsStart = 0;
  	fPresentationTimePre = fPresentationTime;
  	fDurationInMicroseconds = timeinc;
  }else{
	fDurationInMicroseconds = timevaldiff(&fPresentationTimePre, &fPresentationTime )*1000;
	fPresentationTimePre = fPresentationTime;
  }


  if( fDurationInMicroseconds < timeinc)
  {
  	unsigned long msec;
	msec = fPresentationTime.tv_usec;
  	msec += (timeinc - fDurationInMicroseconds);
	fPresentationTime.tv_sec += msec/1000000;
	fPresentationTime.tv_usec = msec%1000000;
	fDurationInMicroseconds = timeinc;

	fPresentationTimePre = fPresentationTime;
  }
#endif


  return 1;
}



int GetVolInfo(void *pBuff,int bufflen)
{
       AV_DATA av_data;

       semEmpty = semget(gSEMVE, 1, 0666 | IPC_CREAT);

  	semFull = semget(gSEMV, 1, 0666 | IPC_CREAT);

	  if (!semaphore_p(semFull)) 
	{
	        printf("semaphore_p() video_vol error ! \n");
                exit(1);	
  	}
	

        ShareMemRead(&av_data,pBuff);  
	    
        if (!semaphore_v(semEmpty))
	{
	       printf("semaphore_v() video_vol error ! \n");
               exit(1);	
  	}   
	 	
        return av_data.size;
}


int GetSprop(void *pBuff)
{
	static char tempBuff[200000];
	int ret = 0;
	int cnt = 0;
	int IsSPS = 0;
	int IsPPS = 0;
	int SPS_LEN = 0;
	int PPS_LEN = 4;
	char *pSPS = tempBuff;//0x7
	char *pPPS = tempBuff;//0x8
	char *pSPSEncode = NULL;
	char *pPPSEncode = NULL;

	ret = GetVolInfo(tempBuff,sizeof(tempBuff));

	for(;;)
	{
		if(pSPS[0] == 0 && pSPS[1] == 0 && pSPS[2] == 0 && pSPS[3] == 1)
		{
			if( (pSPS[4]& 0x1F) == 7 )
			{
				IsSPS = 1;
				break;
			}
		}
		pSPS++;
		cnt++;
		if( (cnt+4) > ret )
			break;
	}
	if(IsSPS)
		pSPS += 4;

	cnt = 0;
	for(;;)
	{
		if(pPPS[0] == 0 && pPPS[1] == 0 && pPPS[2] == 0 && pPPS[3] == 1)
		{
			if( (pPPS[4]& 0x1F) == 8 )
			{
				IsPPS = 1;
				break;
			}
		}
		pPPS++;
		cnt++;
		if( (cnt+4) > ret )
		{
		      pPPS += 4;
		      break;
		}
	}

	if(IsPPS)
	pPPS += 4;

	SPS_LEN = (unsigned int)pPPS-4 - (unsigned int)pSPS;
        printf("^^^^^^SPS_LEN is :^%d\n",SPS_LEN);

	pSPSEncode = base64Encode(pSPS,SPS_LEN);
	pPPSEncode = base64Encode(pPPS,PPS_LEN);

	sprintf((char *)pBuff,"%s,%s",(char *)pSPSEncode,(char *)pPPSEncode);

	delete[] pSPSEncode;
	delete[] pPPSEncode;

	return 1;
}

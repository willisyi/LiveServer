#define BUF_MAX_SZ 500000
//#define AUDIO_BUF_MAX_SZ 5000
#define SHM_KEY 33
//#define SHM_KEY_A 99


typedef struct  VideoBuf
{
    int     I_Frame;
    int     samplingRate;
    int     timestamp;
    int     data_size;
    char   buf_encode[BUF_MAX_SZ];
} VideoBuf;


typedef struct  AudioBuf
{
    unsigned char   numChannels;
    int     samplingRate;
    int     timestamp;
    int     data_size;
    char   buf_encode[3000];
} AudioBuf;


typedef struct shared_use
{ 
     VideoBuf  frame;
     AudioBuf  buf;
}shared_use;






typedef struct _av_data
{
	unsigned int serial;	/**< frame serial number */
	unsigned int size;		/**< frame size */
	unsigned int isIframe;	/**< I_FRAME is 1  or  P_FRAME  is  0*/
	unsigned int timestamp;	/**< get frame time stamp */
	unsigned char * ptr;	/**<  pointer for data ouput */
} AV_DATA;



int ShareMemInit(int key);  //intial shared memory
//int AudioShareMemInit(int key);
int ShareMemRead(AV_DATA * ptr,void *buf);
int AudioShareMemRead(AV_DATA * ptr,void *buf);
void Del_ShareMem(void * ptr);







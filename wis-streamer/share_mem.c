#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "share_mem.h"

static int shmid;
//static int shmid_audio;

int ShareMemInit(int key)
{
	shmid = shmget(key, sizeof( shared_use), IPC_CREAT | 0660);
	if(shmid < 0)
		shmid = shmget(key, 0, 0);
	if(shmid < 0)
		return -1;
	return shmid;
}

/*int AudioShareMemInit(int key)
{
	shmid_audio = shmget(key, sizeof(struct shared_audio), IPC_CREAT | 0660);
	if(shmid_audio < 0)
		shmid_audio = shmget(key, 0, 0);
	if(shmid_audio < 0)
		return -1;
	return shmid_audio;
}
*/

int AudioShareMemRead(AV_DATA * ptr,void *buf)
{
 
  void *pSrc = shmat(shmid,(void *)0,0);
  shared_use *shared_stuff;
  if(pSrc==(void *)-1)
        {
    	      printf("Src shmat error\n");
		exit(1);
    	}
  shared_stuff = ( shared_use*)pSrc;
  memcpy(buf,(void *)shared_stuff->buf.buf_encode,shared_stuff->buf.data_size);
  ptr->size = shared_stuff->buf.data_size;
  ptr->timestamp = shared_stuff->buf.timestamp;
  shmdt(pSrc);
  return 1;
}

int ShareMemRead(AV_DATA * ptr,void *buf)
{ 
 
  void *pSrc = shmat(shmid,(void *)0,0);
  shared_use *shared_stuff;
  if(pSrc == (void *)-1)
        {
    	      printf("Src shmat error\n");
		exit(1);
    	}
  shared_stuff = ( shared_use *)pSrc;
  memcpy(buf,(void *)shared_stuff->frame.buf_encode,shared_stuff->frame.data_size);
  ptr->size = shared_stuff->frame.data_size;
  ptr->isIframe = shared_stuff->frame.I_Frame;
  ptr->timestamp = shared_stuff->frame.timestamp;
  shmdt(pSrc);
  return 1;
}


void Del_ShareMem(void * ptr)
{
   if(shmdt(ptr)==-1)
   {
       printf("shmdt failed\n");
       exit(1);
   }
   if(shmctl(shmid,IPC_RMID,0)==-1)
   {
       printf("shmctl(IPC_RMID) failed\n");
       exit(1);
   }
}


#include <stdio.h>

#include "semaphore.h" 


int set_semvalue(int sem_id,int val)
{
	union semun sem_union;

	sem_union.val = val;
	if(semctl(sem_id, 0, SETVAL, sem_union) == -1) return(0);
	return(1);
}

/* 函数del_semvalue通过semctl调用清理信号量,
 * 将command参数设置为IPC_RMID,表示要将删除一个已经无需使用的信号量标识符
 */
void del_semvalue(int sem_id)
{
	union semun sem_union;
    
	if (semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
		fprintf(stderr, "Failed to delete semaphore\n");
}

/* 函数semaphore_p 执行P()操作,将sem_b结构中的sem_op设置为-1. */
int semaphore_p(int sem_id)
{
	struct sembuf sem_b;
    
	sem_b.sem_num = 0;
	sem_b.sem_op = -1; /* P() */
	sem_b.sem_flg = SEM_UNDO;
	if (semop(sem_id, &sem_b, 1) == -1) 
	{
		fprintf(stderr, "semaphore_p failed\n");
		return(0);
	}
	return(1);
}

/* 函数semaphore_v执行V()操作,将sem_b结构中的sem_op设置为+1 */
int semaphore_v(int sem_id)
{
	struct sembuf sem_b;
    
	sem_b.sem_num = 0;
	sem_b.sem_op = 1; /* V() */
	sem_b.sem_flg = SEM_UNDO;
	if (semop(sem_id, &sem_b, 1) == -1) 
	{
		fprintf(stderr, "semaphore_v failed\n");
		return(0);
    }
	return(1);
}


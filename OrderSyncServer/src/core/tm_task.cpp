#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
// #include <asm/atomic.h>
#include "atomic.h"
#include <time.h>
#include "tm_task.h"

static void do_kill(int signo, int procid)
{
		char cmd_buf[64] = {0};

		if(procid <= 0)
				return;
		snprintf(cmd_buf, sizeof(cmd_buf) - 1, "kill -%d %u > /dev/null 2>&1", signo, procid);	
		system(cmd_buf);

}


static void* map_file (const char* filename, int size, bool& f_new)
{
		f_new = true;
		int t_fd = ::open(filename, O_RDONLY);
		if(t_fd > 0)
		{
				::close(t_fd);
				f_new = false;
				//printf("%s is old\n", filename);
		}

		int fd = ::open(filename, O_RDWR|O_CREAT, 0666);
		void *map = NULL;

		if(fd >= 0)
		{
				if(size > 0)
						ftruncate(fd, size);
				else
						size = lseek(fd, 0L, SEEK_END);

				if(size > 0)
						map = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
				::close(fd);
		} else if(size > 0) {
				map = mmap(0, size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS, -1, 0);
		}

		if(map == MAP_FAILED)
		{
				map = NULL;
		}

		return map;
}

struct time_mgr
{
		struct timeval  tval;
		int  tm;
		atomic_t tm_fag;
		atomic_t tval_fag;
		int pid;
};


void get_timetask(const char* tmfile)
{
		if(!tmfile)
		{
				tmfile = "./timemgrtask";
		}
		bool f_new;
		struct time_mgr* ptm_mgr = (struct time_mgr*)  map_file("./timemgrtask",sizeof(struct time_mgr), f_new);
		if(!ptm_mgr)
		{
				printf("map ./timemgrtask fail\n ");
				printf("create time mgr task fail\n");
				exit(0);
		}

		gettimeofday(&ptm_mgr->tval, NULL);
		ptm_mgr->tm = time(NULL);

		if(!f_new)
		{
				//printf("tm task %d\n", ptm_mgr->pid);
				if(ptm_mgr->pid != 0)
						do_kill(9, ptm_mgr->pid);   	       	   
				ptm_mgr->pid = 0;
		}

		int pid = fork();



		if(pid < 0)
		{
				printf("create time mgr task fail: %m\n");		 
				exit(0);
		}

		if(pid > 0)
		{
				ptm_mgr->pid = pid;
				return;
		}


		ptm_mgr->pid = getpid();
		int count = 0;

		while(true)
		{
				atomic_add(1,&ptm_mgr->tval_fag);
				gettimeofday(&ptm_mgr->tval, NULL);

				count++;
				if(count > 20)
				{
						count = 0;
						//atomic_set(&ptm_mgr->tm_fag, 1);	
						atomic_add(1,&ptm_mgr->tm_fag);
						ptm_mgr->tm = time(NULL);
						//atomic_set(&ptm_mgr->tm_fag, 0);				   
				}
				usleep(5000);		  
		}

}

int get_time(const char* tmfile)
{
		if(!tmfile)
		{
				tmfile = "./timemgrtask";
		}

		bool f_new;
		static  struct time_mgr* ptm_mgr = (struct time_mgr*)  map_file(tmfile,sizeof(struct time_mgr), f_new);

		if(!ptm_mgr)
				return time(NULL);

		int c_tm = 0;
		int flag;

		do
		{
				flag = atomic_read(&ptm_mgr->tm_fag) ;

				c_tm = ptm_mgr->tm;
		}while(atomic_read(&ptm_mgr->tm_fag) != flag);  

		return c_tm;

}

void get_timeofday(struct timeval* pval,const char* tmfile)
{
		if(!tmfile)
		{
				tmfile = "./timemgrtask";
		}
		bool f_new;
		static  struct time_mgr* ptm_mgr = (struct time_mgr*)  map_file(tmfile,sizeof(struct time_mgr), f_new);   

		if(!ptm_mgr)
		{
				gettimeofday(pval,NULL);
				return;
		}

		// while(atomic_read(&ptm_mgr->tval_fag) != 0);
		int flag = 0;

		do
		{
				flag = atomic_read(&ptm_mgr->tval_fag) ;
				pval->tv_sec = ptm_mgr->tval.tv_sec;
				pval->tv_usec = ptm_mgr->tval.tv_usec;
		}while(atomic_read(&ptm_mgr->tval_fag) != flag);

		return;	

}

int getdelay(const struct timeval& begin)
{
		struct timeval now;
		//gettimeofday(&now, NULL);
		get_timeofday(&now);
		unsigned delay = 0;
		if((delay = now.tv_sec - begin.tv_sec) > 0)
		{
				delay *= 1000;	
		}

		if(now.tv_usec > begin.tv_usec)
		{
				delay += (now.tv_usec - begin.tv_usec) / 1000;		
		}
		else
		{
				delay -= (begin.tv_usec - now.tv_usec) / 1000;
		}

		return delay;
}

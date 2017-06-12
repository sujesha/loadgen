 /* Needs root permission, inside VM.
 * Needs write permission in /root/files/ 
 */

/* Disclaimer: Though theoretically, this program could create multiple threads
 *             of same type, like multiple 'R', or 'W' threads, it is not practically
 *             possible because some global variables might be used per thread-type
 *             say, read_file_index, which will get (inadvertently) shared among
 * 			   the threads. Originally, there should be one such variable per thread
 *             but that may not be possible in current design.
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <math.h>

#include <net/ethernet.h>
#include <netinet/ether.h>
#include <netinet/ip.h>

#include <string.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>

#include <unistd.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>



#define MIN_INTERVAL	5
#define SSH_NET_CREATE() "ssh root@%s 'dd if=/dev/urandom of=/var/www/Doc%ld.txt bs=4k count=%ld 2> /dev/null'"
//#define DISK_CREATE() "dd if=/dev/urandom of=/root/files/%s%ld bs=4k count=%ld conv=fdatasync"
#define DISK_CREATE() "dd if=/dev/urandom of=/root/files/%s%ld bs=4k count=%ld"
#define NETLOAD() "wget -O netfile%ld.txt http://%s/Doc%ld.txt 2> /dev/null"
#define MAKE_NETFILE_STR(str, x) do { sprintf(str, "netfile%ld.txt", x); } while(0);
#define MAKE_HTTP_STR(str, x, y) do { sprintf(str, "http://%s/Doc%ld.txt", x, y); } while(0);
//#define WLOAD() "dd if=/dev/zero of=/root/files/%s%ld bs=4k count=%ld conv=fdatasync"
#define WLOAD() "dd if=/dev/zero of=/root/files/%s%ld bs=4k count=%ld"
#define RLOAD() "dd if=/root/files/%s%ld of=/dev/null bs=4k count=%ld 2> /dev/null"
#define MAKE_OF_STR(str, x, y)	do { sprintf(str, "of=/root/files/%s%ld", x, y); } while(0);
#define MAKE_IF_STR(str, x, y)	do { sprintf(str, "if=/root/files/%s%ld", x, y); } while(0);
#define MAKE_COUNT_STR(str, x)	do { sprintf(str, "count=%ld", x); } while(0);
#define MAKE_INDEX_FILENAME_STR(str, x)	do { sprintf(str, "%s_fileindex.sss.txt", x); } while(0);
#define ARG(x)	(point_to_arg_array + num_threads)->x
#define CPU(x)  (point_to_arg_array + num_threads)->union_arg.cpu_arg.x
#define NET(x)  (point_to_arg_array + num_threads)->union_arg.net_arg.x
#define DISK_RD(x)  (point_to_arg_array + num_threads)->union_arg.disk_rd_arg.x
#define DISK_WR(x)  (point_to_arg_array + num_threads)->union_arg.disk_wr_arg.x

#define COUNT_FIBO_TERMS 2000000
#define TIME_FOR_2000000_FIBO_TERMS 10
#define ERRFILE "generate_loads.err.sss.txt"
#define LOGFILE "generate_loads.log.sss.txt"
#define STDOUT_FD 1

    struct timeval start, end, first;
	double startTime = 0;
	double endTime = 0;
	double firstTime = 0;
	long eff_us_interval = 0;

//char errfile[256];
FILE *errfile_p = NULL;
int logfile_fd;
int dup_fd;
char logfile_buf[256];

/* Global variables */
long num_threads=0;
long num_threads_started=0;
pthread_t *point_to_array;
struct generic_arg_t *point_to_arg_array;
long read_file_index;

char netload_cmd[256];

int daemonize()
{
    pid_t   pid;

#ifdef DEBUG_SS
        sprintf(logfile_buf, "in daemonize\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

//	printf("in daemonize() \n");
    if ( (pid = fork()) < 0)
    {
		printf("fork failed\n");
    	exit(-1);
    }
    else if (pid != 0)
    {
#ifdef DEBUG_SS
        sprintf(logfile_buf, "parent pid = %d\n", pid); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

//        printf("parent pid = %d\n", pid);
        exit(0);
    }       /* parent goes bye-bye */

    /* child continues */
#ifdef DEBUG_SS
        sprintf(logfile_buf, "child continues pid = %d\n", pid); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
//	printf("child continues pid = %d\n", pid);
    setsid();       /* become session leader */

#ifdef DEBUG_SS
        sprintf(logfile_buf, "clear our file mode creation mask\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
//	printf("clear our file mode creation mask\n");
    umask(0);       /* clear our file mode creation mask */

    return 0 ;
}

/* Arguments specific to disk write load */
struct disk_wr_load_arg_t
{
    long num_4k;					/* Number of 4k blocks in a file - determines file size to write */
    char prefix[256];			/* Prefix of file name - Different prefix for consecutive runs to be used by wrapper */
	long twice_ram_num;			/* Number of files of size num_4k needed to cover RAM twice */
};

/* Arguments specific to disk read load */
struct disk_rd_load_arg_t
{
	int pick;					/* 0, 1 or 2 */
    long num_4k;					/* Number of 4k blocks in a file - determines file size to read */
	char prefix[256];			/* Prefix of file name - Different prefix for consecutive runs to be used by wrapper */
	long twice_ram_num;			/* Number of files of size num_4k needed to cover RAM twice */
	char file_index_filename[256];		/* To record file index, first check for existence, if no, then index = 1, else read value + 1. If index > count, index = 1 */
};

/* Arguments specific to net Rx load (or Tx wget-requests) */
struct net_load_arg_t
{
	long kbps;					/* Rx rate */
	long num_4k;					/* Number of 4k blocks in a file - determines file size to request via wget */
	long interval;
    long batch;					/* Used if interval calculated as < MIN_INTERVAL msec. "batch" is the numbers of requests to run in a single thread before sleeping for interval */
	char webserver[256];		/* Webserver's IP */
};

/* Arguments specific to cpu load */
struct cpu_load_arg_t
{
	long perc;
	long interval;
    long batch;				/* Used if interval calculated as < MIN_INTERVAL msec. "batch" is the numbers of requests to run in a single thread before sleeping for interval */
};

/* Load can be of one of the following types - cpu, net, disk-rd, disk-wr */
typedef union
{
	struct cpu_load_arg_t cpu_arg;
	struct net_load_arg_t net_arg;
	struct disk_rd_load_arg_t disk_rd_arg;
	struct disk_wr_load_arg_t disk_wr_arg;
}union_arg_t;

/* Generic structure to hold arguments to the load threads */
struct generic_arg_t
{
	char thread_type[10];
	union_arg_t union_arg;
};

struct batch_req_t
{
    char prefix[256];
    char webserver[256];
	long iteration;
	long num_4k;
	char load_type[256];
	char file_index_filename[256];
};

long fibonacci(long n)
{
	long a = 0;
	long b = 1;
	long sum = 0;
	int i = 0;
	long bigger_sum = 0;
#ifdef DEBUG_SS
	sprintf(logfile_buf, "fibonacci\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif


	for (i=1; i<=n; i++)
	{
#ifdef DEBUG_SS
		sprintf(logfile_buf, "%ld\t", b); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
		sum = a + b;
		a = b;
		b = sum;
		bigger_sum+=sum;
	}
	return bigger_sum;
}

int read_integer_from_file(char *filename)
{
        FILE *fptr;
        long value;
        struct stat file_stat;

        /* If file has not yet been created, then create and write 0 in it */
        if (stat(filename, &file_stat))
        {
                fprintf(errfile_p, "File %s doesnt exist, so create and write 0 in it\n", filename);
#ifdef DEBUG_SS 
                sprintf(logfile_buf, "File %s doesnt exist, so create and write 0 in it\n", filename); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
                fptr = fopen(filename, "w");
                        if (fptr == NULL)
                {
#ifdef DEBUG_SS
                        sprintf(logfile_buf, "%s can not be created for writing 0\n", filename); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#else
                        fprintf(errfile_p, "%s can not be created for writing 0\n", filename);
#endif
                        exit(-1);
                }
                value = 0;
                fprintf(fptr, "%ld", value);
#ifdef DEBUG_SS
                sprintf(logfile_buf, "New read file index written value = %ld\n", value); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

                fclose(fptr);

        }
        /* Open/re-open the file and read up the value */
        fptr = fopen(filename, "r");
        if (fptr == NULL)
        {
#ifdef DEBUG_SS
        sprintf(logfile_buf, "%s can not be opened for reading\n", filename); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#else
                fprintf(errfile_p, "%s can not be opened for reading\n", filename);
#endif
                exit(-1);
        }

        fscanf(fptr, "%ld", &value);
#ifdef DEBUG_SS
        sprintf(logfile_buf, "Last read file index = %ld\n", value); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

        fclose(fptr);
}

void write_integer_into_file(char *filename, long value)
{
        FILE *fptr;

        fptr = fopen(filename, "w");
        if (fptr == NULL)
        {
#ifdef DEBUG_SS
        sprintf(logfile_buf, "%s can not be opened for writing\n", filename); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#else
                fprintf(errfile_p, "%s can not be opened for writing\n", filename);
#endif
                exit(-1);
        }

        fprintf(fptr, "%ld", value);
#ifdef DEBUG_SS
        sprintf(logfile_buf, "Last read file index written value = %ld\n", value); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

        fclose(fptr);
}


void dur_alarm_catcher(int parm)
{
	long index=0;
	pthread_t temp;

	temp = point_to_array[0];
	free(point_to_array);
	free(point_to_arg_array);
	close(logfile_fd);

	/* Sending SIGKILL signal to first (any) thread is enough to kill whole process */
	pthread_kill(temp, SIGKILL);

}

void batch_exec_system(void *arg)
{
	struct batch_req_t *ptr;
	int pfd[2];
	ptr = (struct batch_req_t*)arg;

/* Only needed inside the child process, but added here to avoid data manipulation between vfork() and execl() ---- */
//		char disk_cmd[256];
//		char cmd[256], prefix[25], load_type[25], webserver[25];
//		long iteration, num_4k, batch;
		char if_str[256], of_str[256], count_str[25], netfile_str[256], http_str[256];
		int ret;

		int dev_null_fd;

		MAKE_OF_STR(of_str, ptr->prefix, ptr->iteration);
		MAKE_COUNT_STR(count_str, ptr->num_4k);
	 	MAKE_IF_STR(if_str, ptr->prefix, ptr->iteration);
		MAKE_NETFILE_STR(netfile_str, ptr->num_4k);
		MAKE_HTTP_STR(http_str, ptr->webserver, ptr->num_4k);
/* ---- */


	int pstat;
	pid_t   pid;

//#ifndef DEBUG_SS
/* The following patch of code for duping /dev/null to stderr, prevents execl from spewing output on screen */
	dev_null_fd = open("/dev/null", O_WRONLY);
	if (dev_null_fd < 0)
	{
		fprintf(errfile_p, "/dev/null can not be opened for O_WRONLY\n");
		exit(-1);
	}

	close(2);
	dup2(dev_null_fd, 2);
	close(dev_null_fd);		/* Freeing the to-be-unused descriptor */
/* The above patch of code for duping /dev/null to stderr, prevents execl from spewing output on screen */
//#endif

#ifdef DEBUG_SS
    sprintf(logfile_buf, "batch_exec_system: Dup'ed stderr to send execl output to /dev/null fd = %d\n", dev_null_fd); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

    if ( (pid = vfork()) < 0)
    {
        fprintf(errfile_p, "vfork failed pid = %d\n", pid);
        exit(-1);
    }
    else if (pid == 0)
    {
#ifdef TIME_DEBUG_SS
          gettimeofday(&start, NULL);
          startTime = 1.0*start.tv_sec + 1e-6 * start.tv_usec;
		  sprintf(logfile_buf, "Time: %f, Just after entering vfork() child\n", startTime); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

/*
		strcpy(cmd, ptr->cmd);
		strcpy(prefix, ptr->prefix);
		iteration = ptr->iteration;
		num_4k = ptr->num_4k;
		batch = ptr->batch;
		strcpy(load_type, ptr->load_type);
		strcpy(webserver, ptr->webserver);
*/

			switch(ptr->load_type[0])
			{
				case 'W':
				case 'w': 
#ifdef DEBUG_SS
					sprintf(logfile_buf, "batch_exec_system: dd command\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

					/* This execl() call will replace everything beyond this. So nothing matters except the parent case beyond this */
					//ret = execl("/bin/dd", "dd", "if=/dev/zero", of_str, "bs=4k", count_str, "conv=fdatasync", (char *)0);
					ret = execl("/bin/dd", "dd", "if=/dev/zero", of_str, "bs=4k", count_str, (char *)0);

					/* We will come here only if execl() fails */
					if (ret < 0)
					{
						fprintf(errfile_p, "execl for W failed\n");
						_exit(0);
					}
					break;
				case 'R':
				case 'r':
#ifdef DEBUG_SS
					sprintf(logfile_buf, "exec_system: dd command\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
					write_integer_into_file(ptr->file_index_filename, ptr->iteration);

					/* This execl() call will replace everything beyond this. So nothing matters except the parent case beyond this */
	                ret = execl("/bin/dd", "dd", if_str, "of=/dev/null", "bs=4k", count_str, (char *)0);

	                /* We will come here only if execl() fails */
	                if (ret < 0)
	                {
						fprintf(errfile_p, "execl for R failed\n");
	                    _exit(0);
	                }
	                break;

				case 'N':
				case 'n':
#ifdef DEBUG_SS
					sprintf(logfile_buf, "exec_system: start\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

					/* This execl() call will replace everything beyond this. So nothing matters except the parent case beyond this */
                    //ret = execl("/usr/bin/wget", "wget", "-O", netfile_str, http_str, (char *)0);
#ifdef TIME_DEBUG_SS
          gettimeofday(&start, NULL);
          startTime = 1.0*start.tv_sec + 1e-6 * start.tv_usec;
		  sprintf(logfile_buf, "Time: %f, Just before execl() for wget \n", startTime); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
                    ret = execl("/usr/bin/wget", "wget", "-O", netfile_str, http_str, (char *)0);

                    /* We will come here only if execl() fails */
                    if (ret < 0)
                    {
						fprintf(errfile_p, "execl for N failed\n");
                        _exit(0);
                    }
                    break;

				default:
#ifdef DEBUG_SS			
					sprintf(logfile_buf, "exec_system: should never come here x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif			
					break;
			}
    }
	else
	{
/*
#ifdef TIME_DEBUG_SS
          gettimeofday(&start, NULL);
          startTime = 1.0*start.tv_sec + 1e-6 * start.tv_usec;
		  sprintf(logfile_buf, "Time: %f, Just before waitpid() in vfork() parent \n", startTime); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
*/
		//waitpid(pid, NULL, WAIT_ANY);
		pid = waitpid(pid, &pstat, 0);
/*
#ifdef TIME_DEBUG_SS
          gettimeofday(&start, NULL);
          startTime = 1.0*start.tv_sec + 1e-6 * start.tv_usec;
		  sprintf(logfile_buf, "Time: %f, Just after waitpid() in vfork() parent \n", startTime); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
*/
		if (pid == -1 || !WIFEXITED(pstat) || WEXITSTATUS(pstat) != 0)
		{
#ifdef DEBUG_SS
			sprintf(logfile_buf, "batch_exec_system: Error with waitpid %d\n", pid); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
			fprintf(errfile_p, "batch_exec_system: Error with waitpid %d\n", pid);
			exit(-1);
		}
#ifdef DEBUG_SS
		else
		{
			sprintf(logfile_buf, "batch_exec_system: Child returned for thread number %ld\n", ptr->iteration); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		}
#endif
		
	}

	//pthread_exit(0);		//Not needed if thread is DETACHED or if NO threading.
}

/* This function needs to be used only for Net loads */
void make_net_req_thread(long iteration, long num_4k, long batch, char *webserver, char *load_type)
{
	pthread_t batch_req_thread;
	pthread_attr_t tattr;

	int single_iret, ret;
	long i = 0;

	struct batch_req_t batch_req;
	strcpy(batch_req.prefix, "");
	batch_req.iteration = iteration;
	batch_req.num_4k = num_4k;
	strcpy(batch_req.webserver, webserver);
	strcpy(batch_req.load_type, load_type);

	/* set the thread detach state */
	/* If thread is needed, uncomment this -------
	ret = pthread_attr_init(&tattr);
	ret = pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
	if (ret != 0)
	{
		fprintf(errfile_p, "Error while detaching thread\n");
		exit(-1);
	}
	

#ifdef DEBUG_SS
	sprintf(logfile_buf, "Trying to create batch thread %ld\n", iteration); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif	
	 ---- */

	/* For net load, batch_exec_system() is called as a thread, for disk, it is directly called */
	/* If thread is needed, uncomment this -------
	single_iret = pthread_create(&batch_req_thread, &tattr, (void*)batch_exec_system, (void*)&batch_req);
	if (single_iret < 0)
	{
		fprintf(errfile_p, "Could not initialize single request thread for command");
        exit(-1);
    }

	pthread_attr_destroy(&tattr);
	 ---- */

	/* Trying to directly call batch_exec_system(), instead of via thread - same as the disk loads */
	for (i=0; i<batch; i++)
	{
		batch_exec_system((void *)&batch_req);
	}
}


void generate_cpu_load(void *arg1)
{
	long interval=0, batch=0;
	struct generic_arg_t *ptr;
	long i = 0;

	ptr = (struct generic_arg_t*)arg1;
	interval = ptr->union_arg.cpu_arg.interval;
	batch = ptr->union_arg.cpu_arg.batch;
	long result = 0;
	while(1)
	{
#ifdef DEBUG_SS
		sprintf(logfile_buf, "Generate cpu load for perc %ld and interval %ld\n", batch, interval); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
//		result = 1;
#endif
		for(i=1; i<=batch; i++)
		{
			result = fibonacci(COUNT_FIBO_TERMS);
		}
		result!=0? usleep(interval*1000):fprintf(errfile_p, "generate_loads: fibonacci result=0\n");
	}
	pthread_exit(NULL);
}

void generate_net_load(void *arg1)
{
	long interval=0, num_4k=0, batch=1;
	char webserver[256];
	struct generic_arg_t *ptr;
	long iteration = 0;


	ptr = (struct generic_arg_t*)arg1;
	interval = ptr->union_arg.net_arg.interval;
	num_4k = ptr->union_arg.net_arg.num_4k;
	strcpy(webserver, ptr->union_arg.net_arg.webserver);

#ifdef DEBUG_SS
	sprintf(logfile_buf, "Generating net load\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

	while(1)
	{
        gettimeofday(&first, NULL);
        firstTime = 1.0*first.tv_sec + 1e-6 * first.tv_usec;
#ifdef TIME_DEBUG_SS
		sprintf(logfile_buf, "Time: %f, Before call to make net thread\n", firstTime); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
		iteration++;
//		sprintf(netload_cmd, NETLOAD(), num_4k, webserver, num_4k);
#ifdef DEBUG_SS
		sprintf(logfile_buf, "In net load, thread %ld\n", iteration); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		make_net_req_thread(iteration, num_4k, batch, webserver, "N");
#else
		make_net_req_thread(iteration, num_4k, batch, webserver, "N");
#endif

        gettimeofday(&end, NULL);
        endTime = 1.0*end.tv_sec + 1e-6 * end.tv_usec;
#ifdef TIME_DEBUG_SS
		sprintf(logfile_buf, "Time: %f, After call to make net thread\n", endTime); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
		eff_us_interval = (int)(interval*1000.0 - (endTime - firstTime)*1000000.0);
 		if (eff_us_interval > 0)
		{
			usleep(eff_us_interval);
#ifdef TIME_DEBUG_SS
			sprintf(logfile_buf, "Slept for %ld us\n", eff_us_interval); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
		}
		else
		{
			fprintf(errfile_p, "No time to sleep, already took %g us\n", (endTime-firstTime)*1000000); 
		}
		
	//	usleep(interval*1000);
#ifdef TIME_DEBUG_SS
          gettimeofday(&start, NULL);
          startTime = 1.0*start.tv_sec + 1e-6 * start.tv_usec;
		  sprintf(logfile_buf, "Time: %f, Just after interval of %d ms\n", startTime, interval); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif 
		

	}
	pthread_exit(NULL);
}

long read_up_file_index()
{
	long file_index;
	

	return file_index;
}

void generate_disk_rd_load(void *arg1)
{
	long interval=0, num_4k=0, batch=0;
	struct generic_arg_t *ptr;
	char prefix[256], rload_cmd[256];
	char index_filename[256];
	long iteration = 0;
    struct batch_req_t batch_req;
	int pick;
	long value;

	ptr = (struct generic_arg_t*)arg1;
    num_4k = ptr->union_arg.disk_rd_arg.num_4k;
    strcpy(prefix, ptr->union_arg.disk_rd_arg.prefix);
    pick = ptr->union_arg.disk_rd_arg.pick;
	strcpy(index_filename, ptr->union_arg.disk_rd_arg.file_index_filename);

	if (pick == 0)
	{
		value = read_integer_from_file(index_filename);
		if (value != 0)
		{
			fprintf(errfile_p, "value %ld can not be non-zero for pick=0 case for disk read load\n", value);
#ifdef DEBUG_SS
			sprintf(logfile_buf, "value %ld can not be non-zero for pick=0 case for disk read load\n", value); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif  
			exit(-1);
		}
		iteration = 1;
	
    }
    else if (pick == 1)
	{
		/* Read the index of last file read, and increment it to read next file */
		iteration = read_integer_from_file(index_filename) + 1;

		/* If the last file read was the last file available, then the next file is the 1st one */
		if (iteration > ptr->union_arg.disk_rd_arg.twice_ram_num) 
		{
			iteration = 1;
		}
	}
	else
	{
		fprintf(errfile_p, "Invalid pick value %d in generate_disk_rd_load()\n", pick);
#ifdef DEBUG_SS
    	sprintf(logfile_buf, "Invalid pick value %d in generate_disk_rd_load()\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
		exit(-1);
	}


#ifdef DEBUG_SS
	sprintf(logfile_buf, "Generating disk-rd load\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
    while(1)
    {

        /* Disk load doesnt need thread, because there is no sleep interval. So, directly invoke batch_exec_system() instead of invoking it as a thread handler */
        strcpy(batch_req.prefix, prefix);
        batch_req.iteration = iteration;
        batch_req.num_4k = num_4k;
        strcpy(batch_req.load_type, "R");
		strcpy(batch_req.file_index_filename, index_filename);

        batch_exec_system((void *)&batch_req);
		usleep(1000);		//To give some respite between requests 
        if (iteration == ptr->union_arg.disk_rd_arg.twice_ram_num)
		{
			iteration = 1;
        }
        else
        {
            iteration++;
        }
    }
    pthread_exit(NULL);
}

void generate_disk_wr_load(void *arg1)
{
	long interval=0, num_4k=0;
	struct generic_arg_t *ptr;
	char prefix[256];
	long iteration = 1;
    struct batch_req_t batch_req;

	ptr = (struct generic_arg_t*)arg1;
    num_4k = ptr->union_arg.disk_wr_arg.num_4k;
	strcpy(prefix, ptr->union_arg.disk_wr_arg.prefix);


#ifdef DEBUG_SS
	sprintf(logfile_buf, "Generating disk-wr load\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

	while(1)
	{
		if (iteration > ptr->union_arg.disk_wr_arg.twice_ram_num)
		{
			iteration = 1;
		}
		else
		{
			iteration++;
		}
#ifdef DEBUG_SS
		sprintf(logfile_buf, "In disk-wr load\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

		/* Disk load doesnt need thread, because there is no sleep interval. So, directly invoke batch_exec_system() instead of invoking it as a thread handler */
   		strcpy(batch_req.prefix, prefix);
	    batch_req.iteration = iteration;
    	batch_req.num_4k = num_4k;
	    strcpy(batch_req.load_type, "W");

		batch_exec_system((void *)&batch_req);
	}
	pthread_exit(NULL);
}


//pthread_t make_thread(char *thread_type, int arg_index, char *interval, char *number, char *name)
pthread_t make_thread(struct generic_arg_t* thread_arg, int arg_index)
{
		pthread_t loading_thread;
		int loading_iret;

		/* Already passing the pointer to thread_arg, so dont need the following 3 */
		/*
		struct generic_arg_t thread_arg;
		thread_arg.interval = atoi(interval);
	    thread_arg.number = atoi(number);
		strcpy(thread_arg.name, name);
   		*/
		
	
		/* Assigning different loading functions to threads, based on the input thread_type */	
		if (strcmp(thread_arg->thread_type,"cpu") == 0)
		{
			loading_iret = pthread_create(&loading_thread, NULL, (void*)generate_cpu_load, (void*)thread_arg);
		}
		else if (strcmp(thread_arg->thread_type,"net") == 0)
        {
            loading_iret = pthread_create(&loading_thread, NULL, (void*)generate_net_load, (void*)thread_arg);
#ifdef DEBUG_SS
			sprintf(logfile_buf, "Created net thread\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

        }
		else if (strcmp(thread_arg->thread_type,"disk_rd") == 0)
        {
            loading_iret = pthread_create(&loading_thread, NULL, (void*)generate_disk_rd_load, (void*)thread_arg);
        }
		else if (strcmp(thread_arg->thread_type,"disk_wr") == 0)
        {
            loading_iret = pthread_create(&loading_thread, NULL, (void*)generate_disk_wr_load, (void*)thread_arg);
        }
		else
		{
			fprintf(errfile_p, "Thread-type %s not handled\n", thread_arg->thread_type);
			exit(-1);
		}

		if (loading_iret < 0)
		{
			fprintf(errfile_p, "Could not initialize %s thread, arg number %d", thread_arg->thread_type, arg_index);
			exit(-1);
		}
		return loading_thread;
}

/* prep_for_generate_loads.c will read RAM size from /proc/meminfo and write to /root/RAMSIZE_k. Here, we simply read it up */
long read_ram_size_in_k_from_file()
{
	FILE *fp;
	long RAMSIZE_k = 0;

	fp = fopen("/root/RAMSIZE_k", "r");
	if (fp == NULL)
	{
#ifdef DEBUG_SS
        sprintf(logfile_buf, "/root/RAMSIZE_k can not be opened\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#else
		fprintf(errfile_p, "/root/RAMSIZE_k can not be opened\n");
#endif
		exit(-1);
	}
		
	fscanf(fp, "%ld", &RAMSIZE_k);
#ifdef DEBUG_SS
	sprintf(logfile_buf, "RAMSIZE = %ld kB\n", RAMSIZE_k); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

	fclose(fp);
	return RAMSIZE_k;
}

int main(int argc, char *argv[])
{
	int arg_index=0;
	int index=0;
	int daemonize_flag = 0;
	int dur_secs;
	char disk_create_cmd[256];
	long file_num = 0;
	long RAM_SIZE_k = 0;
	long dummy_size_kB;	
	struct stat file_stat;
	char filename_str[256];
	int dev_null_fd;
		
	errfile_p = fopen(ERRFILE, "a") ;
	if (errfile_p == NULL)
	{
		exit(-1);
	}

	//logfile_fd = open(LOGFILE, O_APPEND|O_CREAT) ;
    logfile_fd = open(LOGFILE, O_WRONLY|O_CREAT, S_IRWXU);
	if (logfile_fd < 0)
	{
		fprintf(errfile_p, "Log file could not be opened\n");
		exit(-1);
	}

	if (argc%4 != 3 || argc == 3)
	{
#ifdef DEBUG_SS
		sprintf(logfile_buf, "Input: argc=%d\n", argc); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "Usage: %s <daemonize> <expt-duration> [[<thread-type> <rate|pick> <number> <name>]]\n", argv[0]); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "%s", "<expt-duration> should be integer\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "%s", "<thread-type> can be R, W, N, C\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "%s", "<rate> in Kbps for net with 1Mbps=1000Kbps and max=100Mbps & <rate> in perc for cpu, <pick>=0,1,2 for disk R\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=0 for disk R with re-created files\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=1 for disk R with second half of already existing files\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=2 for disk R with first half of already existing files\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
        fprintf(stdout, "%s", "<number> can be 'number of 4k blocks' for R/W,\
'number of 4kB' for N and xxxx for C\n");
		fprintf(stdout, "%s", "<name> can be file-prefix for R/W,\
web-server for N and xxxx for C\n");
#else
		fprintf(errfile_p, "Input: argc=%d\n", argc);
		fprintf(errfile_p, "Usage: %s <daemonize> <expt-duration> [[<thread-type> <rate|pick> <number> <name>]]\n", argv[0]);
		fprintf(errfile_p, "%s", "<expt-duration> should be integer\n");
		fprintf(errfile_p, "%s", "<thread-type> can be R, W, N, C\n");
		fprintf(errfile_p, "%s", "<rate> in Kbps for net with 1Mbps=1000Kbps and max=100Mbps & <rate> in perc for cpu, <pick>=0,1,2 for disk R\n");
		sprintf(logfile_buf, "<pick>=0 for disk R with re-created files\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=1 for disk R with second half of already existing files\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=2 for disk R with first half of already existing files\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
        fprintf(errfile_p, "%s", "<number> can be 'number of 4k blocks' for R/W,\
'number of 4kB' for N and xxxx for C\n");
		fprintf(errfile_p, "%s", "<name> can be file-prefix for R/W,\
web-server for N and xxxx for C\n");
#endif
		exit(-1);
	}

	daemonize_flag = atoi(argv[1]);
	dur_secs = atoi(argv[2]);
	sprintf(logfile_buf, "Duration = %d\n" , dur_secs); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
	if (dur_secs == 0)
    {
#ifdef DEBUG_SS
		sprintf(logfile_buf, "%s", "<expt-duration> should be integer\n\n\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "Usage: %s <daemonize> <expt-duration> [[<thread-type> <rate|pick> <number> <name>]]\n", argv[0]); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "%s", "<duration> should be integer\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
        sprintf(logfile_buf, "%s", "<thread-type> can be R, W, N, C - case-insensitive\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "%s", "<rate> in Kbps for net with 1Mbps=1000Kbps and max=100Mbps & <rate> in perc for cpu, <pick>=0,1,2 for disk R\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=0 for disk R with re-created files\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=1 for disk R with second half of already existing files\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=2 for disk R with first half of already existing files\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
        fprintf(stdout, "%s", "<number> can be 'number of 4k blocks' for R/W,\
'number of 4kB' for N and xxxx for C\n");
		fprintf(stdout, "%s", "<name> can be file-prefix for R/W,\
web-server for N and xxxx for C\n");
		sprintf(logfile_buf, "Nothing to do, zero load => exit\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#else
		fprintf(errfile_p, "%s", "<expt-duration> should be integer\n\n\n");
		fprintf(errfile_p, "Usage: %s <daemonize> <expt-duration> [[<thread-type> <rate|pick> <number> <name>]]\n", argv[0]);
		fprintf(errfile_p, "%s", "<duration> should be integer\n");
        fprintf(errfile_p, "%s", "<thread-type> can be R, W, N, C - case-insensitive\n");
		fprintf(errfile_p, "%s", "<rate> in Kbps for net with 1Mbps=1000Kbps and max=100Mbps & <rate> in perc for cpu, <pick>=0,1,2 for disk R\n");
		sprintf(logfile_buf, "<pick>=0 for disk R with re-created files\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=1 for disk R with second half of already existing files\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=2 for disk R with first half of already existing files\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
        fprintf(errfile_p, "%s", "<number> can be 'number of 4k blocks' for R/W,\
'number of 4kB' for N and xxxx for C\n");
		fprintf(errfile_p, "%s", "<name> can be file-prefix for R/W,\
web-server for N and xxxx for C\n");
		fprintf(errfile_p, "Nothing to do, zero load => exit\n");
#endif
        exit(-1);
    }


	point_to_array = (pthread_t*)malloc(argc*sizeof(pthread_t));
	point_to_arg_array = (struct generic_arg_t*)malloc(argc*sizeof(struct generic_arg_t));

	for (arg_index=3; arg_index<argc; arg_index+=4)
    {
		if (strlen(argv[arg_index]) != 1)
		{
#ifdef DEBUG_SS
			sprintf(logfile_buf, "%s", "<thread-type> can be R, W, N, C - case-insensitive\n\n\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
	        sprintf(logfile_buf, "%s", "<expt-duration> should be integer\n\n\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
			sprintf(logfile_buf, "Usage: %s <daemonize> <expt-duration> [[<thread-type> <rate|pick> <number> <name>]]\n", argv[0]); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
	        sprintf(logfile_buf, "%s", "<duration> should be integer\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
    	    sprintf(logfile_buf, "%s", "<thread-type> can be R, W, N, C - case-insensitive\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
			sprintf(logfile_buf, "%s", "<rate> in Kbps for net with 1Mbps=1000Kbps and max=100Mbps & <rate> in perc for cpu, <pick>=0,1,2 for disk R\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=0 for disk R with re-created files"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=1 for disk R with second half of already existing files"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=2 for disk R with first half of already existing files"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
        	fprintf(stdout, "%s", "<number> can be 'number of 4k blocks' for R/W,\
'number of 4kB' for N and xxxx for C\n");
			fprintf(stdout, "%s", "<name> can be file-prefix for R/W,\
webserver-IP for N and xxxx for C\n");
#else
			fprintf(errfile_p, "%s", "<thread-type> can be R, W, N, C - case-insensitive\n\n\n");
	        fprintf(errfile_p, "%s", "<expt-duration> should be integer\n\n\n");
			fprintf(errfile_p, "Usage: %s <daemonize> <expt-duration> [[<thread-type> <rate|pick> <number> <name>]]\n", argv[0]);
	        fprintf(errfile_p, "%s", "<duration> should be integer\n");
    	    fprintf(errfile_p, "%s", "<thread-type> can be R, W, N, C - case-insensitive\n");
			fprintf(errfile_p, "%s", "<rate> in Kbps for net with 1Mbps=1000Kbps and max=100Mbps & <rate> in perc for cpu, <pick>=0,1,2 for disk R\n");
		sprintf(logfile_buf, "<pick>=0 for disk R with re-created files"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=1 for disk R with second half of already existing files"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=2 for disk R with first half of already existing files"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
        	fprintf(errfile_p, "%s", "<number> can be 'number of 4k blocks' for R/W,\
'number of 4kB' for N and xxxx for C\n");
			fprintf(errfile_p, "%s", "<name> can be file-prefix for R/W,\
webserver-IP for N and xxxx for C\n");
#endif
        	exit(-1);
    	}
	}

	/* Processing the inputs for each load type. The actual loading should begin
     * for all threads parallely after this processing is done. Things to do -
     * 1. Make note of input parameters per load type, rate (R), number(N), name. R < 100 is a constraint or no?
     * 2a) For cpu load - fibonacci COUNT_FIBO_TERMS terms are considered. Read P from config file. Now, interval I = 100P(1-R)/R. However, we have a restriction that we can not sleep for less than 4ms. So, num_fibo=ceil(MIN_INTERVAL ms/P) and I=I*num_fibo
     * 2b) For net load - create file on webserver of S=Nx4K blocks using dd command (urandom). Interval I = S/R msec (note R in Kbps and I in msec)
     * 2c) For disk rd load - cat /proc/meminfo | grep MemTotal | awk '{print $2}' will give RAM in kB. create Y disk files of S=Nx4K blocks using dd command (urandom) where Y = 2 x RAM-size/4N. Before creation, check if already Y files of S size exist. Create only if NO. Read the file via command - dd if=/root/files/prefix_no of=/dev/zero bs=4k count=N
     * 2d) For disk wr load - cat /proc/meminfo | grep MemTotal | awk '{print $2}' will give RAM in kB. Write Y disk files of S=Nx4K blocks using dd command (/dev/zero) where Y = 2 x RAM-size/4N.
     * 3. Note that for cpu load, sleep interval begins after request finishes, i.e. sequentially done. So, if interval = I, then total time per request = P+I. But for net and disk load, sleep interval begins as soon as the request thread is started off. So, total time per request = interval.
     */


	for (arg_index=3; arg_index<argc; arg_index+=4)
	{

		switch(argv[arg_index][0])
		{
			case 'r': 
			case 'R': sprintf(ARG(thread_type), "disk_rd");
					    /* <rate> for disk load was dont-care, post removal of intervals */
						/* Now, replaced by <pick> = 0 for re-create files and then read from 1st, 1 for reading up the file index for existing files and start reading from next file */
						DISK_RD(pick) = atoi(argv[arg_index+1]);
						DISK_RD(num_4k) = atol(argv[arg_index+2]);
						strcpy(DISK_RD(prefix), argv[arg_index+3]);

						/* Reading the RAM size from /proc/meminfo */
						RAM_SIZE_k = read_ram_size_in_k_from_file();

						/* Computing number of files of size num_4k needed to cover RAM twice */
						DISK_RD(twice_ram_num) = (2 * RAM_SIZE_k) / (DISK_RD(num_4k) * 4);
						MAKE_INDEX_FILENAME_STR(DISK_RD(file_index_filename), DISK_RD(prefix)); 

#ifdef DEBUG_SS
                        sprintf(logfile_buf, "twice_ram_num of files = %ld\n", DISK_RD(twice_ram_num)); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

						break;
			case 'w':
			case 'W': sprintf(ARG(thread_type), "disk_wr");
					    /* <rate> for disk load is dont-care, post removal of intervals */
						DISK_WR(num_4k) = atol(argv[arg_index+2]);
						if (DISK_WR(num_4k) == 0)
						{
#ifdef DEBUG_SS
							sprintf(logfile_buf, "Count = 0, implies no Disk Write load to be generated\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
						}
						strcpy(DISK_WR(prefix), argv[arg_index+3]);

                        /* Reading the RAM size from /proc/meminfo */
                        RAM_SIZE_k = read_ram_size_in_k_from_file();

						if (DISK_WR(num_4k) != 0)
						{
                        	/* Computing number of files of size num_4k needed to cover RAM twice */
	                        DISK_WR(twice_ram_num) = (2 * RAM_SIZE_k) / (DISK_WR(num_4k) * 4);

#ifdef DEBUG_SS
	                        sprintf(logfile_buf, "twice_ram_num of files = %ld\n", DISK_WR(twice_ram_num)); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
						}
						else
						{
							DISK_WR(twice_ram_num) = -1;
#ifdef DEBUG_SS
                            sprintf(logfile_buf, "twice_ram_num of files = -1 since num_4k = 0 \n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
						}
                        break;
			case 'n':
			case 'N': sprintf(ARG(thread_type), "net");
						NET(kbps) = atol(argv[arg_index+1]);		
					    if (NET(kbps) == 0)
						{
							/* This VM only has to wait to Rx wget-requests from other and service them.
							 * So, this thread has no wget Tx to do, break out and dont create thread later */
							fprintf(errfile_p, "NET rate = 0\n");
#ifdef DEBUG_SS
                            sprintf(logfile_buf, "NET rate = 0\n", DISK_WR(twice_ram_num)); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
							break;
						}
						NET(num_4k) = atol(argv[arg_index+2]);
						strcpy(NET(webserver), argv[arg_index+3]);

						NET(batch) = 1;
						NET(interval) = (NET(num_4k) * 4096 * 8)/NET(kbps);		/* Since Rate is in Kbps, Interval is in msec */
						if (NET(interval) <= MIN_INTERVAL)
						{	
#ifdef DEBUG_SS
				            sprintf(logfile_buf, "Interval = %ld is not greater than %d\n", (NET(interval)), MIN_INTERVAL); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
							NET(batch) = ceil(MIN_INTERVAL/NET(interval));
							NET(interval) = NET(interval) * NET(batch);
						}
#ifdef DEBUG_SS
	                    sprintf(logfile_buf, "Interval = %ld (should be greater than %d)\n", (NET(interval)), MIN_INTERVAL); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

                        break;
			case 'c':
			case 'C': sprintf(ARG(thread_type), "cpu");
						CPU(perc) = atol(argv[arg_index+1]);		
						CPU(interval) = (long)((100 - CPU(perc)) * TIME_FOR_2000000_FIBO_TERMS);
						CPU(batch) = CPU(perc);

                        break;
			default: fprintf(errfile_p, "%s: Unknown thread-type %d at %d", argv[0], argv[arg_index][0], arg_index);
						exit(-1);
		}

		num_threads++;
	}

	num_threads = 0;
	num_threads_started = 0;

	if (daemonize_flag == 1)
	{
#ifdef DEBUG_SS
        sprintf(logfile_buf, "About to generate_loads\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

        daemonize();
#ifdef DEBUG_SS
        sprintf(logfile_buf, "Daemonizing generate_loads\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
        close(1);
		dup_fd = dup2(logfile_fd, STDOUT_FD);
		close(logfile_fd);		/* Closing the to-be-unused descriptor after use */

        if (dup_fd < 0)
        {
#ifdef DEBUG_SS
            sprintf(logfile_buf, "dup2 failed\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
            fprintf(errfile_p, "dup2 failed\n");
            exit(-1);
        }
#ifdef DEBUG_SS
        sprintf(logfile_buf, "dup2 success %d\n", dup_fd); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif		
    }
    else
    {
        sprintf(logfile_buf, "Not daemonizing since daemonize_flag != 1\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
        fprintf(errfile_p, "Not daemonizing since daemonize_flag != 1\n");
    }

	for (arg_index=3; arg_index<argc; arg_index+=4)
	{
		/* For net load, if rate=0, just need to serve wget requests from other VM, no request generation */
		if (strcmp(ARG(thread_type), "net") == 0 && (NET(kbps) == 0))
		{
#ifdef DEBUG_SS
            sprintf(logfile_buf, "load is net with rate = 0, so nothing to do\n", ARG(thread_type)); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
			//nothing to do
		}
		else if (strcmp(ARG(thread_type), "disk_rd") == 0 && DISK_RD(num_4k) == 0)
		{
#ifdef DEBUG_SS
            sprintf(logfile_buf, "load is disk_rd with size = 0, so nothing to do\n", ARG(thread_type)); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
			//nothing to do
		}
		else if (strcmp(ARG(thread_type), "disk_wr") == 0 && DISK_WR(num_4k) == 0)
		{
#ifdef DEBUG_SS
            sprintf(logfile_buf, "load is disk_wr with size = 0, so nothing to do\n", ARG(thread_type)); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
			//nothing to do
		}
		else
		{
					  
#ifdef DEBUG_SS
            sprintf(logfile_buf, "proceeding to make thread of type %s\n", ARG(thread_type)); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
			/* num_threads holds the total number of threads created till now */
			*(point_to_array + num_threads) = make_thread((point_to_arg_array + num_threads), arg_index);
#ifdef DEBUG_SS
            sprintf(logfile_buf, "made thread of type %s\n", ARG(thread_type)); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
			num_threads_started++;
			
		}
		num_threads++;
	}

	if (num_threads_started == 0)
	{
		//Nothing to do, just exit
		exit(0);
	}


	/* Activating alarm to end threads when "duration" ends */
	signal(SIGALRM, dur_alarm_catcher);
	alarm(dur_secs);

	fprintf(errfile_p, "Joining on child threads\n");

	/* To ensure that main thread does not exit early */	
	for (index=0; index<num_threads; index++)
    {
		 pthread_join(*(point_to_array + index), NULL);
	}

	fprintf(errfile_p, "If all goes well, should never reach here!\n");
	/*
	 * The pthread_exit() in the alarm handler will kill entire process, so this
     * may never get executed during a run
	*/

	exit(0);


//	pthread_exit(NULL);
}

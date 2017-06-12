 /* NEEDs root permission, inside VM.
 * Needs write permission in /root/files/ 
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



#define SSH_NET_CREATE() "ssh root@%s 'dd if=/dev/urandom of=/var/www/Doc%ld.txt bs=4k count=%ld 2> /dev/null'"
#define DISK_CREATE() "dd if=/dev/urandom of=/root/files/%s%ld bs=4k count=%ld conv=fdatasync"
#define NETLOAD() "wget -O netfile%ld.txt http://%s/Doc%ld.txt 2> /dev/null"
#define MAKE_NETFILE_STR(str, x) do { sprintf(str, "netfile%ld.txt", x); } while(0);
#define MAKE_HTTP_STR(str, x, y) do { sprintf(str, "http://%s/Doc%ld.txt", x, y); } while(0);
#define WLOAD() "dd if=/dev/zero of=/root/files/%s%ld bs=4k count=%ld conv=fdatasync"
#define RLOAD() "dd if=/root/files/%s%ld of=/dev/null bs=4k count=%ld 2> /dev/null"
#define MAKE_OF_STR(str, x, y)	do { sprintf(str, "of=/root/files/%s%ld", x, y); } while(0);
#define MAKE_IF_STR(str, x, y)	do { sprintf(str, "if=/root/files/%s%ld", x, y); } while(0);
#define MAKE_COUNT_STR(str, x)	do { sprintf(str, "count=%ld", x); } while(0);
#define MAKE_INDEX_FILENAME_STR(str, x) do { sprintf(str, "%s_fileindex.sss.txt", x); } while(0);
#define ARG(x)	(point_to_arg_array + num_threads)->x
#define CPU(x)  (point_to_arg_array + num_threads)->union_arg.cpu_arg.x
#define NET(x)  (point_to_arg_array + num_threads)->union_arg.net_arg.x
#define DISK_RD(x)  (point_to_arg_array + num_threads)->union_arg.disk_rd_arg.x
#define DISK_WR(x)  (point_to_arg_array + num_threads)->union_arg.disk_wr_arg.x

#define COUNT_FIBO_TERMS 1000
#define ERRFILE "prep_for_generate_loads.err.sss.txt"
#define LOGFILE "prep_for_generate_loads.log.sss.txt"
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

long num_threads=0;
pthread_t *point_to_array;
struct generic_arg_t *point_to_arg_array;

char netload_cmd[256];

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
	char file_index_filename[256];          /* To record file index, first check for existence, if no, then index = 1, else read value + 1. If index > count, index = 1 */
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
    long batch;					/* Used if interval calculated as < MIN_INTERVAL msec. "batch" is the numbers of requests to run in a single thread before sleeping for interval */
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
    long batch;
	char load_type[256];
};

long fibonacci(int n)
{
	long a = 0;
	long b = 1;
	long sum = 0;
	int i = 0;
	long bigger_sum = 0;

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



void generate_cpu_load(void *arg1)
{
	long interval=0, batch=0;
	struct generic_arg_t *ptr;

	ptr = (struct generic_arg_t*)arg1;
	interval = ptr->union_arg.cpu_arg.interval;
	batch = ptr->union_arg.cpu_arg.batch;
	long result = 0;
	while(1)
	{
#ifdef DEBUG_SS
		sprintf(logfile_buf, "Generate cpu load\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		result = 1;
#else
		result = fibonacci(COUNT_FIBO_TERMS);
#endif
		result!=0? usleep(interval*1000):fprintf(errfile_p, "generate_loads: fibonacci result=0\n");
	}
	pthread_exit(NULL);
}

void empty_prefix_files_in_root_directory(char *prefix)
{
	char cmd[256];
	
	sprintf(cmd, "rm -rf /root/files/%s*", prefix);
	system(cmd);
#ifdef DEBUG_SS
	sprintf(logfile_buf, "Emptied /root/files/%s*\n", prefix); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
}

void empty_w_files_in_root_directory()
{
	char cmd[256];
	
	sprintf(cmd, "rm -rf /root/files/w*");
	system(cmd);
#ifdef DEBUG_SS
	sprintf(logfile_buf, "Emptied /root/files/w*\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
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


long read_ram_size_in_k_from_proc()
{
	char cmd[256];
	FILE *fp;
	long RAMSIZE_k = 0;

	/* Reading the machine's RAM size into /root/RAMSIZE_k */
	sprintf(cmd, "cat /proc/meminfo | grep MemTotal | awk '{print $2}' > /root/RAMSIZE_k");
	system(cmd);
	
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

void verify_file_size_in_kB(long size_kB, char *prefix, long count, int pick)
{
	struct stat file_stat;
	long index, i, filesize_kB;
	char filename_str[256];
	long start;

	if (pick == 1)
	{
		/* Load is to read 2nd half of files, so verification should begin from 2nd half and then wrap around */
		start = count/2 + 1;
	}
	else
	{
		/* Load is to read 1st half of files, so verification should begin from 1st half */
		start = 1;
	}

	for (index=start, i=1; i<=count; index++, i++)
	{
		if (index > count)
		{
			index = 1;
		}
		sprintf(filename_str, "/root/files/%s%ld", prefix, index);
#ifdef DEBUG_SS
        sprintf(logfile_buf, "Verifying file %s\n", filename_str); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
        if (!stat(filename_str, &file_stat))
        {
        	filesize_kB = file_stat.st_size/1024;
        }
        else
        {
        	fprintf(errfile_p, "Not able to get stats about file during verification. File doesnt exist? Use option 0 to create anew %s\n", filename_str);
#ifdef DEBUG_SS
		    sprintf(logfile_buf, "Not able to get stats about file during verification. File doesnt exist? Use option 0 to create anew %s\n", filename_str); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

            exit(-1);
        }
		if (filesize_kB != size_kB)
		{
			fprintf(errfile_p, "File exists, but not of required size %ld. Use option 0 to create anew\n", size_kB);
#ifdef DEBUG_SS
            sprintf(logfile_buf, "File exists, but not of required size %ld. Use option 0 to create anew\n", size_kB); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
			exit(-1);
		}
	}
}

int main(int argc, char *argv[])
{
	int arg_index=0;
	int index=0;
	int daemonize_flag = 0;
	int dur_secs;
	char disk_create_cmd[256];
	long file_num = 1;
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
		fprintf(errfile_p, "%s: Log file could not be opened\n", argv[0]);
		exit(-1);
	}

	if (argc%4 != 3 || argc == 3)
	{
#ifdef DEBUG_SS
		sprintf(logfile_buf, "Input: argc=%d\n", argc); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "Usage: %s <dont-care> <dont-care> [[<thread-type> <rate|pick> <number> <name>]]\n", argv[0]); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "%s", "<thread-type> can be R, W, N, C\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "%s", "<rate> in Kbps for net with 1Mbps=1000Kbps and max=100Mbps & <rate> in perc for cpu, <pick>=0,1,2 for disk R\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=0 for disk R with re-created files\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=1 for disk R on 2nd half\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=2 for disk R on 1st half\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
        fprintf(stdout, "%s", "<number> can be 'number of 4k blocks' for R/W,\
'number of 4kB' for N and xxxx for C\n");
		fprintf(stdout, "%s", "<name> can be file-prefix for R/W,\
web-server for N and xxxx for C\n");
#else
		fprintf(errfile_p, "Input: argc=%d\n", argc);
		fprintf(errfile_p, "Usage: %s <dont-care> <dont-care> [[<thread-type> <rate|pick> <number> <name>]]\n", argv[0]);
		fprintf(errfile_p, "%s", "<thread-type> can be R, W, N, C\n");
		fprintf(errfile_p, "%s", "<rate> in Kbps for net with 1Mbps=1000Kbps and max=100Mbps & <rate> in perc for cpu, <pick>=0,1,2 for disk R\n");
		sprintf(logfile_buf, "<pick>=0 for disk R with re-created files\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=1 for disk R on 2nd half\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=2 for disk R on 1st half\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
        fprintf(errfile_p, "%s", "<number> can be 'number of 4k blocks' for R/W,\
'number of 4kB' for N and xxxx for C\n");
		fprintf(errfile_p, "%s", "<name> can be file-prefix for R/W,\
web-server for N and xxxx for C\n");
#endif
		exit(-1);
	}

	daemonize_flag = atoi(argv[1]);
	dur_secs = atoi(argv[2]);

	/* Validation check for duration is removed, coz its dont-care during prep */

	point_to_array = (pthread_t*)malloc(argc*sizeof(pthread_t));
	point_to_arg_array = (struct generic_arg_t*)malloc(argc*sizeof(struct generic_arg_t));

	for (arg_index=3; arg_index<argc; arg_index+=4)
    {
		if (strlen(argv[arg_index]) != 1)
		{
#ifdef DEBUG_SS
			sprintf(logfile_buf, "%s", "<thread-type> can be R, W, N, C - case-insensitive\n\n\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
			sprintf(logfile_buf, "Usage: %s <dont-care> <dont-care> [[<thread-type> <rate|pick> <number> <name>]]\n", argv[0]); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
	        sprintf(logfile_buf, "%s", "<duration> should be integer\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
    	    sprintf(logfile_buf, "%s", "<thread-type> can be R, W, N, C - case-insensitive\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
			sprintf(logfile_buf, "%s", "<rate> in Kbps for net with 1Mbps=1000Kbps and max=100Mbps & <rate> in perc for cpu, <pick>=0,1,2 for disk R\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=0 for disk R with re-created files\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=1 for disk R on 2nd half\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=2 for disk R on 1st half\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
        	fprintf(stdout, "%s", "<number> can be 'number of 4k blocks' for R/W,\
'number of 4kB' for N and xxxx for C\n");
			fprintf(stdout, "%s", "<name> can be file-prefix for R/W,\
webserver-IP for N and xxxx for C\n");
#else
			fprintf(errfile_p, "%s", "<thread-type> can be R, W, N, C - case-insensitive\n\n\n");
			fprintf(errfile_p, "Usage: %s <dont-care> <dont-care> [[<thread-type> <rate|pick> <number> <name>]]\n", argv[0]);
	        fprintf(errfile_p, "%s", "<duration> should be integer\n");
    	    fprintf(errfile_p, "%s", "<thread-type> can be R, W, N, C - case-insensitive\n");
			fprintf(errfile_p, "%s", "<rate> in Kbps for net with 1Mbps=1000Kbps and max=100Mbps & <rate> in perc for cpu, <pick>=0,1,2 for disk R\n");
		sprintf(logfile_buf, "<pick>=0 for disk R with re-created files\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=1 for disk R on 2nd half\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		sprintf(logfile_buf, "<pick>=2 for disk R on 1st half\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
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
						/* Now, replaced by <pick> = 0 for re-create files and, 1 for read existing files */
						DISK_RD(pick) = atoi(argv[arg_index+1]);
						DISK_RD(num_4k) = atol(argv[arg_index+2]);
						strcpy(DISK_RD(prefix), argv[arg_index+3]);

						/* Reading the RAM size from /proc/meminfo */
						RAM_SIZE_k = read_ram_size_in_k_from_proc();

						/* Computing number of files of size num_4k needed to cover RAM twice */
						DISK_RD(twice_ram_num) = (2 * RAM_SIZE_k) / (DISK_RD(num_4k) * 4);
						MAKE_INDEX_FILENAME_STR(DISK_RD(file_index_filename), DISK_RD(prefix));

#ifdef DEBUG_SS
                        sprintf(logfile_buf, "twice_ram_num of files = %ld\n", DISK_RD(twice_ram_num)); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

/* The following patch of code for duping /dev/null to stderr, prevents execl from spewing output on screen */
    dev_null_fd = open("/dev/null", O_WRONLY);
    if (dev_null_fd < 0)
    {
        fprintf(errfile_p, "/dev/null can not be opened for O_WRONLY\n");
        exit(-1);
    }

    close(2);
    dup2(dev_null_fd, 2);
	close(dev_null_fd);     /* Freeing the to-be-unused descriptor */
/* The above patch of code for duping /dev/null to stderr, prevents system() from spewing output on screen */

						
						/* For disk read load, first need to create the base files that will be read if pick = 0 but after deleting the files of same number and diff size that may be existing */
						if (DISK_RD(pick) == 0) 
						{
							empty_prefix_files_in_root_directory(DISK_RD(prefix));

							file_num = 1;
					    	for (file_num = 1; file_num <= DISK_RD(twice_ram_num); file_num++)
	    					{
					        	sprintf(disk_create_cmd, DISK_CREATE(), DISK_RD(prefix), file_num, DISK_RD(num_4k));
								system(disk_create_cmd);
								usleep(1000);	//just a token sleep so that machine doesnt become completly unresponsive..?
							}
							write_integer_into_file(DISK_RD(file_index_filename), 0);
						}
						else if (DISK_RD(pick) == 1 || DISK_RD(pick) == 2 )
						{
							/* Since we are not creating the files, they better be already created */
							/* First, sanity check that the input prefix is not "write.." something. */
							if (DISK_RD(prefix)[0] == 'w' || DISK_RD(prefix)[0] == 'W')
							{
								fprintf(errfile_p, "Use a read-like file prefix for Disk R, not %s\n", DISK_RD(prefix));
#ifdef DEBUG_SS
                                sprintf(logfile_buf, "Use a read-like file prefix for Disk R, not %s\n", DISK_RD(prefix)); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
								exit(-1);
							}
						
							long value = read_integer_from_file(DISK_RD(file_index_filename));
							if (value > DISK_RD(twice_ram_num))
							{
								fprintf(errfile_p, "File index %ld > number of files %ld. How is this possible?\n", value,  DISK_RD(twice_ram_num)); 
#ifdef DEBUG_SS
								sprintf(logfile_buf, "File index %ld > number of files %ld. How is this possible?\n", value,  DISK_RD(twice_ram_num)); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
								exit(-1);
							}


							/* To check, first create a dummy file of given size, then do "du -sh" on that file and awk print $1 to get its created/reported size. Now, there should be DISK_RD(twice_ram_num) number of files with given prefix and observed size. If not, flag error and exit */
							sprintf(disk_create_cmd, DISK_CREATE(), "dummy_read", (long)1, DISK_RD(num_4k));
							system(disk_create_cmd);
							sprintf(filename_str, "/root/files/dummy_read1");
							if (!stat(filename_str, &file_stat))
							{
								dummy_size_kB = file_stat.st_size/1024;
							}
							else
							{
								fprintf(errfile_p, "Not able to get stats about file %s\n", filename_str);
#ifdef DEBUG_SS
	                            sprintf(logfile_buf, "Not able to get stats about file %s\n", filename_str); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
								exit(-1);
							}
							verify_file_size_in_kB(dummy_size_kB, DISK_RD(prefix), DISK_RD(twice_ram_num), DISK_RD(pick));
						}
						else
						{
							fprintf(errfile_p, "Invalid pick input for DISK READ\n");
#ifdef DEBUG_SS
	                        sprintf(logfile_buf, "Invalid pick input for DISK READ\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
						}

						break;
			case 'w':
			case 'W': sprintf(ARG(thread_type), "disk_wr");
					    /* <rate> for disk load is dont-care, post removal of intervals */
						DISK_WR(num_4k) = atol(argv[arg_index+2]);
						strcpy(DISK_WR(prefix), argv[arg_index+3]);

                        /* Reading the RAM size from /proc/meminfo */
                        RAM_SIZE_k = read_ram_size_in_k_from_proc();

                        /* Computing number of files of size num_4k needed to cover RAM twice */
                        DISK_WR(twice_ram_num) = (2 * RAM_SIZE_k) / (DISK_WR(num_4k) * 4);

#ifdef DEBUG_SS
                        sprintf(logfile_buf, "twice_ram_num of files = %ld\n", DISK_WR(twice_ram_num)); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

						empty_w_files_in_root_directory();
                        break;
			case 'n':
			case 'N': sprintf(ARG(thread_type), "net");
						NET(kbps) = atol(argv[arg_index+1]);		
					    if (NET(kbps) == 0)
						{
							/* This VM only has to wait to Rx wget-requests from other and service them.
							 * So, this thread has no wget Tx to do, so no prep needed */
							fprintf(errfile_p, "NET rate = 0\n");
#ifdef DEBUG_SS
      	                    sprintf(logfile_buf, "NET rate = 0\n", DISK_WR(twice_ram_num)); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
							break;
						}
						NET(num_4k) = atol(argv[arg_index+2]);
						strcpy(NET(webserver), argv[arg_index+3]);

						/* For net, create Doc.html on the other VM of 'num_4k' 4k blocks */
						sprintf(netload_cmd, SSH_NET_CREATE(), NET(webserver), NET(num_4k), NET(num_4k));
#ifdef DEBUG_SS
						sprintf(logfile_buf, "%s\n", netload_cmd); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
						system(netload_cmd);

                        break;
			case 'c':
			case 'C': sprintf(ARG(thread_type), "cpu");
						CPU(perc) = atol(argv[arg_index+1]);		
						CPU(batch) = 1;
						/* incomplete */

                        break;
			default: fprintf(errfile_p, "%s: Unknown thread-type %d at %d", argv[0], argv[arg_index][0], arg_index);
						exit(-1);
		}

		num_threads++;
	}

	free(point_to_array);
	free(point_to_arg_array);
	close(logfile_fd);

	exit(0);
}

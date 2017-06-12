/**********************************************************************
 * * file:   report_traff_per_interval.c
 * * date:   Tue May 18 16:06:33 IST 2010  
 * * Author: Sujesha Sudevalayam 
 * * Last Modified:
 * * 
 * * Categorize and print the count of packets (also bytes) that
 * * are received from/transmitted to a particular host (affine Rx/tx) 
 * * and everybody else (non-affine Rx/Tx). 
 * *
 * * Adapted from testpcap3.c and disect2.c, written by Martin Casado
 * * Thanks due for the wonderful tutorial at http://yuba.stanford.edu/~casado/pcap/section1.html
 * *
 * **********************************************************************/
/**********************************************************************
 * * file:   testpcap3.c
 * * date:   Sat Apr 07 23:23:02 PDT 2001  
 * * Author: Martin Casado
 * * Last Modified:2001-Apr-07 11:23:05 PM
 * *
 * * Investigate using filter programs with pcap_compile() and
 * * pcap_setfilter()
 * *
 * **********************************************************************/

/**********************************************************************
 * * file:   disect2.c
 * * date:   Tue Jun 19 20:07:49 PDT 2001  
 * * Author: Martin Casado
 * * Last Modified:2001-Jun-24 10:05:35 PM
 * *
 * * Description: 
 * *
 * *   Large amounts of this code were taken from tcpdump source
 * *   namely the following files..
 * *
 * *   print-ether.c
 * *   print-ip.c
 * *   ip.h
 * *
 * * Compile with:
 * * gcc -Wall -pedantic disect2.c -lpcap (-o foo_err_something) 
 * *
 * * Usage:
 * * a.out (# of packets) "filter string"
 * *
 * **********************************************************************/



#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h> 

#include <net/ethernet.h>
#include <netinet/ether.h> 
#include <netinet/ip.h> 

#include <string.h>
#include <signal.h>
#include <time.h>

#include <unistd.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>



/* tcpdump header (ether.h) defines ETHER_HDRLEN) */
#ifndef ETHER_HDRLEN 
#define ETHER_HDRLEN 14
#endif

#define ERRFILE "report_traff_per_interval.err.sss.txt"
#define LOGFILE "report_traff_per_interval.log.sss.txt"
#define STDOUT_FD 1

//char errfile[256];
FILE *errfile_p = NULL;
int logfile_fd;
int dup_fd;
char logfile_buf[256];

/* Self Node */
char self_host[256];
/* Affine Node */
char to_host[256];

struct	in_addr to_addr;	/* source or dest address */
struct	in_addr self_addr;	/* source or dest address */

char pkt_src_str[256];
char pkt_dst_str[256];

char self_addr_str[256];
char affine_addr_str[256];

/* Maintaining various counts for reporting periodically */
unsigned int aff_rxbytes_count = 0;
unsigned int aff_txbytes_count = 0;
unsigned int aff_rxpkt_count = 0;
unsigned int aff_txpkt_count = 0;
unsigned int nonaff_rxbytes_count = 0;
unsigned int nonaff_txbytes_count = 0;
unsigned int nonaff_rxpkt_count = 0;
unsigned int nonaff_txpkt_count = 0;

unsigned int ip_bytes_count = 0;

unsigned int arp_count = 0;
unsigned int arp_bytes_count = 0;
unsigned int rarp_count = 0;
unsigned int rarp_bytes_count = 0;

unsigned int misc_count = 0;
unsigned int misc_bytes_count = 0;

/* Interval for logging all statistics */
unsigned int logging_interval = 1;

/* File for logging statistics */
char outfile[256];
FILE *outfile_p = NULL;


u_int16_t handle_ethernet
        (u_char *args,const struct pcap_pkthdr* pkthdr,const u_char*
        packet);
u_char* handle_IP
        (u_char *args,const struct pcap_pkthdr* pkthdr,const u_char*
        packet);


/*
 *  * Structure of an internet header, naked of options.
 *
 *  * Stolen from tcpdump source (thanks tcpdump people)
 *  *
 *  * We declare ip_len and ip_off to be short, rather than u_short
 *  * pragmatically since otherwise unsigned comparisons can result
 *  * against negative integers quite easily, and fail in subtle ways.
 *  */
struct my_ip {
	u_int8_t	ip_vhl;		/* header length, version */
#define IP_V(ip)	(((ip)->ip_vhl & 0xf0) >> 4)
#define IP_HL(ip)	((ip)->ip_vhl & 0x0f)
	u_int8_t	ip_tos;		/* type of service */
	u_int16_t	ip_len;		/* total length */
	u_int16_t	ip_id;		/* identification */
	u_int16_t	ip_off;		/* fragment offset field */
#define	IP_DF 0x4000			/* dont fragment flag */
#define	IP_MF 0x2000			/* more fragments flag */
#define	IP_OFFMASK 0x1fff		/* mask for fragmenting bits */
	u_int8_t	ip_ttl;		/* time to live */
	u_int8_t	ip_p;		/* protocol */
	u_int16_t	ip_sum;		/* checksum */
	struct	in_addr ip_src,ip_dst;	/* source and dest address */
};

/* Count the packet, and the number of bytes for each packet */
void my_callback(u_char *args,const struct pcap_pkthdr* pkthdr,const u_char*
        packet)
{
	u_int16_t type = handle_ethernet(args,pkthdr,packet);

    if(type == ETHERTYPE_IP)
    {/* handle IP packet */
        handle_IP(args,pkthdr,packet);
    }
	else if(type == ETHERTYPE_ARP)
    {/* handle arp packet */
		arp_count++;
    }
    else if(type == ETHERTYPE_REVARP)
    {/* handle reverse arp packet */
		rarp_count++;
    }
	else
	{
		misc_count++;
	}

}


/* handle ethernet packets, much of this code gleaned from
 * print-ether.c from tcpdump source
 */
u_int16_t handle_ethernet
        (u_char *args,const struct pcap_pkthdr* pkthdr,const u_char*
        packet)
{
    u_int caplen = pkthdr->caplen;
    u_int length = pkthdr->len;
    struct ether_header *eptr;  /* net/ethernet.h */
    u_short ether_type;

    if (caplen < ETHER_HDRLEN)
    {
        fprintf(errfile_p,"Packet length less than ethernet header length\n");
        return -1;
    }

    /* lets start with the ether header... */
    eptr = (struct ether_header *) packet;
    ether_type = ntohs(eptr->ether_type);

    /* Lets print SOURCE DEST TYPE LENGTH */
	/* SSS - dont want to print any excess trimmings 
    sprintf(logfile_buf,"ETH: "); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
    fprintf(stdout,"%s "
            ,ether_ntoa((struct ether_addr*)eptr->ether_shost));
    fprintf(stdout,"%s "
            ,ether_ntoa((struct ether_addr*)eptr->ether_dhost));
	*/

    /* check to see if we have an ip packet */
	/* SSS - dont want to print any excess trimmings 
    if (ether_type == ETHERTYPE_IP)
    {
        fprintf(outfile_p,"(IP)");
    }else  if (ether_type == ETHERTYPE_ARP)
    {
        fprintf(outfile_p,"(ARP)");
    }else  if (eptr->ether_type == ETHERTYPE_REVARP)
    {
        fprintf(outfile_p,"(RARP)");
    }else {
        fprintf(outfile_p,"(?)");
    }
    fprintf(outfile_p," %d\n",length);
	*/

    if (ether_type == ETHERTYPE_IP)
    {
        ip_bytes_count += length;
    }
	else  if (ether_type == ETHERTYPE_ARP)
    {
        arp_bytes_count += length;
    }
	else  if (eptr->ether_type == ETHERTYPE_REVARP)
    {
        rarp_bytes_count += length;
    }
	else 
	{
		misc_bytes_count += length;
    }
    return ether_type;
}


u_char* handle_IP
        (u_char *args,const struct pcap_pkthdr* pkthdr,const u_char*
        packet)
{
    const struct my_ip* ip;
    u_int length = pkthdr->len;
    u_int hlen,off,version;
    int i;

    u_int16_t len;

    /* jump pass the ethernet header */
    ip = (struct my_ip*)(packet + sizeof(struct ether_header));
    length -= sizeof(struct ether_header); 

    /* check to see we have a packet of valid length */
    if (length < sizeof(struct my_ip))
    {
        printf("truncated ip %d",length);
        return NULL;
    }

    len     = ntohs(ip->ip_len);
    hlen    = IP_HL(ip); /* header length */
    version = IP_V(ip);/* ip version */

    /* check version */
    if(version != 4)
    {
      fprintf(outfile_p,"Unknown version %d\n",version);
      return NULL;
    }

    /* check header length */
    if(hlen < 5 )
    {
        fprintf(outfile_p,"bad-hlen %d \n",hlen);
    }

    /* see if we have as much packet as we should */
    if(length < len)
        printf("\ntruncated IP - %d bytes missing\n",len - length);

    /* Check to see if we have the first fragment */
    off = ntohs(ip->ip_off);
    if((off & 0x1fff) == 0 )/* aka no 1's in first 13 bits */
    {/* print SOURCE DESTINATION hlen version len offset */
/*
 *      fprintf(outfile_p,"IP: ");
        fprintf(outfile_p,"%s ",
                inet_ntoa(ip->ip_src));
        fprintf(outfile_p,"%s %d %d %d %d\n",
                inet_ntoa(ip->ip_dst),
                hlen,version,len,off);
*/

		sprintf(pkt_src_str, "%s", inet_ntoa(ip->ip_src));
		sprintf(pkt_dst_str, "%s", inet_ntoa(ip->ip_dst));

		/* Classifying packet as Rx/Tx and affine/non-affine */
		if (strcmp(self_addr_str, pkt_src_str) == 0)
		{
			/* I am transmitting */
			if (strcmp(affine_addr_str, pkt_dst_str) == 0)
			{
				/* Affine Tx identified */
				aff_txpkt_count++;
				aff_txbytes_count+=len;
			}
			else
			{
				/* Non-affine Tx identified */
				nonaff_txpkt_count++;
				nonaff_txbytes_count+=len;
			}
		}
		else if (strcmp(self_addr_str, pkt_dst_str) == 0)
		{
			/* I am receiving */
			if (strcmp(affine_addr_str, pkt_src_str) == 0)
			{
				/* Affine Rx identified */
				aff_rxpkt_count++;
				aff_rxbytes_count+=len;
			}
			else
			{
				/* Non-affine Rx identified */
                nonaff_rxpkt_count++;
                nonaff_rxbytes_count+=len;
            }
        }
		else
		{
			fprintf(errfile_p, "Error: External traffic caught by our filter?");
			exit(-1);
		}
		
    }
	return NULL;
}

void intr_catcher() 
{
//	fprintf(, "Caught Ctrl-C, exiting...\n");
	fclose(outfile_p);
	exit(0);
}

void alarm_catcher() 
{
	/* Make a note of the statistics */
	unsigned int curr_aff_rxbytes_count = aff_rxbytes_count;
	unsigned int curr_aff_txbytes_count = aff_txbytes_count;
	unsigned int curr_aff_rxpkt_count = aff_rxpkt_count;
	unsigned int curr_aff_txpkt_count = aff_txpkt_count;
	unsigned int curr_nonaff_rxbytes_count = nonaff_rxbytes_count;
	unsigned int curr_nonaff_txbytes_count = nonaff_txbytes_count;
	unsigned int curr_nonaff_rxpkt_count = nonaff_rxpkt_count;
	unsigned int curr_nonaff_txpkt_count = nonaff_txpkt_count;

	unsigned int curr_ip_bytes_count = ip_bytes_count;

	unsigned int curr_arp_count = arp_count;
	unsigned int curr_arp_bytes_count = arp_bytes_count;
	unsigned int curr_rarp_count = rarp_count;
	unsigned int curr_rarp_bytes_count = rarp_bytes_count;

	unsigned int curr_misc_count = misc_count;
	unsigned int curr_misc_bytes_count = misc_bytes_count;


    time_t     now;
    struct tm  *ts;
	char       buf[80];

    /* Get the current time */
    now = time(NULL);

    /* Format and print the time, "ddd yyyy-mm-dd hh:mm:ss zzz" */
    ts = localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ts);
	
	/* Print the timestamp and statistics on outfile_p */
	fprintf(outfile_p, "%s\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\n", buf,
					curr_aff_rxbytes_count, curr_aff_txbytes_count, curr_aff_rxpkt_count, curr_aff_txpkt_count,
					curr_nonaff_rxbytes_count, curr_nonaff_txbytes_count, curr_nonaff_rxpkt_count, curr_nonaff_txpkt_count,
					curr_ip_bytes_count, curr_arp_count, curr_arp_bytes_count, curr_rarp_count, curr_rarp_bytes_count,
					curr_misc_count, curr_misc_bytes_count);

	fflush(outfile_p);
	/* Set up the next alarm occurence before doing any further processing */
	alarm(logging_interval);

}

int daemonize()
{
	pid_t   pid;

        if ( (pid = fork()) < 0)
	{
                return(-1);
	}
        else if (pid != 0)
    	{
            printf("%d\n", pid);
            exit(0);
    	}       /* parent goes bye-bye */

        /* child continues */
        setsid();       /* become session leader */

        umask(0);       /* clear our file mode creation mask */

    return 0 ;
}


int main(int argc,char **argv)
{ 
    int i;
    char *dev; 
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* descr;
    const u_char *packet;
    struct pcap_pkthdr hdr;     /* pcap.h                    */
    struct ether_header *eptr;  /* net/ethernet.h            */
    struct bpf_program fp;      /* hold compiled program     */
    bpf_u_int32 maskp;          /* subnet mask               */
    bpf_u_int32 netp;           /* ip                        */
	char filtering_str[256];	/* built filtering string 	 */		//expected to be much smaller than 256, so...
	int daemonize_flag = 0;

    errfile_p = fopen(ERRFILE, "a") ;
    if (errfile_p == NULL)
    {
        exit(-1);
    }
    
    logfile_fd = open(LOGFILE, O_WRONLY|O_CREAT, S_IRWXU) ;
    if (logfile_fd < 0) 
    {
        fprintf(errfile_p, "Log file could not be opened\n");
        exit(-1);
    }

#ifdef DEBUG_SS
	sprintf(logfile_buf, "Logfile ready\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
	sprintf(logfile_buf, "Logfile ready\n"); write(logfile_fd, logfile_buf, strlen(logfile_buf));
#endif

    if(argc != 6)
	{ 
#ifdef DEBUG_SS
		sprintf(logfile_buf, "Usage: %s <logging-interval-sec> <self-IP> <affine-IP> <log-file> <daemon-flag>\n",argv[0]); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#else
		fprintf(errfile_p,"Usage: %s <logging-interval-sec> <self-IP> <affine-IP> <log-file> <daemon-flag>\n",argv[0]);
#endif
        return 0;
	}

    /* grab a device to peak into... */
    dev = pcap_lookupdev(errbuf);
    if(dev == NULL)
    { 
#ifdef DEBUG_SS
		sprintf(logfile_buf,"pcap_lookupdev failed. Errbuf = %s\n",errbuf); write(STDOUT_FD, logfile_buf, strlen(logfile_buf)); 
#else
		fprintf(errfile_p,"pcap_lookupdev failed. Errbuf = %s\n",errbuf); 
#endif
		exit(-1);
	}

	logging_interval=atoi(argv[1]);
#ifdef DEBUG_SS
		sprintf(logfile_buf,"logging interval = %d\n", logging_interval); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
	
	strcpy(self_host, argv[2]);
	if (inet_aton(self_host, &self_addr) == 0) 
	{
#ifdef DEBUG_SS
    	sprintf(logfile_buf, "Invalid self address: %s\n", self_host); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#else
    	fprintf(errfile_p, "Invalid self address: %s\n", self_host);
#endif
	    return -1;
  	}

	strcpy(to_host, argv[3]);
	if (inet_aton(to_host, &to_addr) == 0) 
	{
#ifdef DEBUG_SS
    	sprintf(logfile_buf, "Invalid affine address: %s\n", to_host); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#else
    	fprintf(errfile_p, "Invalid affine address: %s\n", to_host);
#endif
	    return -1;
  	}

	strcpy(outfile, argv[4]);
	outfile_p = fopen(outfile, "w");
	if(outfile_p == NULL)
	{
#ifdef DEBUG_SS
		fprintf(stdout, "Unable to open output file %s", outfile) ;
#else
		fprintf(errfile_p, "Unable to open output file %s", outfile) ;
#endif
		return 0 ;
  	}

    daemonize_flag = atoi(argv[5]);
    if (daemonize_flag == 1)
    {   
#ifdef DEBUG_SS
        sprintf(logfile_buf, "Daemonizing report_traff_per_interval \n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
		close(1);
        if ((dup_fd = dup2(logfile_fd, STDOUT_FD)) < 0)
		{
#ifdef DEBUG_SS
      		sprintf(logfile_buf, "dup2 failed\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
			fprintf(errfile_p, "dup2 failed\n");
			exit(-1);
		}
		fprintf(errfile_p, "dup2'ed = %d and %d and %d\n", STDOUT_FD, logfile_fd, dup_fd);
		daemonize();
    }
	else
	{
		sprintf(logfile_buf, "Not daemonizing since daemonize_flag != 1\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
		fprintf(errfile_p, "Not daemonizing since daemonize_flag != 1\n");
	}


    /* ask pcap for the network address and mask of the device */
    pcap_lookupnet(dev,&netp,&maskp,errbuf);

    /* open device for reading this time lets set it in promiscuous
 *      * mode so we can monitor traffic to another machine             */
    descr = pcap_open_live(dev,BUFSIZ,1,-1,errbuf);
    if(descr == NULL)
    { 
#ifdef DEBUG_SS
		sprintf(logfile_buf, "pcap_open_live(): %s\n",errbuf); write(STDOUT_FD, logfile_buf, strlen(logfile_buf)); 
#else
		fprintf(errfile_p, "pcap_open_live(): %s\n",errbuf);  
#endif
		exit(-1);
	}

	/* SSS - build the primitive filtering string from the other argv inputs */
	/* SSS - "host ip" will filter all packets to/from ip */
	sprintf(filtering_str, "host %s", inet_ntoa(self_addr));

	sprintf(self_addr_str, "%s", inet_ntoa(self_addr));
	sprintf(affine_addr_str, "%s", inet_ntoa(to_addr));	

#ifdef DEBUG_SS
	sprintf(logfile_buf, "libpcap's filtering string = %s\n", filtering_str); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
	sprintf(logfile_buf, "self_addr_str = %s\n", self_addr_str); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
	sprintf(logfile_buf, "affine_addr_str = %s\n", affine_addr_str); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

    /* Lets try and compile the program.. non-optimized */
    if(pcap_compile(descr,&fp,filtering_str,0,netp) == -1)
    { 
#ifdef DEBUG_SS
		sprintf(logfile_buf, "Error calling pcap_compile\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf)); 
#else
		fprintf(errfile_p,"Error calling pcap_compile\n"); 
#endif
		exit(-1);
	}

    /* set the compiled program as the filter */
    if(pcap_setfilter(descr,&fp) == -1)
    { 
#ifdef DEBUG_SS
		sprintf(logfile_buf, "Error setting filter\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf)); 
#else
		fprintf(errfile_p,"Error setting filter\n"); 
#endif
		exit(-1);
	}

	//Activate interrupt catcher
	signal(SIGINT, intr_catcher);
	signal(SIGTERM, intr_catcher);

#ifdef DEBUG_SS
     sprintf(logfile_buf, "Activated SIGINT and SIGTERM interrupt catchers\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif
	

	signal(SIGALRM, alarm_catcher);
	alarm(logging_interval);

#ifdef DEBUG_SS
     sprintf(logfile_buf, "Activated logging_interval ALARM\n"); write(STDOUT_FD, logfile_buf, strlen(logfile_buf));
#endif

    fprintf(outfile_p, "date-only\ttime\tA_rxB\tA_txB\tA_rxp\tA_txp\tNA_rxB\tNA_txB\tNA_rxp\tNA_txp\tip_B\tarp\tarp_B\trarp\trarp_B\tmisc\tmisc_B\n");


    /* ... and loop */ 
    pcap_loop(descr,-1,my_callback,NULL);


    return 0;
}


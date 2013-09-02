/*
 * Uptime Client
 *
 * V4.09
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 *
 * This is the brand new version of the uptime client using new uptimes
 * protocol v4. Do with it whatever you like and have fun using it!
 *
 * tgm@uptimes.net
 * alex@uptimes.net
 *
 */

#include "config.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <netdb.h>

#ifndef PLATFORM_OS2
#include <sysexits.h>
#else
#define INCL_DOS
#include <os2.h>
#include <sys/select.h>
#include <stdlib.h>
#include "si2.h"

char *os2loginname, *os2password;
int os2hostid = 0;
#endif

#ifdef PLATFORM_BSD
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#ifdef PLATFORM_ULTRIX
#include <fcntl.h>
#include <nlist.h>
#include <sys/fixpoint.h>
#include <sys/cpudata.h>
#endif

#ifdef PLATFORM_SOLARIS
#include <utmp.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/systeminfo.h>
#endif

#ifdef PLATFORM_UNIXWARE
#define NUM_IDLE_ELEMENTS ((24 * 60 * 60) / INTERVAL) /* Calculate idle time as an average over the whole day */
int past_idle_times[NUM_IDLE_ELEMENTS] = { -1 };
#endif

/* Char array sizes */
#define OS_SIZE		64
#define OSLEVEL_SIZE	64
#define CPU_SIZE	64

static int hostid;

static char base64_table[] =
	{ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	  'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	  'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/', '\0'
	};

static char base64_pad = '=';


unsigned char *base64_encode (const unsigned char *string) {
	int length = strlen (string);
	const unsigned char *current = string;
	int i = 0;
	unsigned char *result = (unsigned char *)malloc(((length + 3 - length % 3) * 4 / 3 + 1) * sizeof(char));

	while (length > 2) { /* keep going until we have less than 24 bits */
		result[i++] = base64_table[current[0] >> 2];
		result[i++] = base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
		result[i++] = base64_table[((current[1] & 0x0f) << 2) + (current[2] >> 6)];
		result[i++] = base64_table[current[2] & 0x3f];

		current += 3;
		length -= 3; /* we just handle 3 octets of data */
	}

	/* now deal with the tail end of things */
	if (length != 0) {
		result[i++] = base64_table[current[0] >> 2];
		if (length > 1) {
			result[i++] = base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
			result[i++] = base64_table[(current[1] & 0x0f) << 2];
			result[i++] = base64_pad;
		}
		else {
			result[i++] = base64_table[(current[0] & 0x03) << 4];
			result[i++] = base64_pad;
			result[i++] = base64_pad;
		}
	}
	result[i] = '\0';
	return result;
}

char *www_post (const char *hostname, const char *url, const char *username, const char *password, const char *proxyusername, const char *proxypassword, const char *data) {
	int sock;
	struct sockaddr_in lsin;
	struct hostent *he;
	char auth[128];
	char header[512];
	int n;
	char rec[4096];
	char *s;
#ifdef PROXYUSERNAME
	char *s2;
#endif
	fd_set rset;

	/* Create socket */
	sock = socket (AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		return "001 ERR: Can't create socket";
	}

	memset (&lsin, 0, sizeof (lsin));
	lsin.sin_family = AF_INET;
#ifdef PROXYSERVER
	lsin.sin_addr.s_addr = inet_addr (PROXYSERVER);
	lsin.sin_port = htons (PROXYPORT);
#else
	lsin.sin_addr.s_addr = inet_addr (hostname);
	lsin.sin_port = htons (80);
#endif
	if (lsin.sin_addr.s_addr == -1) {
		/* Resolve host */
#ifdef PROXYSERVER
		he = gethostbyname (PROXYSERVER);
#else
		he = gethostbyname (hostname);
#endif
		if (!he) {
			close (sock);
#ifdef PROXYSERVER
			return "002 ERR: Can't resolve hostname of proxyserver";
#else
			return "002 ERR: Can't resolve hostname of uptime server";
#endif
		}
		memcpy (&lsin.sin_addr, he->h_addr, he->h_length);
	}

	/* Connect to Uptime Server */
	if (connect (sock, (struct sockaddr *) &lsin, sizeof (lsin)) < 0) {
		close (sock);
#ifdef PROXYSERVER
		return "003 ERR: Can't connect to Proxy Server";
#else
		return "003 ERR: Can't connect to Uptime Server";
#endif
	}

	/* Encode username and password */
	sprintf (auth, "%s:%s", username, password);
	s = base64_encode (auth);

#ifdef PROXYUSERNAME
	/* ... and proxy username and password */
	sprintf (auth, "%s:%s", proxyusername, proxypassword);
	s2 = base64_encode (auth);
#endif

	/* Send POST */
#ifdef PROXYSERVER
#ifdef PROXYUSERNAME
	sprintf (header, "POST http://%s%s HTTP/1.0\nProxy-Connection: Close\nUser-Agent: upclient/4.00/upclient-os2-%s\nAuthorization: Basic %s\nProxy-Authorization: Basic %s\nContent-type: application/x-www-form-urlencoded\nContent-length: %d\n\n", hostname, url, VERSION, s, s2, strlen (data) - 1);
#else
	sprintf (header, "POST http://%s%s HTTP/1.0\nProxy-Connection: Close\nUser-Agent: upclient/4.00/upclient-os2-%s\nAuthorization: Basic %s\nContent-type: application/x-www-form-urlencoded\nContent-length: %d\n\n", hostname, url, VERSION, s, strlen (data) - 1);
#endif
#else
	sprintf (header, "POST %s HTTP/1.0\nConnection: Close\nUser-Agent: upclient/4.00/upclient-os2-%s\nHost: %s\nAuthorization: Basic %s\nContent-type: application/x-www-form-urlencoded\nContent-length: %d\n\n", url, VERSION, hostname, s, (int)strlen (data) - 1);
#endif
	if (send (sock, header, strlen (header), 0) < 0) {
		close (sock);
		return "004 ERR: Can't send header";
	}
	if (send (sock, data, strlen (data), 0) < 0) {
		close (sock);
		return "005 ERR: Can't send data";
	}


	/* Wait for reply */
	FD_ZERO (&rset);
	FD_SET (sock, &rset);
	switch (select (sock + 1, &rset, NULL, NULL, NULL)) {
		case -1:
			close (sock);
			return "006 ERR: Can't receive data";
		case 0:
			close (sock);
			return "007 ERR: Timeout";
		default:
			n = read (sock, rec, sizeof (rec) - 1);
			s = strstr (rec, "\r\n\r\n") + 4;
			if (s) {
				close (sock);
				return s;
			}
	}

	close (sock);

	return "006 ERR: No result received";

}

#if defined(PLATFORM_OS2)
#include <sys/utsname.h>
int uname(struct utsname *buf)
{
	ULONG aulBuffer[4];
	APIRET rc;
	if (!buf) return EFAULT;
	rc = DosQuerySysInfo(QSV_VERSION_MAJOR, QSV_MS_COUNT,(void *)aulBuffer, 4*sizeof(ULONG));
	strcpy(buf->sysname,"OS/2");
	strcpy(buf->release, "1.x");
	if (aulBuffer[0] == 20)
	{
		int i = (unsigned int)aulBuffer[1];
		sprintf(buf->release, "%u", i);
		if (i > 20)
		{
			strcpy(buf->sysname,"Warp");
                        sprintf(buf->release, "%d.%d", (int)i/10, i-(((int)i/10)*10));
		}
		else if (i == 10)
			strcpy(buf->release, "2.1");
		else if (i == 0)
			strcpy(buf->release, "2.0");
		strcpy(buf->machine, "i386");
                if(getenv("HOSTNAME") != NULL)
		   strcpy(buf->nodename, getenv("HOSTNAME"));
                else
                   strcpy(buf->nodename, "unknown");
	}
	return 0;
}

/* replacement uptime and load query from pumon */
APIRET get_load(double *load, int *idle, int CPU)
{
    static double  ts_val,   ts_val_prev[CPUS]  ={0.,0.,0.,0.};
    static double  idle_val, idle_val_prev[CPUS]={0.,0.,0.,0.};
    static double  busy_val, busy_val_prev[CPUS]={0.,0.,0.,0.};
    static double  intr_val, intr_val_prev[CPUS]={0.,0.,0.,0.};
    static CPUUTIL CPUUtil[CPUS];
	static int     CPUIdle[CPUS];
    static double  CPULoad[CPUS];
	static int iter = 0;
    int i;
    APIRET rc;

    if(CPU > 0) //Just return value, needless to measure it again
    {
        *load = (CPU >= 0 && CPU < CPUS) ? CPULoad[CPU]:0;
        return 0;
    }

    do
    {
        rc = DosPerfSysCall(CMD_KI_RDCNT,(ULONG) &CPUUtil[0],0,0);
        if (rc)
            return rc;

        for(i=0; i < CPUS; i++)
        {
            ts_val   = LL2F(CPUUtil[i].ulTimeHigh, CPUUtil[i].ulTimeLow);
            idle_val = LL2F(CPUUtil[i].ulIdleHigh, CPUUtil[i].ulIdleLow);
            busy_val = LL2F(CPUUtil[i].ulBusyHigh, CPUUtil[i].ulBusyLow);
            intr_val = LL2F(CPUUtil[i].ulIntrHigh, CPUUtil[i].ulIntrLow);

            if(!ts_val)
                continue;

            if(iter > 0)
            {
                double ts_delta = ts_val - ts_val_prev[i];
                double usage = ((busy_val - busy_val_prev[i])/ts_delta);
                double pidle = ((idle_val - idle_val_prev[i])/ts_delta)*100.0;

                if(usage > 99.0)
                    usage = 100;

                CPULoad[i] = usage;
                CPUIdle[i] = pidle;
            }

            ts_val_prev[i]   = ts_val;
            idle_val_prev[i] = idle_val;
            busy_val_prev[i] = busy_val;
            intr_val_prev[i] = intr_val;
        }
        if(!iter)
            DosSleep(32);

        if(iter < 3)
            iter++;
    }
    while(iter < 2);

    *load = (CPU >= 0 && CPU < CPUS) ? CPULoad[CPU]:0;
    *idle = (CPU >= 0 && CPU < CPUS) ? CPUIdle[CPU]:100;
    return 0;
}

APIRET get_uptime(PULONG pulSeconds)
{
    QWORD qwTmrTime;
    APIRET rc;
    ULONG ulSeconds = 0;
    static ULONG ulTmrFreq = 0;

    if(!ulTmrFreq)
    {
        rc = DosTmrQueryFreq(&ulTmrFreq);
        if(rc)
        {
            ULONG ulMsCount;
            rc = DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, (PVOID)&ulMsCount, sizeof(ULONG));

            if(rc)
                return rc;

            ulSeconds = ulMsCount /1000;
        }
    }
    if(!ulSeconds)
    {
        rc = DosTmrQueryTime(&qwTmrTime);

        if(rc)
            return rc;

        ulSeconds  = LL2F(qwTmrTime.ulHi, qwTmrTime.ulLo) / ((double)ulTmrFreq);
    }
    *pulSeconds = ulSeconds;
    return 0;
}

void getstats(unsigned long* puptime, double* pload, int* pidle, char* os, char* oslevel, char* cpu) {
	struct utsname uts;
    ULONG upsecs;

	get_uptime(&upsecs);
	*puptime = (unsigned long)upsecs/60;

	get_load(pload, pidle, 0);

#if defined(SEND_OS)
	/* Get os info */
	uname (&uts);
#elif defined(SEND_CPU)
	/* Get CPU info */
	uname (&uts);
#endif

#if defined(SEND_OS)
	strncpy (os, uts.sysname, OS_SIZE - 1);
# if defined(SEND_OSLEVEL)
	strncpy (oslevel, uts.release, OSLEVEL_SIZE - 1);
# endif
#endif

#ifdef SEND_CPU
	strncpy (cpu, uts.machine, CPU_SIZE - 1);
#endif
   }
#endif

#if defined(PLATFORM_LINUX)
void getstats(unsigned long* puptime, double* pload, int* pidle, char* os, char* oslevel, char* cpu) {
	struct utsname uts;
	FILE *fp;
	double up=0, id=0;

	/* Get uptime */
	fp = fopen ("/proc/uptime", "r");
	if (!fp)
		exit (-2);

	if (fscanf (fp, "%lf %lf", &up, &id) < 2)
		exit (-3);
	fclose (fp);
	*puptime = (int) (up / 60.0);
	
#if defined(NR_LINUX_UPTIME_WRAPAROUNDS)
        /* Make up for those misserable uptime wraparounds */
	*puptime += ((ULONG_MAX / 100) / 60) * NR_LINUX_UPTIME_WRAPAROUNDS;
#endif

#ifdef SEND_LOADAVG
	/* Get load */
	fp = fopen ("/proc/loadavg", "r");
	fscanf (fp, "%*f %*f %lf %*s", pload);
	fclose (fp);
#endif

#ifdef SEND_IDLETIME
	/* Get idle time */
	*pidle = (int) (100.0 * id / up);
#endif

#if defined(SEND_OS)
	/* Get os info */
	uname (&uts);
#elif defined(SEND_CPU)
	/* Get CPU info */
	uname (&uts);
#endif

#if defined(SEND_OS)
	strncpy (os, uts.sysname, OS_SIZE - 1);
# if defined(SEND_OSLEVEL)
	strncpy (oslevel, uts.release, OSLEVEL_SIZE - 1);
# endif
#endif

#ifdef SEND_CPU
	strncpy (cpu, uts.machine, CPU_SIZE - 1);
#endif
}
#endif

#if defined(PLATFORM_BSD)
void getstats(unsigned long* puptime, double* pload, int* pidle, char* os, char* oslevel, char* cpu) {
	struct utsname uts;
	struct timeval boottime;
	time_t now;
	size_t size;
	double loadavgs[2];
	int mib[2];

	/* Get uptime */
	time (&now);
	mib[0] = CTL_KERN;
	mib[1] = KERN_BOOTTIME;
	size = sizeof (boottime);
	if (sysctl (mib, 2, &boottime, &size, NULL, 0) != -1 &&	(boottime.tv_sec != 0)) {
		*puptime = now - boottime.tv_sec;
		*puptime /= 60;
	}

#ifdef SEND_LOADAVG
	/* Get load average */
	getloadavg(loadavgs, 3);
	/* Use the 3rd element (15 minute load average) */
	*pload = loadavgs[2];
#endif

#if defined(SEND_OS)
	/* Get os info */
	uname (&uts);
#elif defined(SEND_CPU)
	/* Get CPU info */
	uname (&uts);
#endif

#if defined(SEND_OS)
	strncpy (os, uts.sysname, OS_SIZE - 1);
#if defined(SEND_OSLEVEL)
	strncpy (oslevel, uts.release, OSLEVEL_SIZE - 1);
#endif
#endif

#ifdef SEND_CPU
	strncpy (cpu, uts.machine, CPU_SIZE - 1);
#endif
}
#endif

#if defined(PLATFORM_UNIXWARE)
void getstats(unsigned long* puptime, double* pload, int* pidle, char* os, char* oslevel, char* cpu) {
	struct utsname uts;
	FILE *fp;
	char dummy_str[100];
	int days;
	char uptime_today[10];
	unsigned long uptime_mins;
	int temp;
	char sar_cmd[40];
	int pct_idle;
	int i;

	/* Get uptime */
	fp = popen("/usr/bin/uptime", "r");
	if (!fp)
		exit (-2);

	if (fscanf (fp, "%s %s %d %s %s %s", dummy_str, dummy_str, &days, dummy_str, uptime_today, dummy_str) < 2)
		exit (-3);
	fclose (fp);
	uptime_mins = days * 24 * 60;
	temp = atoi(strtok(uptime_today, ":"));
	uptime_mins += temp * 60;
	temp = atoi(strtok(0, ":"));
	*puptime = uptime_mins + temp;

#ifdef SEND_IDLETIME
	/* Get idle time for the current interval */
	sprintf(sar_cmd, "/usr/bin/sar %d 1", INTERVAL);
	fp = popen(sar_cmd, "r");
	fgets(dummy_str, sizeof(dummy_str), fp);	/* blank */
	fgets(dummy_str, sizeof(dummy_str), fp);	/* same as "uname -a" */
	fgets(dummy_str, sizeof(dummy_str), fp);	/* blank */
	fgets(dummy_str, sizeof(dummy_str), fp);	/* header line */
	fscanf (fp, "%s %d %d %d %d", dummy_str, &temp, &temp, &temp, &pct_idle);
	fclose (fp);

	/* First time through, init idle array to the first idle value we
	   capture.  That seems most fair. */
	if (past_idle_times[0] == -1)
		for (i = 0; i < NUM_IDLE_ELEMENTS; ++i)
			past_idle_times[i] = pct_idle;

	/* Add interval to idle array, and calculate the average idle time. */
	temp = 0;
	for (i = 1; i < NUM_IDLE_ELEMENTS; ++i)
		temp += (past_idle_times[i - 1] = past_idle_times[i]);

	temp += (past_idle_times[NUM_IDLE_ELEMENTS - 1] = pct_idle);
	*pidle = temp / (NUM_IDLE_ELEMENTS);
#endif

#ifdef SEND_OS
	/* Get os info */
	uname (&uts);
#else
#ifdef SEND_CPU
	/* Get CPU info */
	uname (&uts);
#endif
#endif

#if defined(SEND_OS)
	strncpy (os, uts.sysname, OS_SIZE - 1);
	if (strcmp (os, "UNIX_SV") == 0)
		strcpy(os, "UnixWare"); /* Change a stupid OS name to something marginally less stupid. */
#if defined(SEND_OSLEVEL)
	/* strncpy (oslevel, uts.release, OSLEVEL_SIZE - 1); */
	strncpy (oslevel, uts.version, OSLEVEL_SIZE - 1);
#endif
#endif

#ifdef SEND_CPU
	strncpy (cpu, uts.machine, CPU_SIZE - 1);
#endif
}
#endif

#if defined(PLATFORM_ULTRIX)
void getstats(unsigned long* puptime, double* pload, int* pidle, char* os, char* oslevel, char* cpu) {
	struct utsname uts;
	time_t now, boottime;
	int fd, up, id, avenrun[3];
	long cpudata_offset;
	struct nlist names[2];
	struct cpudata cpudata;

	/* Open file descriptor to /dev/kmem first */
	if ((fd = open ("/dev/kmem", O_RDONLY)) < 0)
		exit (-2);
	/* Make sure second nlist name pointer is a NULL so nlist(3) knows when to stop. */
	names[1].n_name = NULL;

	/* Get uptime */
	time (&now);
	names[0].n_name = "boottime";
	if (nlist ("/vmunix", names) != 0)
		exit (-3);
	if (lseek (fd, names[0].n_value, SEEK_SET) == -1)
		exit (-4);
	if (read(fd, &boottime, sizeof(boottime)) < 0)
		exit (-5);
	*puptime = (now - boottime) / 60;

#ifdef SEND_LOADAVG
	names[0].n_name = "avenrun";
	if (nlist ("/vmunix", names) != 0)
		exit (-6);
	if (lseek (fd, names[0].n_value, SEEK_SET) == -1)
		exit (-7);
	if (read(fd, avenrun, sizeof(avenrun)) < 0)
		exit (-8);
	/* Use the 3rd element (15 minute load average) */
	*pload = FIX_TO_DBL(avenrun[2]);
#endif

#ifdef SEND_IDLETIME
	names[0].n_name = "cpudata";
	if (nlist ("/vmunix", names) != 0)
		exit (-10);
	if ((fd = open ("/dev/kmem", O_RDONLY)) < 0)
		exit (-11);
	if (lseek (fd, names[0].n_value, SEEK_SET) == -1)
		exit (-12);
	if (read(fd, &cpudata_offset, sizeof(cpudata_offset)) < 0)
		exit (-13);
	if (lseek (fd, cpudata_offset, SEEK_SET) == -1)
		exit (-13);
	if (read(fd, &cpudata, sizeof(cpudata)) < 0)
		exit (-13);
	id = cpudata.cpu_cptime[CP_IDLE];
	*pidle = (int) (100.0 * cpudata.cpu_cptime[CP_IDLE] / (cpudata.cpu_cptime[0] + cpudata.cpu_cptime[1] + cpudata.cpu_cptime[2] + cpudata.cpu_cptime[3]));
	/* Check for wrap-around */
	if (*pidle < 0 || *pidle > 100)
		*pidle = -1;
#endif

#if defined(SEND_OS) || defined(SEND_CPU)
	uname (&uts);
#endif

#ifdef SEND_OS
	strncpy (os, uts.sysname, OS_SIZE - 1);
	/* Make "ULTRIX" look prettier */
	if (strcmp(os, "ULTRIX") == 0)
		strncpy(os, "Ultrix", OS_SIZE - 1);
#ifdef SEND_OSLEVEL
	strncpy (oslevel, uts.release, OSLEVEL_SIZE - 1);
#endif
#endif

#ifdef SEND_CPU
	strncpy (cpu, uts.machine, CPU_SIZE - 1);
	/* Make "RISC" look prettier under Ultrix */
	if (strcmp(os, "Ultrix") == 0 && strcmp(cpu, "RISC") == 0)
		strncpy(cpu, "MIPS", CPU_SIZE - 1);
#endif

	/* Close the kernel file descriptor */
	close(fd);
}
#endif

#if defined(PLATFORM_SOLARIS)
void getstats(unsigned long* puptime, double* pload, int* pidle, char* os, char* oslevel, char* cpu) {
	int fd;
	time_t thetime;
	struct utmp ut;
	int found;

	fd = open (UTMP_FILE, O_RDONLY);
	if (fd < 0) {
		perror ("UTMP_FILE");
		exit (1);
	}

	found = 0;
	while (!found) {
		if (read (fd, &ut, sizeof (ut)) < 0)
			found = -1;
		else
			if (ut.ut_type == BOOT_TIME)
				found = 1;	
	}
	close (fd);

	if (found == -1) {
		fprintf (stderr, "Could not find uptime.\n");
		exit (1);
	}

	time (&thetime);
	*puptime = (thetime - ut.ut_time) / 60;

#if defined(SEND_OS)
	if (sysinfo (SI_SYSNAME, os, OS_SIZE - 1) < 0)
		perror ("sysinfo() call");
#endif
#if (defined(SEND_OSLEVEL) && defined(SEND_OS))
	if (sysinfo (SI_RELEASE, oslevel, OSLEVEL_SIZE - 1) < 0)
		perror ("sysinfo() call");
#endif
#if defined(SEND_CPU)
	if (sysinfo (SI_ARCHITECTURE, cpu, CPU_SIZE - 1) < 0)
		perror ("sysinfo() call");
#endif
}
#endif

void send_update(void) {
	char *ret;
	unsigned long uptime = 0;
	double load = -1.0;
	int idle = -1;
	char os[OS_SIZE] = "";
	char oslevel[OSLEVEL_SIZE] = "";
	char cpu[CPU_SIZE] = "";
	char data[256];

	/* Get the machine's uptime and other stats */
	getstats(&uptime, &load, &idle, os, oslevel, cpu);

#if defined(DEBUG)
	printf("Stats: uptime %ld, load %.1f, idle %d, OS %s, level %s, CPU %s.\n", uptime, load, idle, os, oslevel, cpu);
#endif

	/* Create packet */
	sprintf (data, "hid=%d&uptime=%ld", hostid, uptime);

#ifdef SEND_LOADAVG
	sprintf (data, "%s&load=%.2f", data, load);
#endif
#ifdef SEND_IDLETIME
	sprintf (data, "%s&idle=%d", data, idle);
#endif
#ifdef SEND_OS
	sprintf (data, "%s&os=%s", data, os);
#endif
#ifdef SEND_OSLEVEL
	sprintf (data, "%s&oslevel=%s", data, oslevel);
#endif
#ifdef SEND_CPU
	sprintf (data, "%s&cpu=%s", data, cpu);
#endif

	sprintf (data, "%s\n", data);

	/* Send packet */
#ifdef PROXYUSERNAME
	ret = www_post (UPSERVER, "/server.html", LOGINNAME, PASSWORD, PROXYUSERNAME, PROXYPASSWORD, data);
#else
	ret = www_post (UPSERVER, "/server.html", LOGINNAME, PASSWORD, NULL, NULL, data);
#endif
#ifdef DEBUG
	printf ("The uptime server returned: %s\n", ret);
#endif
#if defined(PLATFORM_OS2)
        DosSleep(1000 * INTERVAL);
#endif
}

void timertick (int sig) {
	send_update();
}

void write_pidfile (int pid) {
	FILE *fp;
	fp = fopen (PIDFILE, "w");
	if (!fp) {
		fprintf (stderr, "Warning: Can't write pidfile\n");
		return;
	}
	fprintf (fp, "%d\n", pid);
	fclose (fp);
}

int main (int argc, char **argv) {
#if !defined(PLATFORM_UNIXWARE) && !defined(PLATFORM_OS2) /* warning eater(tm) */
	struct sigaction sa;
	struct itimerval timer;
#endif
#if !defined(DEBUG) && !defined(PLATFORM_OS2)
	int pid;
#endif
#if defined(PLATFORM_OS2)
        os2loginname = getenv("OS2LOGINNAME");
        os2password = getenv("OS2PASSWORD");
        if(getenv("OS2HOSTID"))
            os2hostid = atoi(getenv("OS2HOSTID"));
#endif

	/* Check for commandline hostid parameter */
	if(argc == 2) {
		hostid = strtol(argv[1], (char **)NULL, 10);
		if (errno == ERANGE) {
			perror("Host ID");
			printf("Usage: upclient [hostid]\n");
			exit (1);
		}
	}
	else
		hostid = HOSTID;

#if !defined(DEBUG) && !defined(PLATFORM_OS2)
	/* Fork into background */
	switch (pid = fork ()) {
		case -1:
			fprintf (stderr, "ERROR: Couldn't fork into background (%s)\n", strerror (errno));
			exit (1);
		case 0:
			close (0); close (1); close (2);
			break;
		default:
			printf ("Uptime client started (pid %d)\n", pid);
			write_pidfile (pid);
			exit (0);
	}
#endif

#if defined(PLATFORM_UNIXWARE) || defined(PLATFORM_OS2)
	for (;;) {
		send_update();
	}
#else
	/* Setup the timer */
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = timertick;
	if(sigaction (SIGALRM, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	timer.it_value.tv_sec = 1;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = INTERVAL;
	timer.it_interval.tv_usec = 0;
	if(setitimer (ITIMER_REAL, &timer, NULL) == -1) {
		perror("setitimer");
		exit(2);
	}

	/* Now go idle(tm) */
	for (;;)
		pause ();
#endif

#if !defined(PLATFORM_UNIXWARE) /* warning eater */
	return 1;
#endif
}

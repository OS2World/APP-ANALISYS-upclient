/*
 * Uptime Client V4.09
 *
 * config.h
 *
 */
#define VERSION "4.09"


/* IMPORTANT!!
 *
 * Guess!!! --> #define PLATFORM_CODE -1
 * Linux    --> #define PLATFORM_CODE  0
 * *BSD     --> #define PLATFORM_CODE  1
 * UnixWare --> #define PLATFORM_CODE  2
 * Ultrix   --> #define PLATFORM_CODE  3
 * Solaris  --> #define PLATFORM_CODE  4
 * OS/2     --> #define PLATFORM_CODE  5
 */
#define PLATFORM_CODE       -1

#if   (PLATFORM_CODE == 0)
#	define PLATFORM_LINUX
#elif (PLATFORM_CODE == 1 || defined(__NetBSD__) || defined(__FreeBSD_version) || defined(OpenBSD))
#	define PLATFORM_BSD
#elif (PLATFORM_CODE == 2)
#	define PLATFORM_UNIXWARE
#elif (PLATFORM_CODE == 3 || defined(ultrix))
#	define PLATFORM_ULTRIX
#elif (PLATFORM_CODE == 4)
#	define PLATFORM_SOLARIS
#elif (PLATFORM_CODE == 5)
#	define PLATFORM_OS2
#else
#	error "You need to define your PLATFORM, pal."
#endif



/* ALSO IMPORTANT!
 *
 * If you are the proud owner of a Linux box, but your uptime
 * wraps around after 497.1026963 days, you can define here
 * how many times that cruel thing happened to your box.
 */
/* #define NR_LINUX_UPTIME_WRAPAROUNDS		-1 */



/* Loginname and password
 *
 * If you haven't registered yourself yet, go to
 * http://www.uptimes.net/register.html
 */
#define LOGINNAME		"Enter your loginname here"
#define PASSWORD		"Enter your password here"



/* Host ID
 *
 * This is the ID of the host you're going to run
 * this client on. You can find it in the first
 * column on http://www.uptimes.net/hosts.html.
 * If your host isn't in the list, click "add host"
 */
#define HOSTID			0



/* Uptime server
 *
 * The client should know where to reach the
 * Uptime server. The default value is ok.
 */
#define UPSERVER		"data.uptimes.net"



/* Proxy Server
 *
 * Define the following when you want to use a
 * proxy server.  Some proxies also require a
 * username and password.
 */
/* #define PROXYSERVER		"my-proxy-server" */
/* #define PROXYPORT		8080 */
/* #define PROXYUSERNAME	"<my-proxy-username>" */
/* #define PROXYPASSWORD	"<my-proxy-server>" */



/* Interval
 *
 * Specify the time between two updates. 120 seconds
 * is a reasonable value. Don't make it lower than
 * 30 seconds or higher than 600.
 */
#define INTERVAL		120



/* Options
 *
 * This client can send various options along with
 * the uptime:
 *
 * SEND_IDLETIME: This is the % of the time the
 *                host has been idle
 * SEND_LOADAVG:  The average load of the host
 *                (over a period of 15 minutes)
 * SEND_OS:       The operating system this client
 *                is running on
 * SEND_OSLEVEL:  The version of the operating
 *                system
 * SEND_CPU:      The CPU powering the system
 *                the client is running on.
 */
#define SEND_IDLETIME
#define SEND_LOADAVG
#define SEND_OS
#define SEND_OSLEVEL
#define SEND_CPU



/* Pidfile
 *
 * Where should the client write it's pidfile?
 */
#define PIDFILE			"<path-to>/upclient.pid"



/* Debug
 *
 * If something doesn't work, you could try defining
 * this
 */
/* #define DEBUG */



/* N O  U S E R  S E R V I C A B L E  P A R T S  B E L O W  H E R E !
 *                   you have been warned :-)
 */



/* Sanity check SEND_ macros -- if a platform doesn't support a given
 * feature, undefine the macro.
 */
#if defined(PLATFORM_UNIXWARE)
#	undef SEND_LOADAVG
#elif defined(PLATFORM_BSD)
#	undef SEND_IDLETIME
#endif

#if defined(NR_LINUX_UPTIME_WRAPAROUNDS)
#if (NR_LINUX_UPTIME_WRAPAROUNDS < 1)
#	error "Define your uptime wraparounds properly, pal."
#endif
#endif

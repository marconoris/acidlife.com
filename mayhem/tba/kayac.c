// $Id: kayac.c,v 109 2002/11/29 6:11:46 xenion Exp $ //
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <setjmp.h>
#include "tcplib.c"

#define FAKE_TIMEOUT 5
#define DATA_TIMEOUT 5
#define VERSION "0.1"
#define BUFLEN 4192

#define MSG_HEADER "\n\nKaYA SERVER: "
#define MSG_TRILER "\n\n"
#define MSG_FAKE MSG_HEADER "kiddie, fake connection already UP!\n" MSG_TRILER


#define TRUE 1
#define FALSE 0

#define MAX(a, b) (a > b ? a : b)

#define SIG_NAME(x) x == SIGURG  ? "SIGURG"  : \
                    x == SIGPIPE ? "SIGPIPE" : \
                    x == SIGQUIT ? "SIGQUIT" : \
                    x == SIGINT  ? "SIGINT"  : \
                    x == SIGTERM ? "SIGTERM" : \
                    x == SIGHUP  ? "SIGHUP"  : \
                    x == SIGSEGV ? "SIGSEGV" : \
                    x == SIGBUS  ? "SIGBUS"  : "UNKNOWN"


void            fatal(char *, ...);
void            init_opt(int, char **);
void            help();
void            get_shost(char *);
void            get_dhost(char *);
size_t          getmytime(char *, size_t, const char *);
void            printlog(const char *format, ...);
void            sigdie(int);



typedef struct {

    /*
     * hosts 
     */

    unsigned long   dhost;
    unsigned long   shost;

    /*
     * TCP ports 
     */

    int             sport0;
    int             sport1;
    int             dport;

    /*
     * sockets 
     */

    int             sfake_in;	// fake_client -> kaya
    int             sfake_out;	// kaya_data -> victim
    int             sdata_in;	// data_client -> kaya
    int             sdata_out;	// data_kaya -> victim 

    int             listen_sfake;
    int             listen_sdata;

    /*
     * misc 
     */

    int             fake_connected;
    int             data_connected;

    FILE           *lfile;
    FILE           *pfile;

    int             bg;		/* run in background */

    int             fake_timeout;
    int             data_timeout;


} OPT;


OPT             o;


int
main(int argc, char **argv)
{
    char            buf[BUFLEN];	/* I/O buffer */
    fd_set          rx_set,	/* Read set */

                    wk_set;	/* Working set */
    int             mx,		/* max fd, used with selec() */

                    z,
                    s,		/* temp socket */

                    bytes;


    init_opt(argc, argv);

    if (o.bg == TRUE)
	if (fork())
	    exit(0);

    if (o.bg == TRUE)
	printlog("Running in background\n");
    printlog("(pid: %d)\n", getpid());
    printlog("\n");

    signal(SIGTERM, sigdie);
    signal(SIGINT, sigdie);
    signal(SIGQUIT, sigdie);
    signal(SIGHUP, sigdie);
    signal(SIGSEGV, sigdie);
    signal(SIGPIPE, sigdie);
    signal(SIGURG, SIG_IGN);
    signal(SIGALRM, conn_timeout);

    printlog(".---- %s:%d (fake timeout: %ds)\n", ipfromlong(o.shost),
	     o.sport0, o.fake_timeout);
    printlog("|.--- %s:%d (data timeout: %ds)\n", ipfromlong(o.shost),
	     o.sport1, o.data_timeout);
    printlog("|.\n");
    printlog("`-.-.-> %s:%d\n", ipfromlong(o.dhost), o.dport);
    printlog("\n");

    o.listen_sfake = mylisten(o.shost, o.sport0);
    o.listen_sdata = mylisten(o.shost, o.sport1);

    FD_ZERO(&rx_set);
    FD_SET(o.listen_sfake, &rx_set);
    FD_SET(o.listen_sdata, &rx_set);

    mx = MAX(o.listen_sfake, o.listen_sdata) + 1;

    o.fake_connected = FALSE;
    o.data_connected = FALSE;

    printlog("  Running..\n");


    for (;;) {

	for (z = mx - 1; z >= 0 && !FD_ISSET(z, &rx_set); z = mx - 1)
	    mx = z;

	FD_ZERO(&wk_set);

	for (z = 0; z < mx; ++z)
	    if (FD_ISSET(z, &rx_set))
		FD_SET(z, &wk_set);

	z = select(mx, &wk_set, NULL, NULL, NULL);
	if (z == -1)
	    fatal("select()");

	if (FD_ISSET(o.listen_sfake, &wk_set)) {

	    /*
	     * fake connection request
	     */

	    /*
	     * In order to be notified of incoming connections on a
	     * socket, you can use select(2) or poll(2).  A readable event
	     * will be delivered when a new connection is attempted and
	     * you may then call accept to get a socket for that con-
	     * nection.
	     */

	    printlog("  fake: connection request\n");

	    s = get_new_connection(o.listen_sfake);
	    if (s == -1) {
		printlog("  fake: client-kaya connection failed\n");
		continue;
	    }

	    if (o.fake_connected == TRUE) {
		printlog("  fake: connection already UP, closed\n");
		send(s, MSG_FAKE, sizeof MSG_FAKE, 0);
		close(s);
		continue;
	    }

	    alarm(o.fake_timeout);
	    z = connect_to(&(o.sfake_out), o.dhost, o.dport);
	    alarm(0);

	    if (z < 0) {
		printlog
		    ("  fake: kaya-victim connection failed (%s)\n",
		     z == -1 ? "ECONNREFUSED" : "TIMEOUT");
		close(s);
		printlog("  fake: client-kaya connection closed\n");
		continue;
	    }
	    // printlog(" fake: kaya-victim connected\n");

	    printlog("  fake: connection UP!\n");

	    o.sfake_in = s;

	    FD_SET(o.sfake_in, &rx_set);
	    FD_SET(o.sfake_out, &rx_set);

	    mx = MAX(MAX(o.sfake_in, o.sfake_out) + 1, mx);

	    o.fake_connected = TRUE;

	    continue;
	}


	memset(buf, 0, BUFLEN);

	if FD_ISSET
	    (o.sfake_in, &wk_set) {
	    bytes = recv(o.sfake_in, buf, sizeof(buf), 0);

	    if (o.pfile && bytes > 0) {
		fwrite(buf, 1, bytes, o.pfile);
		fflush(o.pfile);
	    }

	    if (bytes == 0) {

		/*
		 * even if no data is sent by the client process to read,
		 * the act of the client closing the socket will cause
		 * select(2) to register a read event.
		 */

		/*
		 * fakeclient-kaya connection closed 
		 */

		printlog
		    ("* fake: client connection lost, closing all connections.\n");

		FD_CLR(o.sfake_in, &rx_set);
		FD_CLR(o.sfake_out, &rx_set);

		close(o.sfake_in);
		close(o.sfake_out);

		o.fake_connected = FALSE;

		continue;
	    }

	    send(o.sfake_out, buf, bytes, 0);
	    }



	memset(buf, 0, BUFLEN);

	if FD_ISSET
	    (o.sfake_out, &wk_set) {
	    bytes = recv(o.sfake_out, buf, sizeof(buf), 0);

	    if (o.pfile && bytes > 0) {
		fwrite(buf, 1, bytes, o.pfile);
		fflush(o.pfile);
	    }

	    if (bytes == 0) {

		/*
		 * even if no data is sent by the client process to read,
		 * the act of the client closing the socket will cause
		 * select(2) to register a read event.
		 */

		/*
		 * kaya-victim connection closed
		 */

		printlog
		    ("* fake: victim connection lost, closing all connections.\n");

		FD_CLR(o.sfake_in, &rx_set);
		FD_CLR(o.sfake_out, &rx_set);

		close(o.sfake_in);
		close(o.sfake_out);

		o.fake_connected = FALSE;

		continue;
	    }
	    send(o.sfake_in, buf, bytes, 0);
	    }




    }

    sigdie(SIGQUIT);
    return (0);
}


void
init_opt(int argc, char **argv)
{
    int             c;

    o.dhost = INADDR_ANY;
    o.shost = INADDR_ANY;
    o.sport0 = 0;
    o.sport1 = 0;
    o.dport = 0;
    o.fake_timeout = FAKE_TIMEOUT;
    o.data_timeout = DATA_TIMEOUT;
    o.bg = FALSE;

    o.lfile = stdout;
    o.pfile = NULL;

    while ((c = getopt(argc, argv, "s:d:t:T:l:p:bh")) != EOF)
	switch (c) {

	case 's':
	    get_shost(optarg);
	    break;


	case 'd':
	    get_dhost(optarg);
	    break;

	case 't':
	    o.fake_timeout = atoi(optarg);
	    break;

	case 'T':
	    o.data_timeout = atoi(optarg);
	    break;


	case 'l':
	    if (o.lfile != stdout)
		break;

	    o.lfile = fopen(optarg, "w");
	    if (o.lfile == NULL) {
		o.lfile = stdout;
		fatal("unable to open log file");
	    }

	    break;

	case 'p':
	    if (o.lfile != NULL)
		break;

	    o.pfile = fopen(optarg, "w");
	    if (o.pfile == NULL)
		fatal("unable to open pakets log file");
	    break;


	case 'b':
	    o.bg = TRUE;
	    break;

	case 'h':
	    help();
	    break;

	default:
	    fatal("try -h");
	}



    if (!o.sport0)
	fatal("source port 0 needed");
    if (!o.sport1)
	fatal("source port 1 needed");
    if (!o.dport)
	fatal("destination port needed");

}


void
get_shost(char *s)
{

    char           *p,
                   *p_old,
                    e;


    e = 0;

    /*
     * host:port0:port1
     */

    for (p = s; *p != '\0' && *p != ':'; p++);

    if (*p == '\0')
	++e;
    else
	*p = '\0';

    o.shost = getlongbyname(s);

    if (e)
	return;

    p_old = ++p;

    while (*p != '\0' && *p != ':')
	p++;

    if (*p == '\0')
	++e;
    else
	*p = '\0';

    o.sport0 = atoi(p_old);

    if (e)
	return;

    o.sport1 = atoi(++p);

}


void
get_dhost(char *s)
{

    char           *p,
                    e;

    e = 0;

    /*
     * host:port
     */

    for (p = s; *p != '\0' && *p != ':'; p++);

    if (*p == '\0')
	++e;
    else
	*p = '\0';

    o.dhost = getlongbyname(s);

    if (e)
	return;

    ++p;

    o.dport = atoi(p);

}

void
printlog(const char *format, ...)
{

    char            timebuf[20];

    va_list         ap;
    va_start(ap, format);

    getmytime(timebuf, 20, "%d/%m/%y#%H:%M:%S");

    fprintf(o.lfile, "%s ", timebuf);
    vfprintf(o.lfile, format, ap);
    fflush(o.lfile);

    va_end(ap);

}

size_t
getmytime(char *s, size_t max, const char *format)
{

    time_t          t;
    struct tm      *mytm;
    size_t          z;

    time(&t);

    mytm = localtime(&t);

    z = strftime(s, max, format, mytm);

    return (z);

}


void
fatal(char *pattern, ...)
{
    va_list         ap;
    va_start(ap, pattern);

    vfprintf(stderr, pattern, ap);
    fprintf(stderr, "; exit forced.\n");

    va_end(ap);

    exit(-1);
}


void
sigdie(int signo)
{

    fprintf(o.lfile, "\n");
    printlog("caught %s signal (%d), cleaning up\n", SIG_NAME(signo),
	     signo);

    close(o.sfake_in);
    close(o.sfake_out);
    close(o.sdata_in);
    close(o.sdata_out);

    if (o.lfile != stdout)
	fclose(o.lfile);
    if (o.pfile != NULL)
	fclose(o.pfile);

    exit(0);

}

void
help()
{

    printf("KaYA client v%s\n\n", VERSION);
    printf("USAGE: kayac [options]\n\n");
    printf
	("-s shost|any:sport0:sport1          Source host and port(s) to listen on\n");
    printf
	("-d dhost:dport                      Destination host and port to connect to\n");
    printf
	("-t timeout                          fake timeout (default: %ds)\n",
	 FAKE_TIMEOUT);
    printf
	("-T timeout                          data timeout (default: %ds)\n",
	 DATA_TIMEOUT);
    printf
	("-l filename                         Use file instead of stdout\n");
    printf("-p filename                         Log packets to file\n");
    printf("-b                                  Run in background\n");
    printf("-V                                  Display version number\n");
    printf("-h                                  This\n\n");
    exit(0);

}

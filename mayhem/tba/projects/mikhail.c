/*
 *
 * $Id: mikhail.c,v 2.5beta12 2002/03/28 18:03:34 xenion Exp $
 *
 * ---------------------------------------------------------------------------
 * No part of this project may be used to break the law, or to cause damage of
 * any kind. And I'm not responsible for anything you do with it.
 * ---------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (by Poul-Henning Kamp, Revision 42):
 * <xenion@acidlife.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you
 * think this stuff is worth it, you can buy me a beer in return.
 * xenion ~ Dallachiesa Michele
 * ---------------------------------------------------------------------------
 */

// compile: cc -Wall -DLINUX mikhail.c -o mikhail

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


#define  BUFFLEN        8192
#define  CONN_TIMEOUT   20	/* default timeout proxy-remotehost
				 * connection */
#define  VERSION        "2.5beta12"
#define  TRUE           1
#define  FALSE          0


int             mylisten(unsigned long, int);
void            fatal(char *, ...);
void            printlog(const char *format, ...);
int             get_new_connection(int);
int             connect_to_outsock(unsigned long, int);
int             connect_from_outsock(unsigned long, int);
void            sigdie(int);
unsigned long   getlongbyname(u_char *);
void            conn_timeout(int);
void            help();
void            init_opt(int, char **);
size_t          getmytime(char *, size_t, const char *);
char           *ipfromlong(unsigned long);
void            get_shost(char *);
void            get_dhost(char *);


typedef struct {

    /*
     * hosts 
     */

    unsigned long   dhost;	/* destination host, the remote host */
    unsigned long   shost;	/* source host, the proxy host */

    /*
     * ports 
     */

    int             dport;	/* destination host port */
    int             sport0;	/* listen source port 0 (with -i option) */
    int             sport1;	/* listen source port 1 */
    int             sport2;	/* listen source port 2 */
    int             sport;	/* port to bind for proxy-remotehost conn. 
				 */
    int             enable_sport;
    int             enable_sport2;

    /*
     * sockets 
     */

    int             listen_sport0;	/* listen source port 0 (with -i
					 * option) */
    int             listen_sport1;	/* listen source port 1 */
    int             listen_sport2;	/* listen source port 2 */

    int             outsock;
    int             insock1;	/* client1-sourcehost connection */
    int             insock2;	/* client2-sourcehost connection */
    int             insock;	/* insock == (insock1 || insock2) */

    /*
     * misc
     */

    int             timeout;
    int             client1_connected;	/* client1 connected? TRUE/FALSE */
    int             client2_connected;	/* client2 connected? TRUE/FALSE */
    int             background;
    int             logpackets;
    int             listen_outsock;
    FILE           *lfile;
    FILE           *pfile;

} OPT;


OPT             o;

sigjmp_buf      jmp_env;

int
main(int argc, char **argv)
{

    char            buf[BUFFLEN];	/* I/O buffer */
    fd_set          rx_set,	/* Read set */

                    wk_set;	/* Working set */
    int             mx,		/* max fd, used with selec() */

                    z,
                    s,		/* temp socket */

                    bytes;

    init_opt(argc, argv);

    if (o.background == TRUE)
	if (fork())
	    exit(0);

    if (o.background == TRUE)
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

    printlog("  .--- %s:%d\n", ipfromlong(o.shost), o.sport1);

    if (o.enable_sport2 == TRUE)
	printlog("  |--- %s:%d\n", ipfromlong(o.shost), o.sport2);

    if (o.listen_outsock == FALSE)
	printlog("  `--> %s:%d (timeout=%ds)\n", ipfromlong(o.dhost),
		 o.dport, o.timeout);
    else
	printlog("  `--> %s:%d <--- [remotehost] (timeout=%ds)\n",
		 ipfromlong(o.shost), o.sport0, o.timeout);

    printlog("\n");

    o.listen_sport1 = mylisten(o.shost, o.sport1);

    if (o.enable_sport2 == TRUE)
	o.listen_sport2 = mylisten(o.shost, o.sport2);

    FD_ZERO(&rx_set);
    FD_SET(o.listen_sport1, &rx_set);

    if (o.enable_sport2 == TRUE) {
	FD_SET(o.listen_sport2, &rx_set);
	mx = o.listen_sport2 + 1;	/* mx = max fd + 1 */
    } else
	mx = o.listen_sport1 + 1;	/* mx = max fd + 1 */

    o.client1_connected = FALSE;
    o.client2_connected = FALSE;

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

	if (FD_ISSET(o.listen_sport1, &wk_set)) {

	    /*
	     * client1-proxy connection request
	     */

	    /*
	     * In order to be notified of incoming connections on a
	     * socket, you can use select(2) or poll(2).  A readable event 
	     * will be delivered when a new connection is attempted and
	     * you may then call accept to get a socket for that con-
	     * nection. 
	     */

	    printlog("  client1-proxy connection request\n");

	    s = get_new_connection(o.listen_sport1);
	    if (s == -1) {
		printlog("  client1-proxy connection failed\n");
		continue;
	    }

	    if (o.client1_connected == TRUE) {
		printlog
		    ("  there's already a client1-proxy connection, closed\n");
		close(s);
		continue;
	    }

	    alarm(o.timeout);
	    if (o.listen_outsock == FALSE)
		z = connect_to_outsock(o.dhost, o.dport);
	    else {
		printlog
		    ("  Waiting for Connection from remotehost (timeout=%ds)\n",
		     o.timeout);
		z = connect_from_outsock(o.shost, o.sport0);
	    }
	    alarm(0);

	    if (o.listen_outsock == TRUE)
		close(o.listen_sport0);

	    if (z < 0) {
		printlog
		    ("  proxy-remotehost connection failed (%s)\n",
		     z == -1 ? "ECONNREFUSED" : "TIMEOUT");

		close(s);

		printlog("  client1-proxy connection closed\n");

		continue;
	    }

	    printlog("  proxy connected to remotehost\n");

	    o.insock1 = s;

	    FD_SET(o.outsock, &rx_set);
	    FD_SET(o.insock1, &rx_set);

	    /*
	     * o.outsock will always be more than o.insock
	     */

	    if (o.outsock + 1 > mx)
		mx = o.outsock + 1;

	    o.client1_connected = TRUE;

	    /*
	     * now client1-proxy-remotehost 
	     */

	    o.insock = o.insock1;
	    printlog("* Now client1-proxy-remotehost\n");

	    continue;
	}


	if (FD_ISSET(o.listen_sport2, &wk_set)) {

	    /*
	     * client2-proxy connection request
	     */

	    printlog("  client2-proxy connection request\n");

	    s = get_new_connection(o.listen_sport2);
	    if (s == -1) {
		printlog("  client2-proxy connection failed\n");
		continue;
	    }

	    if (o.client1_connected == FALSE) {
		printlog
		    ("  client1 ins't connected, client2 connection closed\n");
		close(s);
		continue;
	    }

	    if (o.client2_connected == TRUE) {
		printlog
		    ("  there's already a client2-proxy connection, closed\n");
		close(s);
		continue;
	    }

	    o.insock2 = s;

	    if (o.insock2 + 1 > mx)
		mx = o.insock2 + 1;

	    FD_CLR(o.insock1, &rx_set);
	    FD_SET(o.insock2, &rx_set);

	    if (o.insock2 + 1 > mx)
		mx = o.insock2 + 1;

	    o.client2_connected = TRUE;

	    /*
	     * now client2-proxy-remotehost
	     */

	    o.insock = o.insock2;
	    printlog("* Now client2-proxy-remotehost\n");

	    continue;
	}

	memset(buf, 0, BUFFLEN);

	if FD_ISSET
	    (o.insock, &wk_set) {
	    bytes = recv(o.insock, buf, sizeof(buf), 0);

	    if (o.logpackets == TRUE && bytes > 0) {
//		fprintf(o.pfile, "\n\n<send:%d>\n\n", bytes);
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
		 * if o.insock == o.insock1, client1 has 
		 * closed the connection. if o.insock ==
		 * o.insock2, client2 has closed the
		 * connection. 
		 */

		if (o.insock == o.insock1) {

		    printlog
			("* client1-proxy connection lost, closing all connections.\n");

		    FD_CLR(o.insock1, &rx_set);
		    FD_CLR(o.insock2, &rx_set);
		    FD_CLR(o.outsock, &rx_set);

		    if (o.client2_connected == TRUE) {
			close(o.insock2);
			o.client2_connected = FALSE;
		    }

		    close(o.insock1);
		    o.client1_connected = FALSE;

		    close(o.outsock);

		} else {	/* 
				 *  o.insock == o.insock2 
				 */

		    printlog("  client2-proxy connection lost\n");

		    FD_CLR(o.insock2, &rx_set);
		    FD_SET(o.insock1, &rx_set);

		    if (o.insock1 + 1 > mx)
			mx = o.insock1 + 1;

		    close(o.insock2);
		    o.client2_connected = FALSE;

		    /*
		     * now client1-proxy-remotehost
		     */

		    o.insock = o.insock1;
		    printlog("* Now client1-proxy-remotehost\n");

		}

		continue;
	    }

	    send(o.outsock, buf, bytes, 0);
	    }

	if FD_ISSET
	    (o.outsock, &wk_set) {
	    bytes = recv(o.outsock, buf, sizeof(buf), 0);

	    if (o.logpackets == TRUE && bytes > 0) {
//		fprintf(o.pfile, "\n\n<recv:%d>\n\n", bytes);
		fwrite(buf, 1, bytes, o.pfile);
		fflush(o.pfile);
	    }

	    if (bytes == 0) {

		FD_CLR(o.insock1, &rx_set);
		FD_CLR(o.insock2, &rx_set);
		FD_CLR(o.outsock, &rx_set);

		close(o.outsock);

		if (o.client2_connected == TRUE) {
		    close(o.insock2);
		    o.client2_connected = FALSE;
		}

		close(o.insock1);
		o.client1_connected = FALSE;

		printlog
		    ("* proxy-remotehost connection lost, closing all connections.\n");

		continue;
	    }
	    send(o.insock, buf, bytes, 0);
	    }
    }

    /*
     * Control never gets here 
     */

    exit(0);

}


int
connect_to_outsock(unsigned long host_addr, int port)
{

    struct sockaddr_in addr;
    int             z;

    o.outsock = socket(PF_INET, SOCK_STREAM, 0);
    if (o.outsock == -1)
	fatal("connect_to_outsock(): socket()");

    if (o.enable_sport == TRUE) {

	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(o.sport);

	addr.sin_addr.s_addr = INADDR_ANY;

	z = bind(o.outsock, (struct sockaddr *) &addr, sizeof addr);
	if (z == -1)
	    fatal("connect_to_outsock(): bind()");

    }

    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    addr.sin_addr.s_addr = host_addr;

    if (sigsetjmp(jmp_env, 1) == 0) {
	z = connect(o.outsock, (struct sockaddr *) &addr, sizeof addr);

	if (errno == ECONNREFUSED)
	    return -1;

	if (z == 0)
	    return 0;
    }

    return -2;

}


int
connect_from_outsock(unsigned long host_addr, int port)
{

    o.listen_sport0 = mylisten(host_addr, port);

    if (sigsetjmp(jmp_env, 1) == 0) {
	o.outsock = get_new_connection(o.listen_sport0);
	return 0;
    }

    return -2;

}


void
fatal(char *pattern, ...)
{

    va_list         ap;
    va_start(ap, pattern);

    vfprintf(o.lfile, pattern, ap);
    fprintf(o.lfile, "; exit forced.\n");

    va_end(ap);

    exit(-1);

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


int
mylisten(unsigned long host_addr, int port)
{

    int             z,
                    s,
                    len_inet;
    struct sockaddr_in addr_proxy;

    s = socket(PF_INET, SOCK_STREAM, 0);
    if (s == -1)
	fatal("mylisten(): socket()");

    memset(&addr_proxy, 0, sizeof addr_proxy);
    addr_proxy.sin_family = AF_INET;
    addr_proxy.sin_port = htons(port);
    addr_proxy.sin_addr.s_addr = host_addr;

#ifdef LINUX

    z = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &z, sizeof(z));

#endif

    len_inet = sizeof addr_proxy;
    z = bind(s, (struct sockaddr *) &addr_proxy, len_inet);
    if (z == -1)
	fatal("mylisten(): bind() port : %d", port);

    z = listen(s, 1);
    if (z == -1)
	fatal("mylisten(): listen()");

    return s;

}


int
get_new_connection(int s)
{

    int             len_inet;
    int             c;

    struct sockaddr_in addr_client;

    len_inet = sizeof addr_client;
    c = accept(s, (struct sockaddr *) &addr_client, &len_inet);

    /*
     * RETURN VALUE The call returns -1 on error.  If it succeeds, it
     * returns a non-negative integer that is a descriptor for the
     * accepted socket. 
     */

    if (c > 0)
	printlog("  connect from %s:%d\n", inet_ntoa(addr_client.sin_addr),
		 ntohs(addr_client.sin_port));

    return c;

}


void
sigdie(int signo)
{

    switch (signo) {
    case SIGURG:
	printlog("caught SIGURG signal, cleaning up\n");
	break;
    case SIGPIPE:
	printlog("caught SIGPIPE signal, cleaning up\n");
	break;
    case SIGQUIT:
	printlog("caught SIGQUIT signal, cleaning up\n");
	break;
    case SIGINT:
	printlog("caught SIGINT signal, cleaning up\n");
	break;
    case SIGTERM:
	printlog("caught SIGTERM signal, cleaning up\n");
	break;
    case SIGHUP:
	printlog("caught SIGHUP signal, cleaning up\n");
	break;
    case SIGSEGV:
	printlog("caught SIGSEGV signal, cleaning up\n");
	break;
    case SIGBUS:
	printlog("caught SIGBUS signal, cleaning up\n");
	break;
    default:
	printlog("caught signal %d, cleaning up\n", signo);
	break;
    }


    if (o.client1_connected || o.client2_connected)
	close(o.outsock);

    if (o.client1_connected)
	close(o.insock1);

    if (o.client2_connected) {
	close(o.insock1);
	close(o.insock2);
    }

    if (o.listen_outsock == TRUE)
	close(o.listen_sport0);

    if (o.enable_sport2 == TRUE)
	close(o.listen_sport2);

    close(o.listen_sport1);


    fclose(o.lfile);
    if (o.logpackets == TRUE)
	fclose(o.pfile);


    exit(1);

}


unsigned long
getlongbyname(u_char * host)
{

    struct in_addr  addr;
    struct hostent *host_ent;

    if (strcasecmp(host, "any") == 0)
	return INADDR_ANY;

    if ((addr.s_addr = inet_addr(host)) == -1) {

	if ((host_ent = gethostbyname(host)) == NULL) {
	    fatal("'%s': gethostbyname() or inet_addr() err: %s", host,
		  strerror(errno));
	}
	bcopy(host_ent->h_addr, (char *) &addr.s_addr, host_ent->h_length);
    }

    return addr.s_addr;

}


void
conn_timeout(int n)
{

    siglongjmp(jmp_env, 1);

}


void
help()
{
    printf("Mikhail v%s\n\n", VERSION);
    printf("USAGE: mikhail [options]\n\n");
    printf
	("-s shost|any:sport1[:sport2]        Source host and port(s) to listen on\n");
    printf
	("-c sport                            Bind local port for proxy-remotehost conn.\n");
    printf
	("-d dhost:dport                      Destination host and port to connect to\n");
    printf
	("-i port                             Listen Mode: wait for connect on port\n");
    printf
	("-t timeout(s)                       Set timeout for proxy-remotehost connection\n");
    printf("                                    default: %ds\n",
	   CONN_TIMEOUT);
    printf
	("-l filename                         Use file instead of stdout\n");
    printf("-p filename                         Log packets to file\n");
    printf("-b                                  Run in background\n");
    printf("-V                                  Display version number\n");
    printf("-h                                  This\n\n");
    exit(0);

}


void
init_opt(int argc, char **argv)
{
    int             c;

    o.shost = INADDR_ANY;
    o.dhost = INADDR_ANY;
    o.dport = 0;
    o.sport = 0;
    o.sport0 = 0;
    o.sport1 = 0;
    o.sport2 = 0;
    o.enable_sport = FALSE;
    o.enable_sport2 = FALSE;
    o.timeout = CONN_TIMEOUT;
    o.logpackets = FALSE;
    o.background = FALSE;
    o.listen_outsock = FALSE;
    o.lfile = stdout;

    while ((c = getopt(argc, argv, "s:c:d:l:p:t:i:bVh")) != EOF)
	switch (c) {

	case 's':
	    get_shost(optarg);
	    break;

	case 'd':
	    get_dhost(optarg);
	    break;

	case 't':
	    o.timeout = atoi(optarg);
	    break;

	case 'c':
	    o.enable_sport = TRUE;
	    o.sport = atoi(optarg);
	    break;

	case 'i':
	    o.listen_outsock = TRUE;
	    o.sport0 = atoi(optarg);
	    break;

	case 'b':
	    o.background = TRUE;
	    break;

	case 'l':
	    o.lfile = fopen(optarg, "w");
	    if (o.lfile == NULL) {
		o.lfile = stdout;
		fatal("unable to open log file");
	    }
	    break;

	case 'p':
	    o.pfile = fopen(optarg, "w");
	    if (o.pfile == NULL)
		fatal("unable to open pakets log file");
	    o.logpackets = TRUE;
	    break;

	case 'V':
	    printf("Mikhail v%s\n", VERSION);
	    exit(0);
	    break;

	case 'h':
	    help();
	    break;

	default:
	    fatal("try -h");

	}

    if (o.sport1 == 0)
	fatal("sport1 required");
    if (o.enable_sport2 == TRUE && o.sport2 == 0)
	fatal("sport2 required");

    if (o.listen_outsock == FALSE) {
	if (o.dport == 0)
	    fatal("dport required %s");
    } else {
	if (o.dhost != INADDR_ANY)
	    fatal("-i option excludes -d option");
	if (o.sport0 == 0)
	    fatal("sport0 required");
    }

}


void
get_shost(char *s)
{

    char           *p,
                   *p_old;

    /*
     * host:port1:port2 
     */

    for (p = s; *p != '\0' && *p != ':'; p++);

    if (*p == '\0')
	return;

    *p = '\0';

    o.shost = getlongbyname(s);

    p_old = ++p;

    while (*p != '\0' && *p != ':')
	p++;

    if (*p == '\0') {
	o.enable_sport2 = FALSE;
    } else
	o.enable_sport2 = TRUE;

    *p = '\0';

    o.sport1 = atoi(p_old);

    if (o.enable_sport2) {
	p++;
	o.sport2 = atoi(p);
    }

}


void
get_dhost(char *s)
{

    char           *p;

    /*
     * host:port 
     */

    for (p = s; *p != '\0' && *p != ':'; p++);

    if (*p == '\0')
	help();

    *p = '\0';

    o.dhost = getlongbyname(s);

    ++p;

    o.dport = atoi(p);

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


char
               *
ipfromlong(unsigned long s_addr)
{

    struct in_addr  myaddr;

    myaddr.s_addr = s_addr;
    return inet_ntoa(myaddr);

}


/*
 * EOF 
 */

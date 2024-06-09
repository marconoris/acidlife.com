/*
 *
 * $Id: c0n.c,v 1.0 2002/03/28 18:03:34 xenion Exp $
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

#define TIMEOUT 20
#define BUFFLEN 8192
#define LOGFILE "c0n.log"
#define VER "1.0"

sigjmp_buf      jmp_env;

int             connect_to(int *, unsigned long, int);
void            fatal(char *, ...);
unsigned long   getlongbyname(u_char *);
void            proxize(int, int);
void            conn_timeout(int);

FILE           *f;

int
main(int argc, char **argv)
{
    int             shost,
                    dhost;


    if (argc < 5) {
	printf
	    ("host To host Connect0r v%s (host1:port1 <---> host2:port)\n",
	     VER);
	printf("USAGE: c0n <host1> <port1> <host2> <port2>\n\n");
	exit(1);
    }

    if (fork())
	exit(1);

    if ((f = fopen(LOGFILE, "w")) == NULL) {
	f = stdout;
	fatal("fopen()");
    }

    signal(SIGALRM, conn_timeout);

    fprintf(f, "Connecting to %s:%d..\n", argv[1], atoi(argv[2]));
    alarm(TIMEOUT);
    if (connect_to(&shost, getlongbyname(argv[1]), atoi(argv[2])) < 0)
	fatal("unable to connect to source host");

    fprintf(f, "Connecting to %s:%d..\n", argv[3], atoi(argv[4]));
    alarm(TIMEOUT);
    if (connect_to(&dhost, getlongbyname(argv[3]), atoi(argv[4])) < 0)
	fatal("unable to connect to destination host");

    alarm(0);

    fprintf(f, "Working!\n");
    proxize(shost, dhost);
    fprintf(f, "Connections lost, exiting");

    close(shost);
    close(dhost);
    fclose(f);

    return 0;

}


int
connect_to(int *s, unsigned long host_addr, int port)
{

    struct sockaddr_in addr;
    int             z;

    *s = socket(PF_INET, SOCK_STREAM, 0);
    if (*s == -1)
	fatal("connect_to(): socket()");

    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    addr.sin_addr.s_addr = host_addr;

    if (sigsetjmp(jmp_env, 1) == 0) {
	z = connect(*s, (struct sockaddr *) &addr, sizeof addr);

	if (errno == ECONNREFUSED)
	    return -1;

	if (z == 0)
	    return 0;
    }

    return -2;

}


void
fatal(char *pattern, ...)
{

    va_list         ap;
    va_start(ap, pattern);

    vfprintf(f, pattern, ap);
    fprintf(f, "; exit forced.\n");

    fclose(f);

    va_end(ap);

    exit(-1);

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
proxize(int shost, int dhost)
{

    char            buf[BUFFLEN];
    int             bytes,
                    mx;
    fd_set          local_fdset;

    mx = (shost > dhost ? shost : dhost) + 1;

    bytes = 1;

    while (bytes) {

	FD_ZERO(&local_fdset);
	FD_SET(shost, &local_fdset);
	FD_SET(dhost, &local_fdset);

	bytes = select(mx, &local_fdset, NULL, NULL, NULL);

	if ((bytes == -1) && (errno == EINTR))
	    continue;

	memset(buf, 0, BUFFLEN);

	if FD_ISSET
	    (shost, &local_fdset) {
	    bytes = recv(shost, buf, sizeof(buf), 0);
	    send(dhost, buf, bytes, 0);
	    }


	if FD_ISSET
	    (dhost, &local_fdset) {
	    bytes = recv(dhost, buf, sizeof(buf), 0);
	    send(shost, buf, bytes, 0);
	    }
    }


}


void
conn_timeout(int n)
{

    fatal("Connection failed");
    exit(0);

}

// EOF

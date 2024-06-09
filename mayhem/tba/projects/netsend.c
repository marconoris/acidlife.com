/*
 * 
 * $Id: netsend.c,v 3.0beta1 2002/02/16 21:02:19 xenion Exp $
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

/*
 * compile: cc -Wall -o netsend netsend.c 
 * Send bug reports to xenion@acidlife.com, thx
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
#include <sys/stat.h>


#define  BUFFLEN        8192
#define  CONN_TIMEOUT   20
#define  CONN_SLEEP     0
#define  VERSION        "3.0beta1"
#define  TEMPFILE       "ech.tmp"
#define  ECHOLEN        100


void            fatal(char *, ...);
void            fconnect_to(unsigned long, int, FILE ** rx, FILE ** tx);
void            sigdie(int);
unsigned long   getlongbyname(u_char *);
void            conn_timeout(int);
void            help();
void            init_opt(int, char **);
off_t           filelen(char *);
void            update_status(off_t, off_t);
void            echize(char *, char *);
char           *ipfromlong(unsigned long);


typedef struct {

    unsigned long   host;	/* host */
    short           port;	/* port */
    int             s;		/* socket */
    off_t           count;
    off_t           f_len;
    FILE           *s_rx;
    FILE           *s_tx;
    FILE           *f_in;
    char           *f_in_name;
    int             timeout;
    int             echolen;

} OPT;


OPT             o;
char            buf[BUFFLEN];


int
main(int argc, char **argv)
{

    signal(SIGTERM, sigdie);
    signal(SIGINT, sigdie);
    signal(SIGQUIT, sigdie);
    signal(SIGHUP, sigdie);
    signal(SIGSEGV, sigdie);
    signal(SIGPIPE, sigdie);
    signal(SIGURG, SIG_IGN);
    signal(SIGALRM, conn_timeout);

    init_opt(argc, argv);

    printf("Creating %s (bytes per echo: %d) from %s (%ld Kb)\n",
	   TEMPFILE, o.echolen, o.f_in_name, filelen(o.f_in_name) / 1024);

    echize(o.f_in_name, TEMPFILE);
    free(o.f_in_name);

    o.f_in = fopen(TEMPFILE, "r");
    if (o.f_in == NULL)
	fatal("unable to open %s", TEMPFILE);

    printf("\nConnecting to %s:%d (timeout=%ds)\n", ipfromlong(o.host),
	   o.port, o.timeout);

    fconnect_to(o.host, o.port, &o.s_rx, &o.s_tx);

    o.f_len = filelen(TEMPFILE);
    o.count = 0;

    printf("Sending file (%ld Kb)\n", o.f_len / 1024);

    while (!feof(o.f_in)) {

	if (fgets(buf, sizeof(buf), o.f_in) == NULL)
	    continue;
	o.count += strlen(buf);
	update_status(o.count, o.f_len);
	fputs(buf, o.s_tx);
	fgets(buf, sizeof(buf), o.s_rx);

    }

    fclose(o.f_in);

    printf("\nFile Uploaded :)\n\n");

    exit(0);
}

void
fconnect_to(unsigned long host_addr, int port, FILE ** rx, FILE ** tx)
{

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    addr.sin_addr.s_addr = host_addr;

    o.s = socket(PF_INET, SOCK_STREAM, 0);
    if (o.s == -1)
	fatal("connect_to(): socket()");

    alarm(o.timeout);
    if ((connect(o.s, (struct sockaddr *) &addr, sizeof addr)) < 0)
	conn_timeout(0);
    alarm(0);

    *rx = fdopen(o.s, "r");
    if (*rx == NULL)
	fatal("fdopen(): rx");

    *tx = fdopen(dup(o.s), "w");
    if (*tx == NULL)
	fatal("fdopen(): tx");

    setlinebuf(*rx);
    setlinebuf(*tx);

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

    switch (signo) {
    case SIGURG:
	fprintf(stderr, "\ncaught SIGURG signal, cleaning up\n");
	break;
    case SIGPIPE:
	fprintf(stderr, "\ncaught SIGPIPE signal, cleaning up\n");
	break;
    case SIGQUIT:
	fprintf(stderr, "\ncaught SIGQUIT signal, cleaning up\n");
	break;
    case SIGINT:
	fprintf(stderr, "\ncaught SIGINT signal, cleaning up\n");
	break;
    case SIGTERM:
	fprintf(stderr, "\ncaught SIGTERM signal, cleaning up\n");
	break;
    case SIGHUP:
	fprintf(stderr, "\ncaught SIGHUP signal, cleaning up\n");
	break;
    case SIGSEGV:
	fprintf(stderr, "\ncaught SIGSEGV signal, cleaning up\n");
	break;
    case SIGBUS:
	fprintf(stderr, "\ncaught SIGBUS signal, cleaning up\n");
	break;
    default:
	fprintf(stderr, "\ncaught signal %d, cleaning up\n", signo);
	break;
    }

    fflush(stdout);
    close(o.s);
    if (o.f_in_name != NULL)
	free(o.f_in_name);
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

    fatal("Connection failed");
    exit(0);

}


void
help()
{

    printf("Netsend v%s by xenion@acidlife.com\n", VERSION);
    printf("USAGE: netsend [options]\n\n");
    printf("-d host                             Destination host\n");
    printf("-p port                             Destination port\n");
    printf("-l n                                Bytes per echo\n");
    printf
	("-t timeout                          Connection timeout (default: %ds)\n",
	 CONN_TIMEOUT);
    printf("-i filename                         Input file\n");
    printf("-V                                  Display version number\n");
    printf("-h                                  This\n\n");

    exit(0);

}


void
init_opt(int argc, char **argv)
{

    int             c;

    o.host = INADDR_ANY;
    o.port = 0;
    o.timeout = CONN_TIMEOUT;
    o.echolen = ECHOLEN;
    o.f_in_name = NULL;
    unlink(TEMPFILE);

    while ((c = getopt(argc, argv, "d:p:t:i:l:Vh")) != EOF)
	switch (c) {

	case 'd':
	    o.host = getlongbyname(optarg);
	    break;

	case 'p':
	    o.port = atoi(optarg);
	    break;

	case 'i':
	    if (o.f_in_name == NULL)
		o.f_in_name = strdup(optarg);
	    break;

	case 't':
	    o.timeout = atoi(optarg);
	    break;

	case 'l':
	    o.echolen = atoi(optarg);
	    break;

	case 'V':
	    printf
		("Netsend v%s by xenion@acidlife.com ~ Dallachiesa Michele\n",
		 VERSION);
	    exit(0);
	    break;

	case 'h':
	    help();
	    break;

	default:
	    fatal("try -h");

	}


    if (o.host == INADDR_ANY)
	fatal("host required");

    if (o.port == 0)
	fatal("port required");

    if (o.f_in_name == NULL)
	fatal("input file required");

    if (o.echolen > sizeof(buf) || o.echolen < 1)
	fatal("0 < Echo bytes < %d", sizeof(buf));

}

off_t
filelen(char *pathname)
{

    struct stat     buf;

    if (stat(pathname, &buf) == -1)
	fatal("stat()");

    return buf.st_size;

}

void
update_status(off_t now, off_t end)
{
    printf("\rStatus: [%ld/%ld] %ld%%", now, end, now * 100 / end);
    fflush(stdout);
}

void
echize(char *in, char *out)
{
    FILE           *f_in,
                   *f_out;
    int             len,
                    i;

    f_in = fopen(in, "r");
    if (f_in == NULL)
	fatal("unable to open %s", in);
    f_out = fopen(out, "w");
    if (f_out == NULL)
	fatal("unable to open %s", out);

    o.f_len = filelen(in);
    o.count = 0;

    while (!feof(f_in)) {
	len = fread(buf, 1, o.echolen, f_in);
	o.count += len;
	fprintf(f_out, "echo -ne \"");
	for (i = 0; i < len; i++)
	    fprintf(f_out, "\\%o", (unsigned char) buf[i]);
	fprintf(f_out, "\" >> %s%c", in, '\n');
	update_status(o.count, o.f_len);
    }

    fclose(f_in);
    fclose(f_out);

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

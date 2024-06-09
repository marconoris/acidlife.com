/*
 *
 * $Id: sost.c,v 1.0 2002/01/17 19:03:10 xenion Exp $
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
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#define VERSION "1.0"

void            fatal(char *, ...);
void            usage();
off_t           filelen(char *);


int
main(int argc, char **argv)
{
    FILE           *f;
    off_t           size,
                    i;
    int             len_old,
                    len_new,
                    z;
    char           *buf;

    if (argc < 4)
	usage();

    len_old = strlen(argv[2]) + 1;
    len_new = strlen(argv[3]) + 1;

    if (len_new > len_old)
	fatal("length new string > length old string");

    if ((buf = alloca(len_old)) == NULL)
	fatal("alloca()");

    if ((f = fopen(argv[1], "r+")) == NULL)
	fatal("unable to open file");

    size = filelen(argv[1]);

    printf("scanning file..\n");

    for (i = 0; i < size; ++i) {
	fseek(f, i, SEEK_SET);
	z = fread(buf, 1, len_old, f);
	if (z < len_old)
	    break;
	if (memcmp(buf, argv[2], len_old) == 0) {
	    printf("FOUND\n");
	    fseek(f, i, SEEK_SET);
	    z = fwrite(argv[3], 1, len_new, f);
	}
    }

    printf("done\n");

    fclose(f);
    return 0;

}


void
fatal(char *pattern, ...)
{

    va_list         ap;
    va_start(ap, pattern);

    printf(pattern, ap);
    printf("; exit forced.\n");

    va_end(ap);

    exit(-1);

}


void
usage()
{

    printf("Sost v%s\n\n", VERSION);
    printf("usage: sost <filename> <old_string> <new_string>\n\n");
    exit(0);

}


off_t
filelen(char *pathname)
{

    struct stat     buf;

    if (stat(pathname, &buf) == -1)
	fatal("stat()");

    return buf.st_size;

}

// EOF

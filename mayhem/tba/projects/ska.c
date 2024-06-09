/*
 *
 * $Id: ska.c,v 1.0 2002/05/12 16:05:32 xenion Exp $
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

#define VER "1.0"
unsigned char  *echoe(unsigned char *);


int
main(int argc, char **argv)
{
    int             i,
                    j;
    char           *matrix = NULL;

    if (argc < 3) {
	printf
	    ("ska v%s - string obfuscation utility, usueful with ELF encryption utilz\n",
	     VER);
	printf("USAGE: %s NAME [STRING]...\n", argv[0]);
	printf("(STRING(s) are interpreted like echo -e [STRING])\n\n");
	exit(1);
    }

    matrix = argv[1];
    argv++;
    argc--;

    printf("/*\n");
    printf("** generated automatically by ska %s\n**\n", VER);
    printf
	("** --[ LIST ]----------------------------------------------------\n");
    printf("**\n");
    for (i = 1; i < argc; i++)
	printf("** %s[%d]: \"%s\"\n", matrix, i - 1, argv[i]);
    printf("**\n");
    printf
	("** --------------------------------------------------------------\n");
    printf("*/\n\n");

    for (i = 1; i < argc; i++)
	echoe(argv[i]);

    printf("unsigned char *%s[%d];\n\n", matrix, argc);

    for (i = 1; i < argc; i++)
	printf("%s[%d]=alloca(%d);\n", matrix, i - 1, strlen(argv[i]) + 1);

    printf("\n");

    for (i = 1; i < argc && !(j = 0); i++)
	for (j = 0; j <= strlen(argv[i]); j++)
	    printf("%s[%d][%d]= %3d;\n", matrix, i - 1, j, argv[i][j]);

    printf("%s[%d]= NULL;\n\n", matrix, argc - 1);

    printf
	("/*\n** --------------------------------------------------------------\n*/\n\n");

    return 0;
}


unsigned char  *
echoe(unsigned char *s)
{
    unsigned char   c;
    unsigned char  *p = s;

    while ((c = *s++)) {

	if (c == '\\' && *s) {

	    switch (c = *s++) {

	    case 'a':
		c = '\007';
		break;

	    case 'b':
		c = '\b';
		break;

	    case 'f':
		c = '\f';
		break;

	    case 'n':
		c = '\n';
		break;

	    case 'r':
		c = '\r';
		break;

	    case 't':
		c = '\t';
		break;

	    case 'v':
		c = (int) 0x0B;
		break;

	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
		c -= '0';
		if (*s >= '0' && *s <= '7')
		    c = c * 8 + (*s++ - '0');
		if (*s >= '0' && *s <= '7')
		    c = c * 8 + (*s++ - '0');
		break;

	    case '\\':
		break;

	    }

	}

	*p++ = c;

    }

    *p = '\0';
    return s;

}


/*
 * EOF 
 */

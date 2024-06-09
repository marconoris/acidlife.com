/*
 *
 * $Id: 1337WRITE.c,v 1.0 2002/06/03 18:06:44 xenion Exp $
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

// source:

__asm__ volatile("
        jmp TheCall
ThePop:
        popl %ecx
        xorl %edx, %edx
loop:
        movb (%ecx, %edx, 1), %al
        testb %al, %al
        jz end
        incl %edx
        jmp loop
end:
        movl $0x4, %eax
        movl $0x1, %ebx
        int    $0x80
        movl $0x1, %eax
        xorl %ebx, %ebx
        int    $0x80
TheCall:
        call ThePop
        .string \"ya\"
");

*/


#define THESTR "working fine..;)\n"
#define SHELLCODE \
  "\xeb\x22\x59\x31\xd2\x8a" \
  "\x04\x11\x84\xc0\x74\x03" \
  "\x42\xeb\xf6\xb8\x04\x00" \
  "\x00\x00\xbb\x01\x00\x00" \
  "\x00\xcd\x80\xb8\x01\x00" \
  "\x00\x00\x31\xdb\xcd\x80" \
  "\xe8\xd9\xff\xff\xff" THESTR 

int
main(int argc, char **argv)
{
    int            *ret;
    ret = (int *) &ret + 2;
   *ret = (int) SHELLCODE;

    return 0;
}


//EOF

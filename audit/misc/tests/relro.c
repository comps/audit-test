/*
 * Test to exercise PIE and RELRO provided by Roland McGrath <roland@redhat.com>.
 *
 * Description:
 *   Simple test for RELRO, which happens to be a PIE too, but that's only
 *   because this kind of example has to be in PIC code to make RELRO relevant,
 *   and PIE makes it simpler to write a standalone one-file test than writing
 *   a DSO.
 *
 *   The "const" makes "foo" .rodata material, and the init to an external symbol
 *   reference makes it require a data relocation.  Enabling -z relro for this
 *   link puts that .rodata into a RELRO area.  This program will crash because
 *   the page containing "foo" has been made read-only when "main" runs.
 *   Without RELRO, it would let you modify "foo" even though it's supposed to
 *   be const.
 *
 * Test with RELRO should fail:
 *   $ gcc -pie -fPIE -g  -Wl,-z,relro -o relro relro.c
 *   $ ./relro
 *   Segmentation fault (core dumped)
 *
 * Test without RELRO should pass:
 *   $ gcc -pie -fPIE -g -o no-relro relro.c
 *   $ ./no-relro
 *
**/


#include <stdio.h>

void *const foo = &stdout;

int main (void)
{
  *(void **) &foo = &stderr;
  return 0;
}

/* access1.c
 *	test access pattern
 */

#include "syscall.h"
#include "stdlib.h"
/* 
 * This is a hack to give us access to the physical page size without having
 * to include large numbers of files from the rest of Nachos.
 */
#include "phys.h"

#define Dim 	1024

char A[PageSize][Dim];

int
main()
{
    int i, j;

    for (i = 0; i < PageSize; i+=8) {
        for (j = 0; j < Dim; j++) {
            A[i][j] = 1;
        }
    }

    Exit(0);
}

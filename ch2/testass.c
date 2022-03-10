#include "ec440threads.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <stdio.h>


int main() {
        jmp_buf dammit;
        long unsigned int newval;
    pthread_t ID = 5;
    if (setjmp( dammit)== 0) {
        printf("Set the jump\n");
        printf("%lu \n",dammit->__jmpbuf[7]);
        newval = ptr_demangle(dammit->__jmpbuf[7]);
        printf("%lu \n",newval);
        newval = ptr_mangle(newval);
        printf("%lu \n",newval);
        longjmp(dammit, 1);
    } else {
        printf("Jumped\n");
    }
    return 0;
}
#include <stdio.h>
#include "threads.c"
#include "ec440threads.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <pthread.h>

void my_thread(void* my_arg) {
    printf("Hello World\n");
    
    // Do something in this thread
//    if (i_need_to_exit_now) {
//        pthread_exit(NULL); // Can exit with another value
//    }
    // Maybe do some more things in this thread
    // Suggestion: do something that takes a long time
     // Can return other values
}

int main() {
//    int *Stack_Bottom;
//    Stack_Bottom = (int *) malloc(16);
//    void *Stack_Pointer;
//    Stack_Pointer = (void *) ((int) Stack_Bottom);
//    Stack_Pointer = (void *) ((16 + (int) (Stack_Bottom)));
//    Stack_Pointer = (void *) ((16 + (int) (Stack_Bottom))&-16);
//    return 0;
//    pthread_t tid;
//    int error_number = pthread_create(
//            &tid, NULL, my_thread, NULL /*my_arg*/);
//    pthread_t my_tid = pthread_self();
    // my_tid must not equal tid (a different thread)
    // jmp_buf dammit;
    // pthread_t ID = 5;
    // if (setjmp( dammit)== 0) {
    //     printf("Set the jump\n");
    //     longjmp(dammit, 1);
    // } else {
    //     printf("Jumped\n");
    // }
    // return 0;
   int val = pthread_create(5, NULL, my_thread, NULL);

    // Suggestion: do something that takes a long time, and test that
    // both this and the other long work get to take turns.
//    jmp_buf env;
//    int val = setjmp(env);


    return 0;
}
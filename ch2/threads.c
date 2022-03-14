#include "ec440threads.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>


/* You can support more threads. At least support this many. */
#define MAX_THREADS 128
/* Signal statuses */
#define THREAD_CREATED 1
#define THREAD_EXITED 2
#define ALARM_TRIGGERED 3
#define THREAD_JUMPED 4
/* Your stack should be this many bytes in size */
#define THREAD_STACK_SIZE 32767

#define MAIN_THREAD 1
#define SUCCESSFUL_EXECUTION 1
#define ERROR_INITIALIZATION (-2)
#define SHOULDNOTREACHHERE (-3)

#define STACK_ALIGNMENT 16

/* Number of microseconds between scheduling events */
#define SCHEDULER_INTERVAL_USECS (50 * 1000)

/* Extracted from private libc headers. These are not part of the public
 * interface for jmp_buf.
 */
#define JB_RBX 0
#define JB_RBP 1
#define JB_R12 2
#define JB_R13 3
#define JB_R14 4
#define JB_R15 5
#define JB_RSP 6
#define JB_PC 7

enum thread_status
{
    TS_EXITED,
    TS_RUNNING,
    TS_READY
};

typedef struct thread_control_block {
    int thread_ID;
    enum thread_status TS;
    jmp_buf Environment;
    void *stack_bottom;
    struct thread_control_block *next;
} TCB;

typedef struct thread_control_block *TCB_ptr;
typedef TCB_ptr thread_t;

static struct thread_control_block *run_queue;
static struct thread_control_block *last_thread;
static int* thread_counter = (int*) MAIN_THREAD;
static int tc = MAIN_THREAD;

static void schedule(int signal) __attribute__((unused));

static int linked_thread_init(void) {

    int *stack;
    run_queue = (thread_t) malloc(sizeof(TCB));
    stack = (int *) malloc(THREAD_STACK_SIZE);
    // if (setjmp(run_queue->Environment) == 0)
     {
        run_queue->thread_ID = MAIN_THREAD;
        run_queue->stack_bottom = stack;
        run_queue->next = run_queue;
        run_queue->TS = TS_RUNNING;
        last_thread = run_queue;
        // printf("Main thread created\n");
        return SUCCESSFUL_EXECUTION;
    } 
    // else {
    //     printf("Long jump to main tread init function\n");
    //     return THREAD_JUMPED;
    // }
}

// static int stack_grows_down (void *first_address) {
//     int second_address;
//     return first_address > (void *) &second_address;
// }

int thread_exit (void) {
    printf("thread_exit()\n");
    // thread_t temp;
    if(run_queue->thread_ID == MAIN_THREAD) {
        return SHOULDNOTREACHHERE;
    }
    run_queue->TS = TS_EXITED;
    free(run_queue->stack_bottom);
    run_queue = run_queue->next;
    free(last_thread->next);
    last_thread->next = run_queue;
    return SUCCESSFUL_EXECUTION;
//    schedule(THREAD_EXITED);
}

// static void print_mem(void* addr, int count) {
//     unsigned char* mem = (unsigned char*) addr;
//     for(int i = 0; i < count; i++) {
//         printf("%x ", mem[i]);
//     }
//     printf("\n");
// }

// static volatile void thread_wrapper(void) {
//     (*run_queue->Entry) (run_queue->Argc, run_queue->Argv);
//     thread_exit(); //TODO MAKE THREAD EXIT SEPARATE FROM PTHREADEXIT
// }

// void thread_init (volatile TCB *volatile new_TCB, void *stack_pointer) {
//     // printf("Stack_pointer in thread_init = %u\n", (int)stack_pointer);
//     // new_TCB->Environment->__jmpbuf[JB_RSP] = (int) stack_pointer;
//     // new_TCB->Environment->__jmpbuf[JB_RBP] = (int) stack_pointer;

//     printf("Thread_wrapper address = %lu, ptr_mangle = %lu\n", 
//         (unsigned long int)thread_wrapper,
//         (long) ptr_mangle((unsigned long int)thread_wrapper));
//     new_TCB->Environment->__jmpbuf[JB_PC] = ptr_mangle((unsigned long int) thread_wrapper);

//    new_TCB->Environment->__jmpbuf[JB_RSP] = (int) stack_pointer;
//    new_TCB->Environment->__jmpbuf[JB_RBP] = (int) stack_pointer;
//    new_TCB->Environment->__jmpbuf[JB_PC] = (int) thread_wrapper;
// }

void add_thread_to_queue(thread_t new_TCB) {
    last_thread->next = new_TCB;
    new_TCB->next = run_queue;
    last_thread = new_TCB;
}

void pthread_exit(void *value_ptr)
{
    // printf("Thread exited %d \n", run_queue->thread_ID);
    run_queue->TS = TS_EXITED;
    free(run_queue->stack_bottom);

    run_queue = run_queue->next;
    free(last_thread->next);
    last_thread->next = run_queue;
    // printf("mem cleared\n");
    schedule(THREAD_EXITED);

    /* TODO: Exit the current thread instead of exiting the entire process.
     * Hints:
     * - Release all resources for the current thread. CAREFUL though.
     *   If you free() the currently-in-use stack then do something like
     *   call a function or add/remove variables from the stack, bad things
     *   can happen.
     * - Update the thread's status to indicate that it has exited
     */

    exit(0);
}

int thread_create (pthread_t *thread, const pthread_attr_t *attr,
        void *(*start_routine) (void *), void *arg) {
    thread_t new_TCB;
    char *stack_bottom;
    // long first_address;
    // long *stack_pointer;

    new_TCB = (thread_t) malloc(sizeof(TCB));
    if(new_TCB == NULL) { return ERROR_INITIALIZATION; }
    stack_bottom = (char*) calloc(THREAD_STACK_SIZE, 1);

    for(int i = 0; i < THREAD_STACK_SIZE; i++) {
        stack_bottom[i] = -1;
    }

    // stack_pointer = (long *) (stack_grows_down(&first_address))?(THREAD_STACK_SIZE + stack_bottom):stack_bottom;
    
    unsigned long* dest = (unsigned long*) (THREAD_STACK_SIZE + stack_bottom - 8);

    if (setjmp(new_TCB->Environment) == 0) {

        // printf("        dest = %x\n", dest);
        // printf("mangled dest = %x\n", ptr_mangle(dest));

        unsigned long exit_addr = (unsigned long) &pthread_exit;


        *(unsigned long*) dest =  (unsigned long)exit_addr;

        // print_mem(dest, 8);

        new_TCB->Environment->__jmpbuf[JB_PC] = ptr_mangle((unsigned long int)start_thunk);
        new_TCB->Environment->__jmpbuf[JB_R12] = (unsigned long int) start_routine;
        new_TCB->Environment->__jmpbuf[JB_R13] = (unsigned long int) arg;
        new_TCB->Environment->__jmpbuf[JB_RSP] = ptr_mangle((unsigned long) dest);


        new_TCB->thread_ID = tc;
      
        new_TCB->stack_bottom = stack_bottom;
        new_TCB->TS = TS_READY;
        add_thread_to_queue(new_TCB);
        return SUCCESSFUL_EXECUTION;
    } else {
        // printf("Thread jumped\n");
        return THREAD_JUMPED;
    }
}


static void schedule(int signal)
{
    switch(run_queue->TS) {
        case TS_RUNNING: ;

            int val = setjmp(run_queue->Environment);
            if (val != 0) {
                break;
            }
            run_queue->TS = TS_READY;
            run_queue = run_queue->next;

            last_thread = last_thread->next;
            run_queue->TS = TS_RUNNING;
            // printf("TS_RUNNING: Jump to thread_ID %d\n", run_queue->thread_ID);
            longjmp(run_queue->Environment, (int) run_queue->thread_ID);
        case TS_EXITED: ;
            void *karl = (void*) 1;
            pthread_exit(karl);
        case TS_READY:
            // printf("TS_READY: Jump to thread_ID %d\n", run_queue->thread_ID);
            run_queue->TS = TS_RUNNING;
            longjmp(run_queue->Environment, (int) run_queue->thread_ID);
        default:
            perror("ERROR: Thread state could not be defined");
    }

    /* TODO: implement your round-robin scheduler
     * 1. Use setjmp() to update your currently-active thread's jmp_buf
     *    You DON'T need to manually modify registers here.
     * 2. Determine which is the next thread that should run
     * 3. Switch to the next thread (use longjmp on that thread's jmp_buf)
     */
}
/*void alarm_timer(int sig)
{
    signal(SIGALRM, SIG_IGN);
    // printf("Alarm triggered!\n");
    ualarm(SCHEDULER_INTERVAL_USECS,0);
    // signal(SIGALRM, alarm_timer);
    // ualarm(SCHEDULER_INTERVAL_USECS,0);
    schedule(ALARM_TRIGGERED);

}

static void scheduler_init()
{
    struct sigaction sact;
    sigemptyset( &sact.sa_mask );
    sact.sa_flags = SA_NODEFER;
    sact.sa_handler = alarm_timer;
    sigaction( SIGALRM, &sact, NULL );
    ualarm(SCHEDULER_INTERVAL_USECS,0);
}*/

void alarm_timer(int sig)
{
    signal(SIGALRM, SIG_IGN);
    alarm(0);
    signal(SIGALRM, alarm_timer);
    alarm(SCHEDULER_INTERVAL_USECS);
    schedule(ALARM_TRIGGERED);

}

static void scheduler_init()
{
    signal(SIGALRM, alarm_timer);
    alarm(SCHEDULER_INTERVAL_USECS);
}

int pthread_create(
        pthread_t *thread, const pthread_attr_t *attr,
        void *(*start_routine) (void *), void *arg)
{
    // Create the timer and handler for the scheduler. Create thread 0.
    static bool is_first_call = true;
    int val;
    // int Argc = 0;
    int value_init = 0;
    if (is_first_call)
    {
        is_first_call = false;
        value_init = linked_thread_init();
        if (value_init == SUCCESSFUL_EXECUTION) {
            scheduler_init();
        }
        // if(linked_thread_init() != SUCCESSFUL_EXECUTION) {
        //     perror("ERROR: Could not initialize main thread");
        // }
        
    }

    if (value_init != THREAD_JUMPED) {
        // if(arg != NULL) {
        //     // int Argc = 1;
        // }
        thread_counter++;
        tc++;
        thread = (pthread_t*) thread_counter;

        val = thread_create(thread, attr, start_routine, arg);
        if (val == SUCCESSFUL_EXECUTION){
            schedule(THREAD_CREATED);
            return 0;
        } else {
            return 0;
        }
    }
    // printf("Main Thread Jumped %d\n", value_init);
    return 0;

    /* TODO: Return 0 on successful thread creation, non-zero for an error.
     *       Be sure to set *thread on success.
     * Hints:
     * The general purpose is to create a TCB:
     * - Create a stack.
     * - Assign the stack pointer in the thread's registers. Important: where
     *   within the stack should the stack pointer be? It may help to draw
     *   an empty stack diagram to answer that question.
     * - Assign the program counter in the thread's registers.
     * - Wait... HOW can you assign registers of that new stack?
     *   1. call setjmp() to initialize a jmp_buf with your current thread
     *   2. modify the internal data in that jmp_buf to create a new thread environment
     *      env->__jmpbuf[JB_...] = ...
     *      See the additional note about registers below
     *   3. Later, when your scheduler runs, it will longjmp using your
     *      modified thread environment, which will apply all the changes
     *      you made here.
     * - Remember to set your new thread as TS_READY, but only  after you
     *   have initialized everything for the new thread.
     * - Optionally: run your scheduler immediately (can also wait for the
     *   next scheduling event).
     */
    /*
     * Setting registers for a new thread:
     * When creating a new thread that will begin in start_routine, we
     * also need to ensure that `arg` is passed to the start_routine.
     * We cannot simply store `arg` in a register and set PC=start_routine.
     * This is because the AMD64 calling convention keeps the first arg in
     * the EDI register, which is not a register we control in jmp_buf.
     * We provide a start_thunk function that copies R13 to RDI then jumps
     * to R12, effectively calling function_at_R12(value_in_R13). So
     * you can call your start routine with the given argument by setting
     * your new thread's PC to be ptr_mangle(start_thunk), and properly
     * assigning R12 and R13.
     *
     * Don't forget to assign RSP too! Functions know where to
     * return after they finish based on the calling convention (AMD64 in
     * our case). The address to return to after finishing start_routine
     * should be the first thing you push on your stack.
     */
    return -1;
}



pthread_t pthread_self(void)
{
    //return run_queue->thread_ID;/* TODO: Return the current thread instead of -1
     // * Hint: this function can be implemented in one line, by returning
     // * a specific variable instead of -1.
     // */
     return -1;
}

/* Don't implement main in this file!
 * This is a library of functions, not an executable program. If you
 * want to run the functions in this file, create separate test programs
 * that have their own main functions.
 */


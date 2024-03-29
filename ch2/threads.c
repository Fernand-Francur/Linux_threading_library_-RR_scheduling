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
#define MAX_MUTBAR 128
/* Signal statuses */
#define THREAD_CREATED 1
#define THREAD_EXITED 2
#define ALARM_TRIGGERED 3
#define THREAD_JUMPED 4
#define THREAD_BLOCKED 13
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
    TS_READY,
    TS_BLOCKED
};

typedef struct thread_control_block {
    int thread_ID;
    enum thread_status TS;
    jmp_buf Environment;
    void *stack_bottom;
    struct thread_control_block *next;
} TCB;

typedef struct blocked_threads {
  pthread_mutex_t *mutex_ID;
  int* blocked_thread_list;
  bool is_used;
  struct blocked_threads *next;
} BT;

typedef struct barrier_blocked {
  //pthread_barrier_t *barrier_ID;
  int* blocked_thread_list;
  int count;
  int thread_num;
  // struct barrier_blocked *next;
} BB;

typedef struct thread_control_block *TCB_ptr;
typedef TCB_ptr thread_t;

static struct blocked_threads *blocked_first;
// static struct barrier_blocked *first_barrier;
static struct thread_control_block *run_queue;
static struct thread_control_block *last_thread;
static int* thread_counter = (int*) MAIN_THREAD;
static int tc = MAIN_THREAD;
static int* unblock_array = NULL;
static bool ignore = false;
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
	//blocked_first = (BT *) malloc(sizeof(BT));
	unblock_array = (int *) calloc(MAX_MUTBAR, 4);

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
        case TS_BLOCKED: ;
	  if (signal == THREAD_BLOCKED) {
	    int val = setjmp(run_queue->Environment);
            if (val != 0) {
	      break;
	    }
	  } 
	  while (run_queue->TS != TS_READY) {
	    run_queue = run_queue->next;
	    last_thread = last_thread->next;
	  }
	  run_queue->TS = TS_RUNNING;
	  longjmp(run_queue->Environment, (int) run_queue->thread_ID);
        case TS_RUNNING: ;

            int val = setjmp(run_queue->Environment);
            if (val != 0) {
                break;
            }
            run_queue->TS = TS_READY;
	    run_queue = run_queue->next;
	    last_thread = last_thread->next;
	    while (run_queue->TS != TS_READY) {
	      run_queue = run_queue->next;
	      last_thread = last_thread->next;
	    }
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
}
void alarm_timer(int sig)
{
  if (ignore == false) {
  printf("Alarm_Triggered\n");
  // signal(SIGALRM, SIG_IGN);
    // printf("Alarm triggered!\n");
    ualarm(SCHEDULER_INTERVAL_USECS,0);
    // signal(SIGALRM, alarm_timer);
    // ualarm(SCHEDULER_INTERVAL_USECS,0);
    schedule(ALARM_TRIGGERED);
  }

}

static struct sigaction sact;
static sigset_t blocked_signals;

static void scheduler_init()
{
  // struct sigaction sact;
    sigemptyset( &sact.sa_mask );
    sact.sa_flags = SA_NODEFER;
    sact.sa_handler = alarm_timer;
    sigaddset(&blocked_signals, SIGALRM);
    sigaction( SIGALRM, &sact, NULL );
    ualarm(SCHEDULER_INTERVAL_USECS,0);
}

static void lock() {
  sigprocmask(SIG_BLOCK, &blocked_signals, &sact.sa_mask);
}


static void unlock() {
		sigprocmask(SIG_SETMASK, &sact.sa_mask, NULL);
}
/*void alarm_timer(int sig)
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
    }*/

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

typedef struct BBpointer{
  BB *ptr;
} BBptr;

int pthread_barrier_init(   pthread_barrier_t *restrict barrier,
                            const pthread_barrierattr_t *restrict attr,
                            unsigned count)
{
  
  BBptr * tmp = (BBptr *) barrier;
  /*printf("sz li %d, sz ptr %d\n", sizeof(long int), sizeof(char *));

  long int a;
  char *p;

  a = (long int)p;
  p = (char *)a;
  */
  lock();
  // tmp = first_barrier;
 
  BB *barrier_struct = (BB *) calloc(1,sizeof(BB));
  
  barrier_struct->blocked_thread_list = (int *) calloc(count, 4);
  barrier_struct->thread_num = 0;
  barrier_struct->count = count;
  tmp->ptr = barrier_struct;
  // barrier = calloc(1,sizeof(BB));
  // *barrier = (( pthread_barrier_t) barrier_struct);
 
  /*
  if (tmp == NULL || tmp == 0) {
    BB *barrier_struct = (BB *) calloc(1,sizeof(BB));
    barrier_struct->barrier_ID = barrier;
    barrier_struct->count = count;
    barrier_struct->thread_num = 0;
    barrier_struct->blocked_thread_list = (int *) calloc(count, 4);
    first_barrier = barrier_struct;
  } else {
    while (tmp->next != NULL) {
      tmp = tmp->next;
    }
    
    BB *barrier_struct = (BB *) calloc(1,sizeof(BB));
    barrier_struct->barrier_ID = barrier;
    barrier_struct->count = count;
    barrier_struct->thread_num = 0;
    barrier_struct->blocked_thread_list = (int *) calloc(count, 4);
        tmp->next = barrier_struct;
    
	};*/
  unlock();
    return 0;
}

int pthread_barrier_destroy(pthread_barrier_t *barrier)
{
  lock();
  BBptr * tmp = (BBptr *) barrier; 
  free(tmp->ptr->blocked_thread_list);
  free(tmp->ptr);
  
  /*BB *tmp;
  BB *prev;
  lock();
  prev = first_barrier;
  tmp = first_barrier;
  while (tmp->barrier_ID != barrier) {
    tmp = tmp->next;
    if (tmp == NULL) {
      perror("ERROR: No such barrier found");
      unlock();
      return -1;
    }
  }
    if (tmp == first_barrier) {
      if (tmp->next == NULL) {
	// No action necessary
      } else {
	first_barrier = tmp->next;
      }
    } else {
      while (prev->next != tmp) {
	prev = prev->next;
      }
      prev->next = tmp->next;
    }
    if (tmp == first_barrier) {
      free(tmp->blocked_thread_list);
      
      free(tmp);// No action necessary
      first_barrier = NULL;
    } else {
    free(tmp->blocked_thread_list);
    //free(tmp->barrier_ID);
    free(tmp);
    }
  */
    unlock();
    return 0;
}

int pthread_barrier_wait(pthread_barrier_t *barrier)
{

  lock();
  BBptr * tmp = (BBptr *) barrier;
  //BB *barrier_struct = (BB *) barrier;
  
  tmp->ptr->thread_num = tmp->ptr->thread_num + 1;
  if (tmp->ptr->count != tmp->ptr->thread_num) {
    printf("Thread %d Blocked\n", run_queue->thread_ID);
    tmp->ptr->blocked_thread_list[tmp->ptr->thread_num-1] = run_queue->thread_ID;
    run_queue->TS = TS_BLOCKED;
    unlock();
    schedule(THREAD_BLOCKED);
  } else {
    printf("Barrier Unblocked\n");
    struct thread_control_block *tmp_thread;
    tmp_thread = run_queue->next;
    while(tmp_thread != run_queue) {
      for (int i = 0; i < tmp->ptr->count; i++) {
	if (tmp->ptr->blocked_thread_list[i] == tmp_thread->thread_ID) {
	  tmp->ptr->blocked_thread_list[i] = 0;
	  tmp_thread->TS = TS_READY;
	  break;
	}
      }
      tmp_thread = tmp_thread->next;
    }
    tmp->ptr->thread_num = 0;
    //barrier = (pthread_barrier_t *) &barrier_struct;
    unlock();
    return PTHREAD_BARRIER_SERIAL_THREAD;
    }
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
  BT *tmp;
  lock();
  tmp = blocked_first;
  while (tmp->mutex_ID != mutex) {
    tmp = tmp->next;
  }
  if (tmp->is_used == false) {
    printf("Locked\n");
    tmp->is_used = true;
  } else {
    printf("Thread %d Blocked\n", run_queue->thread_ID);
    for (int i=0; i < MAX_MUTBAR; i++) {
      if (tmp->blocked_thread_list[i] == 0) {
	tmp->blocked_thread_list[i] = run_queue->thread_ID;
	break;
      }
    }
    run_queue->TS = TS_BLOCKED;
    unlock();
    schedule(THREAD_BLOCKED);
  }
  unlock();
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
  BT *tmp;
  lock();
  printf("Unlocked\n");
  tmp = blocked_first;
  while (tmp->mutex_ID != mutex) {
    tmp = tmp->next;
  }
  tmp->is_used = false;
  struct thread_control_block *tmp_thread;
  tmp_thread = run_queue->next;
  while(tmp_thread != run_queue) {
    for (int i = 0; i < MAX_MUTBAR; i++) {
      if (tmp->blocked_thread_list[i] == tmp_thread->thread_ID) {
	tmp_thread->TS = TS_READY;
	break;
      }
    }
    tmp_thread = tmp_thread->next;
  }
  unlock();
    return 0;
}

int pthread_mutex_init(
    pthread_mutex_t *restrict mutex,
    const pthread_mutexattr_t *restrict attr)
{
  BT *tmp;
  tmp = blocked_first;
  if (tmp == NULL) {
    BT *mutex_struct = (BT *) malloc(sizeof(BT));
    mutex_struct->mutex_ID = mutex;
    mutex_struct->is_used = false;
    mutex_struct->blocked_thread_list = (int *) calloc(MAX_MUTBAR,4);
    blocked_first = mutex_struct;
  } else {
  while (tmp->next != NULL) {
    tmp = tmp->next;
  }
  BT *mutex_struct = (BT *) malloc(sizeof(BT));
  mutex_struct->mutex_ID = mutex;
  mutex_struct->is_used = false;
  mutex_struct->blocked_thread_list = (int *) calloc(MAX_MUTBAR,4);
  tmp->next = mutex_struct;
  };
  return 0;
}

int pthread_mutex_destroy(
    pthread_mutex_t *mutex)
{
  BT *tmp;
  BT *prev;
  prev = blocked_first;
  tmp = blocked_first;
  while (tmp->mutex_ID != mutex) {
    tmp = tmp->next;
    if (tmp == NULL) {
      perror("ERROR: No such mutex found");
      return -1;
    }
  }
  if (tmp->next == NULL) {
    // No action necessary
  } else if (tmp == blocked_first) {
    blocked_first = tmp->next;
    
  } else {
    while (prev->next != tmp) {
      prev = prev->next;
    }
    prev->next = tmp->next;
  }
  
  for (int i = 0; i < MAX_MUTBAR; i++) {
    if (tmp->blocked_thread_list[i] != 0) {
      for (int j = 0; j < MAX_MUTBAR; j++) {
	if (unblock_array[j] == 0) {
	  unblock_array[j] = tmp->blocked_thread_list[i];
	  tmp->blocked_thread_list[i] = 0;
	  break;
	}
      }
    }
  }
  free(tmp->blocked_thread_list);
  free(tmp);
  
  return 0;
}


pthread_t pthread_self(void)
{
  return run_queue->thread_ID;/* TODO: Return the current thread instead of -1
     // * Hint: this function can be implemented in one line, by returning
     // * a specific variable instead of -1.
     // */
  //   return -1;
}

/* Don't implement main in this file!
 * This is a library of functions, not an executable program. If you
 * want to run the functions in this file, create separate test programs
 * that have their own main functions.
 */


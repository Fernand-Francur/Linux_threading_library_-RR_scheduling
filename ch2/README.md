Copyright Timothy Borunov 2022

PROGRAM: threads.c
// The program is a threading library which imitates the following functions from the pthread.h library:
//     -pthread_create()
//     -pthread_exit()
//     -pthread_self()
// In essence, by including thread.c as a dependency for another file, it grants those files the ability
// to implement multithreading using a built in round robin scheduler.
// At the current time, no solution is being implemented for deadlocks and resources, just the ability
// to create threads which run according to a round-robin scheduler design on a 50 ms interrupt signal

// The program contains the following files in the make file:
FILES: threads.c ec440threads.h (any_program_to_use_threading_library).c

Compile library by running 'make'
Run files with library by modifying the makefile to include programs and running 'make check'
Remove object files by running 'make clean'

The following links were used as resources for creating this library:

Used for understanding the registers of jmp_buf in the versions of C used to make library:
https://elixir.bootlin.com/glibc/glibc-2.27.9000/source/sysdeps/x86_64/jmpbuf-offsets.h

The following was a valuable resource for understanding the logic of the scheduler
and was the main resource used for creating the signaling system in my threading library.
The document features a great demonstration of using setjmp and longjmp and features a lot
of very illuminating pseudo code which served as a good model for my code.
http://www.csl.mtu.edu/cs4411.ck/common/Coroutines.pdf
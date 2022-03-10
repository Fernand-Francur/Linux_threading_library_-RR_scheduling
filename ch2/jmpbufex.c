#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
jmp_buf JumpBuffer;
int result;

void fact(int Result, int Count, int n)
{
  if (Count <= n)
    fact(Result*Count, Count+1, n);
  else {
    result = Result;
    longjmp(JumpBuffer, 1);
  }
}

void factorial(int n)
{
  fact(1, 1, n);
}




void main(int argc, char *argv[])
{
  int n;
  n = atoi(argv[1]);
  if (setjmp(JumpBuffer) == 0)
    factorial(n);
  else
  printf("%d! = %d\n", n, result);
  exit(0);
}



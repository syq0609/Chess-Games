#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>

static ucontext_t uctx_main, uctx_func1, uctx_func2;
char func1_stack[16384];
char func2_stack[16384];


#define handle_error(msg) \
do { perror(msg); exit(EXIT_FAILURE); } while (0)

static void
func1(void)
{
	printf("func1: started\n");
	printf("func1: yield to main\n");
	if (swapcontext(&uctx_func1, &uctx_main) == -1)
		handle_error("swapcontext");
	printf("func1: returning\n");
}

static void
func2(void)
{
	printf("func2: started\n");
    //创建corot1
	printf("func2: create 1\n");
	if (getcontext(&uctx_func1) == -1)
		handle_error("getcontext");
	uctx_func1.uc_stack.ss_sp = func1_stack;
	uctx_func1.uc_stack.ss_size = sizeof(func2_stack);
    uctx_func1.uc_link = &uctx_main;
	makecontext(&uctx_func1, func1, 0);

    //yield to main
	printf("func2: yield to main\n");
	if (swapcontext(&uctx_func2, &uctx_main) == -1)
		handle_error("swapcontext");
	printf("func2: returning\n");
}

int
main(int argc, char *argv[])
{
	printf("main: started\n");

    //创建并启动corot2
	printf("main: create 2\n");
	if (getcontext(&uctx_func2) == -1)
		handle_error("getcontext");
	uctx_func2.uc_stack.ss_sp = func2_stack;
	uctx_func2.uc_stack.ss_size = sizeof(func2_stack);
    uctx_func2.uc_link = &uctx_main;
	makecontext(&uctx_func2, func2, 0);

	printf("main: resume 2\n");
	if (swapcontext(&uctx_main, &uctx_func2) == -1)
		handle_error("swapcontext");

    //启动或恢复corot1
	printf("main: resume 1\n");
	if (swapcontext(&uctx_main, &uctx_func1) == -1)
		handle_error("swapcontext");

	printf("main: exiting\n");
	exit(EXIT_SUCCESS);
}

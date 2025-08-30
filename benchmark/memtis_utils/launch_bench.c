#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <sys/wait.h>

int syscall_htmm_start = 449;
int syscall_htmm_end = 450;

long htmm_start(pid_t pid, int node)
{
    return syscall(syscall_htmm_start, pid, node);
}

long htmm_end(pid_t pid)
{
    return syscall(syscall_htmm_end, pid);
}

int main(int argc, char** argv)
{
    pid_t pid;
    int state;

    if (argc < 2) {
	printf("Usage ./launch_bench [BENCHMARK]");	
	htmm_end(-1);
	return 0;
    }

    char *new_argv[argc + 3];
    new_argv[1] = "numactl";
    new_argv[2] = "--membind=0,2";
    for (int i = 1; i < argc; i++) {
        new_argv[i + 2] = argv[i];
    }       
    new_argv[argc + 2] = NULL;              
    pid = fork();
    if (pid == 0) {
        printf("new_argv[1]: %s\n", new_argv[1]);
        execvp(new_argv[1], &new_argv[1]);
        perror("Fails to run bench");
        exit(-1);
    }
#ifdef __NOPID
    htmm_start(-1, 0);
#else
    htmm_start(pid, 0);
#endif
    printf("pid: %d\n", pid);
    waitpid(pid, &state, 0);

    htmm_end(-1);
    
    return 0;
}

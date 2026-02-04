#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "common.h"
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
static void usage(const char *a)
{
    fprintf(stderr, "Usage: %s <cmd> [args]\n", a);
    exit(1);
}
static double d(struct timespec a, struct timespec b)
{
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9;
}
int main(int c, char **v)
{
    if (c < 2)
        usage(v[0]);
    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0)
        DIE("clock_gettime start");

    pid_t pid = fork();
    if (pid < 0)
        DIE("fork");

    if (pid == 0)
    {
        // child: run command v[1] with args v[1..]
        execvp(v[1], &v[1]);
        // only reached if exec fails
        DIE("execvp");
    }

    int status;
    if (waitpid(pid, &status, 0) < 0)
        DIE("waitpid");

    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0)
        DIE("clock_gettime end");

    double elapsed = d(start, end);

    if (WIFEXITED(status))
    {
        printf("pid=%d elapsed=%.3f exit=%d\n", pid, elapsed, WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status))
    {
        printf("pid=%d elapsed=%.3f signal=%d\n", pid, elapsed, WTERMSIG(status));
    }
    else
    {
        printf("pid=%d elapsed=%.3f status=unknown\n", pid, elapsed);
    }

    return 0;
}

#include "common.h"
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *a)
{
    fprintf(stderr, "Usage: %s <pid>\n", a);
    exit(1);
}
static int isnum(const char *s)
{
    if (!s || !*s)
    {
        return 0;
    }
    for (; *s; s++)
    {
        if (!isdigit((unsigned char)*s))
            return 0;
    }
    return 1;
}
int main(int c, char **v)
{
    if (c != 2 || !isnum(v[1]))
        usage(v[0]);
    long pid = strtol(v[1], NULL, 10);
    char path[256];
    snprintf(path, sizeof(path), "/proc/%ld/stat", pid);
    FILE *f = fopen(path, "r");
    if (!f)
    {
        DIE("open stat");
    }
    char line[4096];
    if (!fgets(line, sizeof(line), f))
    {
        fclose(f);
        DIE("read stat");
    }
    fclose(f);
    char *rp = strrchr(line, ')');
    if (!rp)
        DIE_MSG("bad stat format");
    char state = '?';
    long ppid = -1;
    unsigned long long utime = 0, stime = 0;
    if (sscanf(rp + 2, "%c %ld", &state, &ppid) != 2)
        DIE_MSG("bad stat fields");
    int field = 3;
    char *p = rp + 2;
    char tmp[4096];
    snprintf(tmp, sizeof(tmp), "%s", p);
    char *save = NULL;
    char *tok = strtok_r(tmp, " ", &save);
    while (tok)
    {
        if (field == 14)
            utime = strtoull(tok, NULL, 10);
        if (field == 15)
        {
            stime = strtoull(tok, NULL, 10);
            break;
        }
        field++;
        tok = strtok_r(NULL, " ", &save);
    }

    long ticks = sysconf(_SC_CLK_TCK);
    double cpu = (ticks > 0) ? (double)(utime + stime) / (double)ticks : 0.0;

    snprintf(path, sizeof(path), "/proc/%ld/status", pid);
    f = fopen(path, "r");
    if (!f)
        DIE("open_status");

    long vmrss = 0;
    char buf[512];
    while (fgets(buf, sizeof(buf), f))
    {
        if (sscanf(buf, "VmRSS: %ld", &vmrss) == 1)
            break;
    }
    fclose(f);
    snprintf(path, sizeof(path), "/proc/%ld/cmdline", pid);
    f = fopen(path, "r");
    if (!f)
        DIE("open cmdline");
    char cmd[4096];
    size_t n = fread(cmd, 1, sizeof(cmd) - 1, f);
    fclose(f);

    for (size_t i = 0; i < n; i++)
        if (cmd[i] == '\0')
            cmd[i] = ' ';
    cmd[n] = '\0';

    printf("PID: %ld\n", pid);
    printf("State: %c\n", state);
    printf("PPID: %ld\n", ppid);
    printf("Cmd: %s\n", cmd[0] ? cmd : "[empty]");
    printf("CPU: %.3f\n", cpu);
    printf("VmRSS: %ld\n", vmrss);

    return 0;
}

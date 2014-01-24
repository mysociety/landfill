/*
 * dumpmem.c:
 * Dump part of a process's address space.
 *
 * Copyright (c) 2007 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 * 
 *
 */

// Usage example:
// dumpmem $PID `grep '\[heap\]' /proc/$PID/maps | sed 's/^\([0-9a-f]*\)-\([0-9a-f]\)*/0x\1 0x\2'`

static const char rcsid[] = "$Id: dumpmem.c,v 1.2 2007-03-05 08:40:55 francis Exp $";

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ptrace.h>
#include <sys/wait.h>

#define err(...) \
            do { \
                fprintf(stderr, "dumpmem: "); \
                fprintf(stderr, __VA_ARGS__); \
                fprintf(stderr, "\n"); \
            } while (0)
#define die(...) \
            do { \
                err(__VA_ARGS__); \
                exit(1); \
            } while (0)

int main(int argc, char *argv[]) {
    pid_t target;
    int status;
    unsigned long addr1, addr2, a;
    char *p;

    if (2 == argc
        && (0 == strcmp(argv[1], "-h") || 0 == strcmp(argv[1], "--help"))) {
        printf(
"dumpmem -h | PID START END\n"
"\n"
"Write the part of the address space of PID between START and END (addresses\n"
"in that process's address space; prefix \"0x\" for hex or \"0\" for octal)\n"
"to standard output. Try using /proc/PID/maps to find ranges to dump.\n"
"\n"
"Copyright (c) 2007 Chris Lightfoot. All rights reserved.\n"
"Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/\n"
"%s\n",
            rcsid);
        return 0;
    }
    
    if (argc != 4)
        die("arguments are target process ID and address range");
    else if (0 == (target = (pid_t)atoi(argv[1]))
        || target < 1)
        die("\"%s\" is not a valid process ID", argv[1]);
    else if (1 == target)
        die("can't dump memory of init");

    addr1 = strtoul(argv[2], &p, 0);
    if (*p)
        die("\"%s\" is not a valid address", argv[2]);
    addr2 = strtoul(argv[3], &p, 0);
    if (*p)
        die("\"%s\" is not a valid address", argv[3]);

    if (addr2 <= addr1)
        die("end address must be after start address");
    
    if (-1 == ptrace(PTRACE_ATTACH, target, NULL, NULL))
        die("ptrace: %s", strerror(errno));

    /* target has received a SIGSTOP; wait for it to stop */
    if (-1 == waitpid(target, &status, WUNTRACED))
        die("waitpid: %s", strerror(errno));

    for (a = addr1; a <= addr2; a += sizeof(long)) {
        void *q;
        long r;
        q = (void*)a;
        r = ptrace(PTRACE_PEEKTEXT, target, q, NULL);
        fwrite(&r, sizeof r, 1, stdout);
    }
    
    kill(target, SIGCONT);
    ptrace(PTRACE_DETACH, target, NULL, NULL);
    
    return 0;
}


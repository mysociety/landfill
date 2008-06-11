/* Found this in Chris's home directory on water. Looks useful maybe.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    char *buf;
    char cmdline[64];
    int i;
    buf = malloc(10485760);
    memset(buf, 0, 10485760);
    printf("in parent:\n");
    sprintf(cmdline, "pmap -d %d", (int)getpid());
    system(cmdline);
    sprintf(cmdline, "cat /proc/%d/smaps", (int)getpid());
    system(cmdline);
    for (i = 0; i < 10; ++i) {
        if (0 == fork()) {
            sleep(60);
            _exit(0);
        }
    }
    if (0 == fork()) {
        printf("\n\nin child:\n");
        sprintf(cmdline, "pmap -d %d", (int)getpid());
        system(cmdline);
        sprintf(cmdline, "cat /proc/%d/smaps", (int)getpid());
        system(cmdline);
        _exit(0);
    }
}

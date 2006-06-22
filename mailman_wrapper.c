/*
 * mailman_wrapper.c:
 * Setuid wrapper to invoke Mailman python scripts from Exim.
 *
 * $Id: mailman_wrapper.c,v 1.1 2006-06-22 15:36:03 chris Exp $
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LIST_UID    38
#define LIST_GID    38

#define EXIM_UID    102
#define EXIM_GID    102

#define LIST_DOMAIN_PROG    "/etc/exim4/scripts/mailman_list_domain"
#define TEST_SENDER_PROG    "/etc/exim4/scripts/mailman_test_sender"

#define err(...)    do { fprintf(stderr, "mailman_wrapper: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while (0)
#define die(...)    do { err(__VA_ARGS__); exit(1); } while (0)

int main(int argc, char *argv[]) {
    char *aa[4] = {0}, *ee[] = { "PATH=/bin:/usr/bin:/usr/local/bin", NULL };
    if ((getuid() != 0 && getuid() != EXIM_UID)
        || (getgid() != 0 && getgid() != EXIM_GID))
        die(strerror(EPERM));
    if (argc < 2)
        die("Need arguments");
    else if (0 == strcmp(argv[1], "list-domain")) {
        if (argc != 3)
            die("Need exactly one argument to list-domain command");
        aa[0] = LIST_DOMAIN_PROG;
        aa[1] = argv[2];
    } else if (0 == strcmp(argv[1], "test-sender")) {
        if (argc != 4)
            die("Need exactly two arguments to test-sender command");
        aa[0] = TEST_SENDER_PROG;
        aa[1] = argv[2];
        aa[2] = argv[3];
    } else
        die("Bad command \"%s\"", argv[1]);
    
    setgid(LIST_GID);
    setuid(LIST_UID);
    
    execve(aa[0], aa, ee);

    die("%s: execve: %s", aa[0], strerror(errno));
}

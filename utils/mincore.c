/*
 * mincore.c:
 * Establish what fraction of a file's pages are in core.
 *
 * TODO: handle block devices properly; for large files, sample.
 * 
 * Copyright (c) 2006 UK Citizens Online Democracy. All rights reserved.
 * Email: francis@mysociety.org; WWW: http://www.mysociety.org/
 *
 */

static const char rcsid[] = "$Id: mincore.c,v 1.2 2006-11-10 22:51:31 chris Exp $";

#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#define PAGE_SIZE   getpagesize()

size_t getmaplen(const size_t s) {
    return ((s / PAGE_SIZE) + 1) * PAGE_SIZE;
}

int main(int argc, char *argv[]) {
    char **a;
    unsigned char *vec = NULL;
    size_t veclen = 0;
    int ret = 0;

    if (argc == 2
        && (0 == strcmp(argv[1], "-h") || 0 == strcmp(argv[1], "--help"))) {
        printf(
"mincore -h | [FILE ...]\n"
"\n"
"Estimate what fraction of each FILE's pages are in core, and print the\n"
"result to standard output. A typical output line might look like,\n"
"  /path/to/file 8192 65536\n"
"for a 64KB file of which 8KB was in core.\n"
"\n"
"Copyright (c) 2006 UK Citizens Online Democracy. All rights reserved.\n"
"Email: francis@mysociety.org; WWW: http://www.mysociety.org/\n"
"%s\n",
            rcsid);
        return 0;
    }
    
    for (a = argv + 1; *a; ++a) {
        int fd = -1;
        char *map = MAP_FAILED;
        size_t maplen = 0, mapped;
        struct stat st;
        int i, nmapped = 0;

        if (-1 == (fd = open(*a, O_RDONLY))
            || -1 == fstat(fd, &st)
            || (S_ISDIR(st.st_mode) && (errno = EISDIR))
            || MAP_FAILED == (map = mmap(NULL, maplen = getmaplen(st.st_size), PROT_READ, MAP_SHARED, fd, 0))) {
            fprintf(stderr, "mincore: %s: %s\n", *a, strerror(errno));
            ++ret;
            goto next;
        }

        if (!vec || veclen < maplen / PAGE_SIZE)
            vec = realloc(vec, veclen = maplen / PAGE_SIZE);

        if (-1 == mincore(map, st.st_size, vec)) {
            fprintf(stderr, "mincore: %s: mincore: %s\n", *a, strerror(errno));
            ++ret;
            goto next;
        }

        for (i = 0, nmapped = 0; i < maplen / PAGE_SIZE; ++i)
            if (vec[i] & 1) ++nmapped;

        mapped = nmapped * PAGE_SIZE;
        if (mapped > st.st_size) mapped = st.st_size;
        printf("%s %u %u\n", *a, (unsigned)mapped, (unsigned)st.st_size);

next:
        if (-1 != fd) close(fd);
        if (MAP_FAILED != map) munmap(map, maplen);
    }
    return ret;
}

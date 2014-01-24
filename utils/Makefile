#
# Makefile:
# Build miscellaneous utilities.
#
# Copyright (c) 2006 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: Makefile,v 1.4 2007-01-24 13:29:49 francis Exp $
#

CFLAGS = -Wall -g -O2
LDFLAGS =
LDLIBS = 

all: mailman_wrapper mincore

mailman_wrapper: mailman_wrapper.c Makefile
	$(CC) $(CFLAGS) \
	        -DEXIM_UID=$(shell getent passwd Debian-exim | cut -d: -f3 ) \
	        -DEXIM_GID=$(shell getent passwd Debian-exim | cut -d: -f4 ) \
	        -DLIST_UID=$(shell getent passwd list | cut -d: -f3 ) \
	        -DLIST_GID=$(shell getent passwd list | cut -d: -f4 ) \
	        mailman_wrapper.c \
	        $(LDFLAGS) $(LDLIBS) -o mailman_wrapper

mincore: mincore.c Makefile
	$(CC) $(CFLAGS) mincore.c -o mincore

clean:
	rm -f mailman_wrapper mincore *~ core

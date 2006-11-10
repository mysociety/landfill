#
# Makefile:
# Build miscellaneous utilities.
#
# Copyright (c) 2006 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: Makefile,v 1.2 2006-11-10 22:50:51 chris Exp $
#

CFLAGS = -Wall -g -O2
LDFLAGS =
LDLIBS = 

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

install: mailman_wrapper mincore
	install -o list -g list -m u+xs,g+xs,o+x mailman_wrapper bin
	install mincore bin

clean:
	rm -f mailman_wrapper mincore *~ core

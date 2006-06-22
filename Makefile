#
# Makefile:
# Build miscellaneous utilities.
#
# Copyright (c) 2006 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: Makefile,v 1.1 2006-06-22 15:47:00 chris Exp $
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

install: mailman_wrapper
	install -o list -g list -m u+xs,g+xs,o+x mailman_wrapper bin

clean:
	rm -f mailman_wrapper *~ core

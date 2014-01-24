#
# Makefile:
# Build pnmtilesplit.
#
# Copyright (c) 2006 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: Makefile,v 1.1 2006-09-22 13:57:01 francis Exp $
#

CFLAGS = -Wall -g -O99
LDFLAGS =
LDLIBS = -lnetpbm		# is this a debianism?

pnmtilesplit: pnmtilesplit.c
	$(CC) $(CFLAGS) pnmtilesplit.c $(LDFLAGS) $(LDLIBS) -o pnmtilesplit

clean:
	rm -f pnmtilesplit *~ core

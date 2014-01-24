#!/usr/bin/perl -w
#
# magic.cgi:
# Daily magic number.
#
# Copyright (c) 2006 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#

my $rcsid = ''; $rcsid .= '$Id: magic.cgi,v 1.1 2006-09-05 23:27:54 chris Exp $';

use strict;

use mysociety::NotApathetic::Config;

my $magic = mysociety::NotApathetic::Config::magic();

print "Content-Type: text/plain; charset=us-ascii\r\n",
        "Content-Length: ", length($magic), "\r\n\r\n$magic";


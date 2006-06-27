#!/usr/bin/perl -I../

use warnings;
use strict;
use mySociety::Boxes::Config;
use mySociety::Boxes::Routines;

print "Content-Type: text/xml\n\n";
my $boxid=param("boxid") || 29;

&generate_rss_feed($boxid);

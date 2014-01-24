#!/usr/bin/perl -I../

use warnings;
use strict;
use mySociety::Boxes::Config;
use mySociety::Boxes::Routines;
use mySociety::Boxes::Designs;

my $boxid=param("boxid") || 1;

print "Content-Type: text/html\n\n";
print &generate_iframe($boxid);



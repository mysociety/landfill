#!/usr/bin/perl

# this is a candidate for Fast::CGI

use warnings;
use strict;



{
    print "Content-type: text/html\n\n";
    
    print <<EOhtml;
sendRPCDone(frameElement, "fast bug", new Array("fast bug track", "fast bugs", "fast bug", "fast bugtrack"), new Array("793,000 results", "2,040,000 results", "6,000,000 results", "7,910 results"), new Array(""));
EOhtml

}


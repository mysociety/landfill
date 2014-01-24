#!/usr/bin/perl

use warnings;
use strict;
use mysociety::IVotedForYouBecause::Config;
use CGI::Fast qw/param/;
my $query;
our $url_prefix=$mysociety::IVotedForYouBecause::Config::url;

while (new CGI::Fast()) {
        $query=param("q")|| '';
        $query=~ s#("\S*)\s+(\S*")#$1+$2#g;
        $query=~ s#"# #g;
        $query=~ s# #/#g;
        $query=~ s#//+#/#g;
        print "Location: ".$url_prefix."/search/?$query\r\n\r\n";
}

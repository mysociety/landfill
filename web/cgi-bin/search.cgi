#!/usr/bin/perl

use warnings;
use strict;
use mysociety::NotApathetic::Config;
use CGI qw/param/;
my $query;
our $url_prefix=$mysociety::NotApathetic::Config::url;

{
        $query=param("q")|| '';
        $query=~ s#("\S*)\s+(\S*")#$1+$2#g;
        $query=~ s#"# #g;
        $query=~ s# #/#g;
        $query=~ s#//+#/#g;
        print "Location: $url_prefix?/$query\n\n";
}

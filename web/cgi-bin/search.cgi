#!/usr/bin/perl

use warnings;
use strict;

use CGI qw/param/;
my $response= '/';
my $value;
{
	foreach my $param (param()) {
		next unless $param =~ /box\d/;
		$value= param($param);
		$value=~ s#^\s*(.*)\s*$#$1#;
		$value=~ s#\s+#\+#g;
		$response.= "/$value";
	}

	$response=~ s#//+#/#g;
	print "Location: http://panopticon.devel.disruptiveproactivity.com/?$response\n\n";
}

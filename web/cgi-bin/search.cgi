#!/usr/bin/perl

use warnings;
use strict;
use mysociety::NotApathetic::Config;
use CGI qw/param/;
my $response= '/';
our $url_prefix=$mysociety::NotApathetic::Config::url;
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
	print "Location: $url_prefix?$response\n\n";
}

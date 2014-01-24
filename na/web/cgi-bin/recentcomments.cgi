#!/usr/bin/perl

use warnings;
use strict;
use CGI qw/param/;
use Date::Manip;
use DBI;
use HTML::Entities;
use mysociety::NotApathetic::Config;
use mysociety::NotApathetic::Routines;

my %State; # State variables during display.
our $url_prefix=$mysociety::NotApathetic::Config::url;

{
	&setup;

	my $options;
	$options->{'search_url'} = $ENV{'QUERY_STRING'} || '';
	$options->{'search_url'} =~ s#page=\d*##g;
	$options->{'limit'} = $mysociety::NotApathetic::Config::main_display_limit;

	&show_comment_summary(&get_comments($options));

}



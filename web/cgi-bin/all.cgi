#!/usr/bin/perl

use warnings;
use strict;
use mysociety::NotApathetic::Config;
use mysociety::NotApathetic::Routines;

my %State; # State variables during display.
my $options;

{
	&setup;

	$options->{'search_url'} = $ENV{'QUERY_STRING'} || '';
	$options->{'search_url'} =~ s#page=\d+##g;
	$options->{'search_term'} = &handle_search_term($ENV{'QUERY_STRING'}); #' 1 = 1 ';
	$options->{'show_all_posts'}=1;

	&summary_listing(&run_query($options));
	#my $rows_printed= &details_listing(&run_query($options));
	#&details_page_footer($rows_printed, $options, '');
}

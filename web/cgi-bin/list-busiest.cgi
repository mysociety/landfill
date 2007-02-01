#!/usr/bin/perl

use warnings;
use strict;
use CGI qw/param/;
use mysociety::NotApathetic::Config;
use mysociety::NotApathetic::Routines;
use HTML::Entities;

our %State; # State variables during display.
my $options;

{
	&setup;

	$options->{'search_url'} = $ENV{'QUERY_STRING'} || '';
	$options->{'search_url'} =~ s#page=\d+##g;
	$options->{'search_term'} = encode_entities(&handle_search_term($ENV{'QUERY_STRING'})); #' 1 = 1 ';
	$options->{'page'}= $ENV{'DOCUMENT_PATH_INFO'} || 0;
	$options->{'page'}=~ s#/##g;
	$options->{'offset'}= $options->{'page'} * $main_display_limit;

        $options->{'busiest'} =  ' and (interesting=1 or commentcount >= 5) ';
        $options->{'subset'} =  'busiest';

	&listing(&run_query($options));
	#my $rows_printed= &details_listing(&run_query($options));
	#&details_page_footer($rows_printed, $options, '');
}




#!/usr/bin/perl

use warnings;
use strict;
use CGI qw/param/;
use mysociety::NotApathetic::Config;
use mysociety::NotApathetic::Routines;
use HTML::Entities;

our %State; # State variables during display.

{
	&setup;

	my $options;
	$options->{'search_url'} = $ENV{'QUERY_STRING'} || '';
	$options->{'search_url'} =~ s#page=\d*##g;

	$options->{'search_term'} = encode_entities(&handle_search_term($options->{'search_url'}));

	$options->{'page'}= $ENV{'DOCUMENT_PATH_INFO'} || 0;
	$options->{'page'}=~ s#/##g;

	$options->{'offset'}= $options->{'page'} * $main_display_limit;

	my $rows_printed= &details_listing(&run_query($options));
	&details_page_footer($rows_printed, $options, '');
}



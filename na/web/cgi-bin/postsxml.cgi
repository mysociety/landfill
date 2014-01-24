#!/usr/bin/perl

use warnings;
use strict;
use XML::Simple;
use mysociety::NotApathetic::Config;
use mysociety::NotApathetic::Routines;

my %State; # State variables during display.
my $search_term = &handle_search_term(); #' 1 = 1 ';


{
        print "Content-Type: text/xml\r\n\r\n";
	&setup;

	my $query=$dbh->prepare("
	              select postid, why, posted, title, commentcount, q1, q2, q3, q4, q5
			from posts
	 	       where validated=1
			 and hidden=0
			 and site='$site_name'
			     $search_term
		    order by posted
			     desc limit 25
		       "); # XXX order by first_seen needs to change


	$query->execute;
	my $result;
	my $google_terms;
	my $comments_html;
	my $date_html;
	my $show_link;
	my $more_link;
	my $results=$query->fetchall_arrayref({});
	print XMLout($results, (GroupTags => {'anon' => 'post'}, KeyAttr=>"postid", NoAttr=>1, RootName=>"notapathetic"));
}




#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;
use XML::RSS;
use mysociety::NotApathetic::Config;
use mysociety::NotApathetic::Routines;
my $url_prefix= $mysociety::NotApathetic::Config::url;
my $email_domain= $mysociety::NotApathetic::Config::email_domain;
my $site_name= $mysociety::NotApathetic::Config::site_name;

if ((defined $ENV{QUERY_STRING}) and ($ENV{QUERY_STRING} =~/^\d+$/)){
	print "Location: $url_prefix/cgi-bin/rss-comments.cgi?$ENV{QUERY_STRING}\r\n\r\n";
}

my %State; # State variables during display.
my $options;

{
	&setup;
        print "Content-Type: text/xml\r\n\r\n";

	my $query;
	($query, $options)= &run_query($options);

 use XML::RSS;
 my $rss = new XML::RSS (version => '1');
 $rss->channel(
   title        => "$site_name",
   link         => "$url_prefix",
   description  => "New posts on $site_name",
   dc => {
     creator    => "team$email_domain",
     publisher  => "team$email_domain",
     language   => 'en-gb',
     ttl        =>  600
   },
   syn => {
     updatePeriod     => "daily",
     updateFrequency  => "1",
     updateBase       => "1901-01-01T00:00+00:00",
   },
);

	while (my $result= $query->fetchrow_hashref) {
            $rss->add_item(
 		       title => "$result->{title}",
                       link => "$url_prefix/comments/$result->{postid}",
                       description=> "$result->{shortwhy}"
                );
        }

        print $rss->as_string;

}




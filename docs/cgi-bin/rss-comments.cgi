#!/usr/bin/perl

use warnings;
use strict;
use FindBin;
use lib "$FindBin::Bin/../../perllib";
use lib "$FindBin::Bin/../../../perllib";
use mySociety::Config;
BEGIN {
    mySociety::Config::set_file("$FindBin::Bin/../../conf/general");
}
use PoP;
use HTML::Entities;
use XML::RSS;

my $url_prefix= mySociety::Config::get('URL');
my $email_domain= mySociety::Config::get('EMAIL_DOMAIN');
my $site_name= mySociety::Config::get('SITE_NAME');
my %State; # State variables during display.

{
    my $Entry=$ENV{QUERY_STRING} || '';
    if (($Entry ne '') and ($Entry !~ /^\d+$/)) {
            print "Location: $url_prefix\r\n\r\n";
            next;
    }

    print "Content-Type: text/xml\r\n\r\n";
	my $query;
	my $title='';
	my $limiter='';
	if ($Entry ne '') {
		$query=$dbh->prepare("
	 	select title from posts where postid='$Entry'
	");
		$query->execute;
		($title)= $query->fetchrow_array;
		$title = "on '$title' ";
		$limiter= " and comments.postid='$Entry' ";
	}
	$query=$dbh->prepare("
	              select comments.postid, comment, commentid, posts.title
			from comments,posts
	 	       where comments.visible=1
		         and comments.postid=posts.postid
                         and site='$site_name'
		             $limiter
		    order by commentid
			desc limit 50
		       "); # XXX order by first_seen needs to change


	$query->execute;
	my $result;
	my $comments_html;
	my $date_html;
 use XML::RSS;
 my $rss = new XML::RSS (version => '1');
 $rss->channel(
   title        => "Comments on $title on  $url_prefix ",
   link         => "$url_prefix",
   description  => "Comments on $title on $url_prefix ",
   dc => {
     creator    => "team$email_domain",
     publisher  => "team$email_domain",
     rights     => 'Copyright 2005',
     language   => 'en-gb',
     ttl        =>  600
   },
   syn => {
     updatePeriod     => "daily",
     updateFrequency  => "1",
     updateBase       => "1901-01-01T00:00+00:00",
   },
);

	while ($result=$query->fetchrow_hashref) {
            $rss->add_item(
                       title => "$result->{title}",
                       link => "$url_prefix/comments/$result->{postid}#comment_$result->{commentid}",
                       description=> "$result->{comment}"
                );
        }

        print $rss->as_string;

}




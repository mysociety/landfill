#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;
use XML::RSS;
use mysociety::NotApathetic::Config;
use CGI::Fast;

my $url_prefix= $mysociety::NotApathetic::Config::url;
my $email_domain= $mysociety::NotApathetic::Config::email_domain;

my $Entry=$ENV{QUERY_STRING};
if (($Entry ne '') and ($Entry !~ /^\d+$/)) {
        print "Location: $url_prefix\n\n";
        exit (0);
}

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password

my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});
my %State; # State variables during display.


while (new CGI::Fast()) {
print "Content-Type: text/xml\n\n";
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




#!/usr/bin/perl

use warnings;
use strict;
use CGI::Carp qw/fatalsToBrowser/;
use DBI;
use HTML::Entities;
use mysociety::NotApathetic::Config;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});
my %State; # State variables during display.
our $url_prefix=$mysociety::NotApathetic::Config::url;

print "Content-type: text/html\n\n";
{

	my $query=$dbh->prepare("
	              select * from posts
	 	       where validated=1
			 and hidden=0
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
	while ($result=$query->fetchrow_hashref) {

		$more_link= $result->{link};
		print <<EOfragment;
	$date_html
	<div class="entry">
		<a href="$url_prefix/admin/comments.shtml?$result->{postid}"><strong>$result->{title}</strong></a>
	<form method="post" action="/admin/cgi-bin/hide.cgi" />
		$result->{shortwhy}
		<input type="hidden" name="postid" value="$result->{postid}" />
		<input type="submit" value="Hide this posting" />
	</form>
		<span>
			<small>posted at  $result->{posted}. <a href="$url_prefix/admin/comments.shtml?$result->{postid}">$result->{commentcount} comments.</a> by $result->{email}</small>
		</span>
	</div>
		<br />
EOfragment
	}

}




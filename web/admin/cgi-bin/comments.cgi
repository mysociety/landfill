#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;

my $Entry=$ENV{QUERY_STRING} || '';

if ($Entry !~ /^\d+$/) {
	print "Content-Type: text/html/\n\n";
	print "$Entry Error - no entry id passed in\n\n";
	exit (0);
}

use mysociety::NotApathetic::Config;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $url_prefix= $mysociety::NotApathetic::Config::url;
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});
my %State; # State variables during display.

print "Content-Type: text/html\n\n";
{
	my $query=$dbh->prepare("
	              select postid,
		             email,
			     age,
			     sex,
			     region,
			     evervoted,
			     why,
			     nochildren,
			     title,
			     posted,
			     commentcount,
			     ethgroup,
			     date_format(posted, \"%H:%i, %e %M\") as posted_formatted
			from posts
		       where postid=$Entry
		    order by posted desc
		       "); # XXX order by first_seen needs to change


	$query->execute;
	my $result;
	my $google_terms;

	if ($query->rows == 0) {
		print "Location: $url_prefix\n";
	}
	$result=$query->fetchrow_hashref;
	my $why = $result->{why};

	$why=~s/\r\n\r\n/<\/p><p>/g;
	$why=~s/\r\n/<br \/>/g;
	
	print <<EOfragment;
	<h2>Administer comments</h2>
	<div class="entry">
		<h4>$result->{title}</h4>
		<p>$why</p>
		<div>
		<small>
		Posted by: $result->{email}
		at $result->{posted}
		</small>
		</div>
	</div>
		<br />

EOfragment
	if ($result->{commentcount} > 0) {print &show_comments();}
}

sub show_comments {
	my $html=" <h2>Comments</h2>";

	my $query=$dbh->prepare(
	  " select * from comments where postid=$Entry");

	my $result;

	$query->execute;

	while ($result=$query->fetchrow_hashref) {

		$html.= <<EOhtml;
	<div class="entry">
		<a name="comment_$result->{commentid}" ></a>
		<p>
		$result->{comment}
		</p>
		<div>
		<form method="post" action="/admin/cgi-bin/hide.cgi">
			<small>Posted by $result->{name} $result->{email} on $result->{posted}.</small>
			<input type="hidden" name="postid" value="$Entry" />
			<input type="hidden" name="commentid" value="$result->{commentid}" />
			<input type="submit" value="Hide Comment" />
		</form>
		</div>
	</div>
EOhtml
	}
	

	return $html;
}



#!/usr/bin/perl -I ../../cgi-bin

use warnings;
use strict;
use CGI::Carp qw/fatalsToBrowser/;
use DBI;
use Date::Manip;
use HTML::Entities;
use mysociety::NotApathetic::Config;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});
my %State; # State variables during display.
our $url_prefix=$mysociety::NotApathetic::Config::url;
our $site_name=$mysociety::NotApathetic::Config::site_name;
my $abuse_address= $mysociety::NotApathetic::Config::abuse_address;
our $admin_url_prefix=$mysociety::NotApathetic::Config::admin_url;

print "Content-type: text/html\n\n";
{

	my $query=$dbh->prepare("
	              select * from posts
	 	       where validated=1
                         and site='$site_name'
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
	my $hideorshow;
	while ($result=$query->fetchrow_hashref) {
		$hideorshow = "hide";
		if ($result->{hidden}==1){$hideorshow = "unhide";}
		$more_link= $result->{link};
		my $someday = UnixDate($result->{posted}, "%E %b %Y");
		print <<EOfragment;
	<div class="entry">
		<h4><a href="$admin_url_prefix/comments/$result->{postid}">$result->{title}</a></h4>
		$result->{shortwhy}
	<form method="post" action="$admin_url_prefix/cgi-bin/$hideorshow.cgi" />
		<input type="hidden" name="postid" value="$result->{postid}" />
		<input type="submit" value="$hideorshow this post" />
	</form>
		<div>
		<small>
			written $someday | <a href="$admin_url_prefix/comments.shtml?$result->{postid}">$result->{commentcount} responses</a> by $result->{email}
		</small>
		</div>
	</div>

EOfragment
	}

}




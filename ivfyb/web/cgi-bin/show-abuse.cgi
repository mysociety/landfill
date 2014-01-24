#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;
use CGI::Fast qw/param/;

use mysociety::IVotedForYouBecause::Config;
my $dsn = $mysociety::IVotedForYouBecause::Config::dsn; # DSN connection string
my $db_username= $mysociety::IVotedForYouBecause::Config::db_username;              # database username
my $db_password= $mysociety::IVotedForYouBecause::Config::db_password;         # database password
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});

while (new CGI::Fast()) {
        print "Content-Type: text/html; charset=iso-8859-1\r\n\r\n";
    	my $postid= param('postid') || next;
    	my $commentid= param('commentid') || '';

	print "<input type=\"hidden\" name=\"postid\" value=\"$postid\" />\n";
	print "<input type=\"hidden\" name=\"commentid\" value=\"$commentid\" />\n";

    	if ($commentid) {
		&output_comment($postid,$commentid);
    	} else {
		&output_post($postid,$commentid);
	
	}
}

sub output_comment {
	my $postid= shift;
	my $commentid=shift;

	my $query=$dbh->prepare("
	              select * from comments
		       where postid=? and commentid=? "); 
	$query->execute ($postid, $commentid);
	my $result=$query->fetchrow_hashref;

	$result->{comment} =~ s#\r\n#</p>\n\n<p>\n#g;
	$result->{comment} =~ s#<p>\n</p>\n\n<p>\n#<p>\n#g;
	print <<EOentry;
	<div class="entry">
		$result->{comment}
	</div>
EOentry
	return;
}

sub output_post {
	my $postid= shift;

	my $query=$dbh->prepare("
	              select * from posts
		       where postid=?"); 
	$query->execute ($postid);
	my $result=$query->fetchrow_hashref;

	$result->{why} =~ s#\n#</p>\n\n<p>\n#g;
	$result->{why} =~ s#<p>\n</p>\n\n<p>\n#<p>\n#g;

	print <<EOentry;
		<strong>$result->{title}</strong>
		<p>$result->{why}</p>
EOentry
}



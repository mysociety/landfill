#!/usr/bin/perl -I ../../web/cgi-bin

use warnings;
use strict;
use DBI;
use HTML::Entities;
use HTML::Scrubber;
use Email::Valid;
use Mail::Mailer;
use CGI qw/param/;

use mysociety::NotApathetic::Config;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});
my $url_prefix= $mysociety::NotApathetic::Config::url;
my $admin_url_prefix= $mysociety::NotApathetic::Config::admin_url;
my $abuse_address= 'abuse'. $mysociety::NotApathetic::Config::email_domain; 

{
	&handle_abuse();
	print "Location: $admin_url_prefix/index.shtml\n\n";
}


sub handle_abuse {
    my $postid= param('postid') || return;
    my $commentid= param('commentid') || '';
	
    &hide_abuse($postid, $commentid);

    my $mailer= new Mail::Mailer 'sendmail';
    my %headers;
    my $address      = $abuse_address;
    my $name         = 'NotApathetic abuse report';

    $headers{"Subject"}= "Abuse moderation: http://www.notapathetic.com/" ;
    $headers{"To"}= "$name <$address>" ;
    $headers{'From'}= "Not Apathetic <$abuse_address>" ;
    $headers{"X-Originating-IP"}= $ENV{'HTTP_X_FORWARDED_FOR'}  || $ENV{'REMOTE_ADDR'} || return;
    $mailer->open(\%headers);


    print $mailer <<EOmail;

$ENV{REMOTE_USER} just decided that this post should disappear
	$admin_url_prefix/comments.shtml?$postid#$commentid

To bring it back, visit
	$admin_url_prefix/cgi-bin/unhide.cgi?postid=$postid;commentid=$commentid

EOmail

    $mailer->close;

    return;
}


sub die_cleanly {
	my $reason=shift;
	print "Content-Type: text/plain\n\n

	Your submission failed:
		$reason
	Please go back and correct this before submitting again.
	";
	exit(0);
}


sub hide_abuse {
	my $postid= shift;
	my $commentid= shift || '';

	
	my $postid_q= $dbh->quote($postid);
	my $commentid_q= '';
	if ($commentid  ne '') { 
		$commentid_q= $dbh->quote($commentid);
		$dbh->do("update comments set visible=0 where postid=$postid_q and commentid=$commentid_q");
		$dbh->do("update posts set commentcount=commentcount-1 where postid=$postid_q");
	} else {
		$dbh->do("update posts set hidden=1 where postid=$postid_q");
	}
}

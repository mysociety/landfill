#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;
use HTML::Scrubber;
use Email::Valid;
use CGI::Fast qw/param/;
use mysociety::NotApathetic::Config;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});
my $url_prefix= $mysociety::NotApathetic::Config::url;
my $abuse_address= 'abuse'. $mysociety::NotApathetic::Config::email_domain; 

while (new CGI::Fast()) {
	&send_email;
	print "Location: $url_prefix/abuse/sent/\r\n\r\n";
}


sub send_email {
    my $postid= param('postid') || return;
    my $commentid= param('commentid') || '';

    return unless ($postid =~ m#^\d+$#);
    return unless ($commentid=~ m#^\d*$#);

    print "Location: $url_prefix/abuse/?postid=$postid;commentid=$commentid\r\n\r\n";
    exit;

    use Mail::Mailer;
    my $mailer= new Mail::Mailer 'sendmail';#, Server => 'mailrouter.mcc.ac.uk';
    my %headers;
    my $address      = $abuse_address;
    my $name         = 'NotApathetic abuse report';

    $headers{"Subject"}= "NA Report of abuse http://www.notapathetic.com/admin/comments.shtml?$postid#$commentid" ;
    $headers{"To"}= "$name <$address>" ;
    $headers{'From'}= "Not Apathetic <$abuse_address>" ;
    $headers{"X-Originating-IP"}= $ENV{'HTTP_X_FORWARDED_FOR'}  || $ENV{'REMOTE_ADDR'} || return;
    $mailer->open(\%headers);


    print $mailer <<EOmail;

Someone things $url_prefix/admin/comments.shtml?$postid#$commentid is abusive
EOmail

    $mailer->close;

    return;
}


sub die_cleanly {
	my $reason=shift;
	print "Content-Type: text/plain\r\n\r\n

	Your submission failed:
		$reason
	Please go back and correct this before submitting again.
	";
	exit(0);
}

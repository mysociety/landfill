#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;
use HTML::Scrubber;
use Email::Valid;
use CGI qw/param/;
use mysociety::NotApathetic::Config;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});
my $url_prefix= $mysociety::NotApathetic::Config::url;

{
	&send_email;
	print "Location: $url_prefix/abuse/\n\n";
}


sub send_email {
    my $postid= param('postid') || return;
    my $commentid= param('commentid') || '';

    return unless ($postid =~ m#^\d+$#);
    return unless ($commentid=~ m#^\d*$#);

    use Mail::Mailer;
    my $mailer= new Mail::Mailer 'sendmail';#, Server => 'mailrouter.mcc.ac.uk';
    my %headers;
    my $address      = 'na@msmith.net';
    my $name         = 'NotApathetic abuse report';

    $headers{"Subject"}= "Report of abuse in http://www.notapathetic.com/comments/$postid#$commentid" ;
    $headers{"To"}= "$name <$address>" ;
    $headers{'From'}= 'Not Apathetic <na@msmith.net>' ;
    $headers{"X-Originating-IP"}= $ENV{'HTTP_X_FORWARDED_FOR'}  || $ENV{'REMOTE_ADDR'} || return;
    $mailer->open(\%headers);


    print $mailer <<EOmail;

Someone things $url_prefix/comments/$postid#$commentid is abusive
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

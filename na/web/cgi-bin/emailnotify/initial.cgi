#!/usr/bin/perl -I ../

use warnings;
use strict;
use DBI;
use HTML::Entities;
use HTML::Scrubber;
use Email::Valid;
use Text::Wrap;
use Mail::Mailer qw(sendmail);
use CGI qw/param/;
use mysociety::NotApathetic::Config;
use mysociety::NotApathetic::Routines;
my $url_prefix= $mysociety::NotApathetic::Config::url;

{
	my %Passed_Values;
	my $mailer= new Mail::Mailer 'sendmail';
        foreach my $param (param()) {
                $Passed_Values{$param}=param($param);
		if ($param ne 'email') {
			$Passed_Values{$param}=~ s#@# _at_ #g;
		}
        }

	if (not defined ($Passed_Values{email})) {
		print "Location: $url_prefix/emailnotify/\n\n";
		next;
	}

	my %headers;

    unless ((Email::Valid->address(-address => $Passed_Values{"email"},-mxcheck => 1 ))) {
	    &die_cleanly("email verification failed");
    }

	my $randomness = rand(); $randomness=~ s/^0\.(\d+)/$1/;
	$Passed_Values{authcode}= $randomness;
	my $authcode_q= $dbh->quote($randomness);

	my $email_q=$dbh->quote($Passed_Values{"email"});
	my $search_q=$dbh->quote($Passed_Values{"search"} || '');
	$dbh->do("
		insert into emailnotify set email=$email_q,
					    authcode=$authcode_q,
					    search=$search_q,
					    validated=0,
					    lastrun=0
		");

	my $rowid=$dbh->{insertid};
	my $site_name=mysociety::NotApathetic::Config::site_name;
	my $to_person = $Passed_Values{"email"} ;

	$headers{'To'}= "$to_person" ;
        $headers{"From"}= "\"$site_name\" <$email_noreply>" ;
	$headers{"Subject"}= "Confirm request for Updates from $site_name";
	$headers{"X-Originating-IP"}= $ENV{'REMOTE_ADDR'} || '';
	$mailer->open(\%headers);


print $mailer <<EOmail;

We've had a request for this address to receive notifications
of new posts on $site_name .

If you wish to confirm this, please click the below link:
	$url_prefix/emailnotify/confirm?u=$rowid\&c=$Passed_Values{authcode}

If you didn't request this (or have changed your mind), just
ignore this message. If you wish to report it, forward it to
$catch_all_address

EOmail

    	$mailer->close;

	print "Location: $url_prefix/emailnotify/emailsent/\n\n";

}

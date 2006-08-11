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
my $url_prefix= $mysociety::NotApathetic::Config::url;
my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $site_name= $mysociety::NotApathetic::Config::site_name;
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $email_noreply= $mysociety::NotApathetic::Config::email_noreply;
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 1});
my $catch_all_address = 'team@yourhistoryhere.com';

{
	my %Passed_Values;
	my $mailer= new Mail::Mailer 'sendmail';
        foreach my $param (param()) {
                $Passed_Values{$param}=param($param);
        }

	if (not defined ($Passed_Values{email})) {
		print "Location: $url_prefix/emailnotify/\n\n";
		next;
	}
    
	my %headers;

    unless ((Email::Valid->address(-address => $Passed_Values{"email"},-mxcheck => 1 ))) {
        if (defined $Passed_Values{error_redirect}) {
	    &die_cleanly("email verification failed");
        } else { # added after spam problem by SS
		&die_cleanly('dead');
	}
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

	my $to_person = $Passed_Values{"email"} ;

	$headers{'To'}= "$to_person" ;
        $headers{"From"}= "\"$site_name\" <$email_noreply>" ;
	$headers{"Subject"}= "Confirm request for Updates from $site_name";
	$headers{"X-Originating-IP"}= $ENV{'HTTP_X_FORWARDED_FOR'}  || $ENV{'REMOTE_ADDR'} || return;
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

sub die_cleanly {
        &mysociety::NotApathetic::Config::die_cleanly(@_);
}

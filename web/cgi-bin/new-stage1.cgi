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
my $email_domain= $mysociety::NotApathetic::Config::email_domain; 
my %State; # State variables during display.
my %Passed_Values;

while (new CGI::Fast()) {
	foreach my $param (param()) {
		$Passed_Values{$param}=param($param);
	}
	$Passed_Values{email} ||= '';
	$Passed_Values{title} ||= '';
	$Passed_Values{why} ||= '';

	&handle_comment;
	
	&send_email;
	print "Location: $url_prefix/new/checkemail/\r\n\r\n";
}


sub handle_comment {
	my %quoted;
	unless (Email::Valid->address( -address => $Passed_Values{email})) {
		&die_cleanly("Email address invalid");
	}

	my $scrubber= HTML::Scrubber->new();
	$scrubber->allow(qw[a em strong p br]);
	$scrubber->comment(0);

	foreach my $pv (keys %Passed_Values) {
		$Passed_Values{$pv}= $scrubber->scrub($Passed_Values{$pv});
		$quoted{$pv}= $dbh->quote($Passed_Values{$pv});
	}

	my $randomness = rand(); $randomness=~ s/^0\.(\d+)/$1/;
	$Passed_Values{authcode}= $randomness;
	my $auth_code_q= $dbh->quote($randomness);

        if ($Passed_Values{why} =~ m/(.{210}.+?\b)/) {
                $quoted{shortwhy}= $dbh->quote($1 . "...");
        } else  {
                $quoted{shortwhy}= $quoted{why};
        }

        if ($Passed_Values{why} =~ m/(.{15}.+?\b)/) {
                $quoted{title}= $dbh->quote($1 . "...");
        } else  {
                $quoted{title}= $quoted{why};
        }


	my $query=$dbh->prepare("
		insert into posts
		   set email=$quoted{email} ,
		       why=$quoted{why} ,
		       shortwhy=$quoted{shortwhy} ,
		       title=$quoted{title} ,
		       posted=now(),
		       authcode=$auth_code_q
	");

	$query->execute;
	$Passed_Values{rowid}= $dbh->{insertid};

}

sub send_email {
    use Mail::Mailer;
    my $mailer= new Mail::Mailer 'sendmail';#, Server => 'mailrouter.mcc.ac.uk';
    my %headers;
    my $address      = $Passed_Values{"email"} || 'nobody' . $email_domain;
    my $name         = $Passed_Values{"name"} || '';

    $headers{"Subject"}= 'Request to post to NotApathetic.com';
    $headers{"To"}= "$name <$address>" ;
    $headers{'From'}= "Not Apathetic <team$email_domain>";
    $headers{"X-Originating-IP"}= $ENV{'HTTP_X_FORWARDED_FOR'}  || $ENV{'REMOTE_ADDR'} || return;
    $mailer->open(\%headers);


    print $mailer <<EOmail;

Hi,

Someone has tried to post to NotApathetic.com using this address
on the topic of $Passed_Values{title}

If it wasn't you, just ignore it.

If this was you, and you wish to confirm the account, please click on
the following link
        $url_prefix/cgi-bin/new-confirm.cgi?u=$Passed_Values{rowid};c=$Passed_Values{authcode}

Thank you
NotApathetic.com

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

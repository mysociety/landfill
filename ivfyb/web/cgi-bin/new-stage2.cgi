#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;
use HTML::Scrubber;
use Email::Valid;
use CGI::Fast qw/param/;
use mysociety::IVotedForYouBecause::Config;

my $dsn = $mysociety::IVotedForYouBecause::Config::dsn; # DSN connection string
my $db_username= $mysociety::IVotedForYouBecause::Config::db_username;              # database username
my $db_password= $mysociety::IVotedForYouBecause::Config::db_password;         # database password
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});
my $url_prefix= $mysociety::IVotedForYouBecause::Config::url;
my %Passed_Values;

begin:

while (new CGI::Fast()) {
	foreach my $param (param()) {
		$Passed_Values{$param}=param($param);
	}
	$Passed_Values{sex} ||= '';
	$Passed_Values{age} ||= '';
	$Passed_Values{title} ||= '';
	$Passed_Values{ethgroup} ||= '';
	$Passed_Values{nochildren} ||= '';
	$Passed_Values{region} ||= '';
	$Passed_Values{title} ||= '';
	$Passed_Values{evervoted} ||= '';
	$Passed_Values{postcode} ||= '';

	&handle_comment;
	
	print "Location: $url_prefix/comments/$Passed_Values{postid}\r\n\r\n";
}


sub handle_comment {
	my %quoted;

	my $scrubber= HTML::Scrubber->new();
	$scrubber->allow(qw[a em strong p br]);
	$scrubber->comment(0);

	foreach my $pv (keys %Passed_Values) {
		$Passed_Values{$pv}= $scrubber->scrub($Passed_Values{$pv});
		$quoted{$pv}= $dbh->quote($Passed_Values{$pv});
	}

	my $randomness = rand(); $randomness=~ s/^0\.(\d+)/$1/;
	my $auth_code_q= $dbh->quote($randomness);
        my $email_alert=0;
        $email_alert=1 if (defined $Passed_Values{'emailalert'});

	my $query=$dbh->prepare("
		update posts
		   set age=$quoted{age} ,
		       sex=$quoted{sex} ,
		       emailalert='$email_alert',
		       title=$quoted{title} ,
		       region=$quoted{region} ,
		       evervoted=$quoted{evervoted} ,
		       ethgroup=$quoted{ethgroup} ,
		       nochildren=$quoted{nochildren} ,
		       postcode=$quoted{postcode},
		       posted=now(),
		       authcode=$auth_code_q
		 where postid=$quoted{postid}
  		   and authcode=$quoted{authcode}
	");

	$query->execute;
}


sub die_cleanly {
	my $reason=shift;
	print "Content-Type: text/plain\r\n\r\n

	Your submission failed:
		$reason
	Please go back and correct this before submitting again.
	";
	goto begin;
}

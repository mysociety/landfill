#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;
use HTML::Scrubber;
use Email::Valid;
use CGI qw/param/;
use CGI::Carp qw/fatalsToBrowser warningsToBrowser/;
use mysociety::NotApathetic::Config;
use mysociety::NotApathetic::Routines;
my $url_prefix= $mysociety::NotApathetic::Config::url;
my $site_name= $mysociety::NotApathetic::Config::site_name; 
my %Passed_Values;

{
	&setup_db();
	foreach my $param (param()) {
		$Passed_Values{$param}=param($param);
	}
	$Passed_Values{q1} ||= '';
	$Passed_Values{q2} ||= '';
	$Passed_Values{title} ||= '';
	$Passed_Values{q3} ||= '';
	$Passed_Values{q4} ||= '';
	$Passed_Values{q5} ||= '';
	$Passed_Values{q6} ||= '';
	$Passed_Values{'postcode'} ||= '';

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
		   set q1=$quoted{q1} ,
		       q2=$quoted{q2} ,
		       emailalert='$email_alert',
		       title=$quoted{title} ,
		       q3=$quoted{q3} ,
		       q4=$quoted{q4} ,
		       q5=$quoted{q5} ,
		       q6=$quoted{q6} ,
		       postcode=$quoted{'postcode'},
		       posted=now(),
		       authcode=$auth_code_q
		 where postid=$quoted{'postid'}
		   and site='$site_name'
  		   and authcode=$quoted{authcode}
	");

	$query->execute || die $dbh->errstr;
}


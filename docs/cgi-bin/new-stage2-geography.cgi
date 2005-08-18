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
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 1});
my $site_name= $mysociety::NotApathetic::Config::site_name;
my $url_prefix= $mysociety::NotApathetic::Config::url;
my %Passed_Values;

{
	foreach my $param (param()) {
		$Passed_Values{$param}=param($param);
	}
	$Passed_Values{title} ||= '';
	$Passed_Values{name} ||= 'Unknown';
	$Passed_Values{region} ||= '';
	$Passed_Values{location} ||= '';
	$Passed_Values{google_lat}= $Passed_Values{location} =~ m#x=(-?\d+\.\d+);#;
	$Passed_Values{google_long}= $Passed_Values{location} =~ m#y=(-?\d+\.\d+)#;

	&handle_comment;
	
	print "Location: $url_prefix//$Passed_Values{postid}\r\n\r\n";
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
		   set title=$quoted{title} ,
		       name=$quoted{name},
		       emailalert=$email_alert,
		       posted=now(),
		       authcode=$auth_code_q,
		       original_geography=$quoted{location},
		       google_lat=$quoted{google_lat},
		       google_long=$quoted{google_long}
		 where postid=$quoted{postid}
  		   and authcode=$quoted{authcode}
		   and site='$site_name'

	");

	$query->execute;
}


sub die_cleanly {
        &mysociety::NotApathetic::Config::die_cleanly(@_);

}

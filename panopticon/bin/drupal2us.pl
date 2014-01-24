#!/usr/bin/perl

use warnings;
use strict;

use DBI;
use PanopticonConfig;
my $new;
my $drupal;
my $r;
$drupal=DBI->connect('DBI:mysql:old_panopticon:localhost', $db_username, $db_password, {RaiseError => 1});
my $drupal_query= $drupal->prepare("select title,link,description,timestamp,fid from aggregator_item ");
$drupal_query->execute || die $drupal->errstr;
$new=$dbh;
my $new_query=$new->prepare("insert into entries set title=?, link=?,feedid=?, content=?, shortcontent=?, pubdate=?, first_seen=?, last_seen=?");



my %lookup;
$lookup{5}=4;# commentonpower.org |
$lookup{6}=5;# Directionlessgov   |
$lookup{4}=6;# hassleme.co.uk     |
$lookup{1}=7;# HearFromYourMP     |
$lookup{12}=8;# mySociety          |
$lookup{7}=9;# Placeopedia        |
$lookup{11}=10;# PledgeBank         |
$lookup{9}=11;# publicwhip         |
$lookup{8}=12;# TheyWorkForYou     |
$lookup{3}=13;# Tom Steinberg      |
$lookup{2}=14;# WriteToThem        |
$lookup{10}=15;# yourhistoryhere    |


while ($r= $drupal_query->fetchrow_hashref) {
	$new_query->execute(
		$r->{title},
		$r->{link},
		$lookup{$r->{fid}},
		$r->{description},
		$r->{description},
		$r->{timestamp},
		$r->{timestamp},
		$r->{timestamp}
	) || warn $new->errstr;
}


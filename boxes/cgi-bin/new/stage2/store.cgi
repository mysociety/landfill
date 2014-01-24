#!/usr/bin/perl -I ../../

use warnings;
use strict;
use mySociety::Boxes::Config;
use mySociety::Boxes::Routines;
my %Passed_Values;

foreach my $p (param()) {
	$Passed_Values{$p}= param($p);
}

my $postcode=$Passed_Values{"postcode"};
my $email=$Passed_Values{"email"};
my $constituency_id= &get_constituencyid_from_postcode($postcode);

$dbh->do("insert into boxinfo set email=?, postcode=?, created=now()", undef, $email, $postcode) || die "query didn't work : " . $dbh->errstr;
my $boxid= $dbh->{mysql_insertid};

foreach my $feed (keys %Passed_Values) {
	next if ($feed eq "email");
	next if ($feed eq "postcode"); # things which aren't feeds
	my $tag='';
	foreach my $sitename (keys %Site_Feed_Names) {
		if ($sitename eq $feed) {
			$tag= $feed;
			$Site_Feed_Names{$sitename}=~ s/POSTCODE/$postcode/;
			$Site_Feed_Names{$sitename}=~ s/CONSTITUENCYID/$constituency_id/;

			$feed=$Site_Feed_Names{$sitename};
		}
	}
	&add_feed_to_box($boxid, $feed, $tag);
}


# print "Content-Type: text/html\n\n";
print "Location: /showfeed/?boxid=$boxid\n\n";


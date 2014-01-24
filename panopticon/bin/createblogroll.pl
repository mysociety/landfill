#!/usr/bin/perl 

BEGIN {push @INC, $ENV{PANOPTICON_PATH}.'/bin/'}

use warnings;
use strict;
use PanopticonConfig;

{
	my $feeds_query=$dbh->prepare(" select  feedid, feedurl, tag  from  feeds where active=1");
	$feeds_query->execute || die $dbh->errstr;
	while (my $r= $feeds_query->fetchrow_hashref) {
		print "<li><a href=\"<!--#echo var=\"root_prefix\"-->?$r->{tag}\">$r->{tag}</a> (<a href=\"$r->{feedurl}\">src</a>)</li>\n";
	}
}


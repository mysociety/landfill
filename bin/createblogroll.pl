#!/usr/bin/perl 

BEGIN {push @INC, $ENV{PANOPTICON_PATH}.'/bin/'}

use warnings;
use strict;
use PanopticonConfig;

{
	my $feeds_query=$dbh->prepare(" select  feedid, feedurl, tag  from  feeds ");
	$feeds_query->execute || die $dbh->errstr;
	while (my $r= $feeds_query->fetchrow_hashref) {
		print "<li><a href=\"$r->{feedurl}\">$r->{tag}</a></li>\n";
	}
}


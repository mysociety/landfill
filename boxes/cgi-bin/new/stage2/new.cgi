#!/usr/bin/perl -I../../

use warnings;
use strict;
use CGI qw/param/;
use DBI;
use mySociety::Boxes::Config;
use mySociety::Boxes::Routines;

print "Content-Type: text/html\n\n";

my $postcode=param("postcode");

my $constituency_id= &get_constituencyid_from_postcode($postcode);

my $query= $dbh->prepare("select feeds.*, feedinfo.title, feedinfo.description from feeds,feedinfo where constituencyid=? and feeds.feedid=feedinfo.feedid");
$query->execute($constituency_id);

foreach my $result ($query->fetchrow_hashref) {
	print <<EOhtml
	<li><input type="checkbox" name="$result->{feedid}"> $result->{title}</li>
EOhtml
}

print "<input type='hidden' name='postcode' value=\"$postcode\" />\n";



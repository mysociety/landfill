#!/usr/bin/perl

use warnings;
use strict;
use FindBin;
use lib "$FindBin::Bin/../../perllib";
use lib "$FindBin::Bin/../../../perllib";
use mySociety::Config;
BEGIN {
    mySociety::Config::set_file("$FindBin::Bin/../../conf/general");
}
use PoP;
use CGI qw/param/;
use HTML::Entities;
use XML::Simple;
use URI::Escape;
use CGI::Fast;


my $site_name= mySociety::Config::get('SITE_NAME');


while (my $q = new CGI::Fast()) {
        print "Content-Type: application/vnd.google-earth.kml+xml\r\n\r\n";
	my $limiter='';
	my $search_term = &handle_search_term($q); #' 1 = 1 ';

	unless ($dbh and $dbh->ping()) {
		# recreate db connection
		use PoP; # reload it.
	}
	my ($topleft_long,$bottomright_lat,$bottomright_long,$topleft_lat) = split(/,/, $q->param('BBOX')) if (defined($q->param('BBOX')));


	if ( defined($topleft_lat) and defined($topleft_long) and
	     defined($bottomright_lat) and defined($bottomright_long)) {
	     $topleft_lat=~ s#[^-\.\d]##g;
	     $topleft_long=~ s#[^-\.\d]##g;
	     $bottomright_lat=~ s#[^-\.\d]##g;
	     $bottomright_long=~ s#[^-\.\d]##g;
		$limiter= <<EOSQL;
	and google_lat <= $topleft_lat and google_lat >= $bottomright_lat
	and google_long >= $topleft_long and google_long <= $bottomright_long
EOSQL
	}
	my $query=$dbh->prepare("
	              select google_lat, google_long, title
			from posts
	 	       where validated=1
			 and hidden=0
                         and site='$site_name'
			     $search_term
			     $limiter
		    order by posted
			     desc limit 50
		       "); # XXX order by first_seen needs to change


	$query->execute;
	my $result;
        print '<?xml version="1.0" encoding="UTF-8"?>'."\n".'<kml xmlns="http://earth.google.com/kml/2.0">' . "\n<Folder><name>Latest entries on Placeopedia</name><description>The 50 latest additions to placeopedia.com</description>\n";

	my ($lat, $long, $title);

	while (($lat, $long, $title) = $query->fetchrow_array ) {
            my $wikiuri = $title;
            $wikiuri =~ tr/ /_/;
            $wikiuri = uri_escape($wikiuri);
            print "<Placemark>\n";
	    print "\t<description><![CDATA[<a href=\"http://en.wikipedia.org/wiki/$wikiuri\">Wikipedia article</a>.]]></description>\n";
            print "\t<name>" . encode_entities($title, '<&>') . "</name>\n";
	    print "\t<LookAt>\n";
	    print "\t\t<longitude>$lat</longitude>\n";
	    print "\t\t<latitude>$long</latitude>\n";
	    print "\t\t<range>0</range>\n";
	    print "\t\t<tilt>0</tilt>\n";
	    print "\t\t<heading>3</heading>\n";
	    print "\t</LookAt>\n";
            print "\t<Point>\n";
            print "\t\t<coordinates>$long,$lat,0</coordinates>\n";
            print "\t</Point>\n";
            print "</Placemark>\n";
	}
	print "</Folder>\n</kml>\n";
}



sub handle_search_term {
	my $q= shift;
	my $search_path= $q->param('q') || '';
	my @search_fields= ('posts.region',
			    'posts.why',
			    'posts.title'
			    );
	return ('') if ($search_path eq '');

        my (@or)= split /\//, $search_path;

        my $limiter= ' and ( 1 = 0 '; # skipped by the optimiser but makes
                                # syntax a lot easier to get right

        foreach my $or (@or) {
                # a/b/c+d/e
                # a or b or (c and d) or e
                next if $or =~ /^\s*$/;
                my $or_q = $dbh->quote("\%$or\%");

                if ($or =~ /\+/) {
                        my @and= split /\+/, $or;
                        $limiter.= " or  ( 1 = 1 ";
                        foreach my $and (@and) {
                                my $and_q = $dbh->quote("\%$and\%");
                                $limiter.= " and ( 1=0 ";
                                foreach my $field (@search_fields) {
                                        $limiter.= " or $field like $and_q ";
                                }
                                $limiter.= " ) \n";
                        }
                        $limiter .= ' ) ';
                } else  {
                        $limiter .= " or ( 1=0 " ;
                                foreach my $field (@search_fields) {
                                        $limiter.= " or $field like $or_q ";
                                }
                        $limiter .= ' ) ';
                }

        }
        $limiter .= " )\n\n ";

        return $limiter;

}





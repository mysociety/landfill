#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use CGI qw/param/;
use HTML::Entities;
use XML::Simple;
use mysociety::NotApathetic::Config;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $site_name= $mysociety::NotApathetic::Config::site_name;

my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 1});
my $search_term = &handle_search_term(); #' 1 = 1 ';

my ($topleft_long,$bottomright_lat,$bottomright_long,$topleft_lat) = split(/,/, param('BBOX')) if (defined(param('BBOX')));
my $limiter='';

{
        print "Content-Type: application/vnd.google-earth.kml+xml\r\n\r\n";


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
	              select postid, why, posted, shortwhy,
		             title, commentcount, google_lat, google_long
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
	my $results;
        print '<?xml version="1.0" encoding="UTF-8"?>'."\n".'<kml xmlns="http://earth.google.com/kml/2.0">' . "\n<Folder><name>Latest entries on YourHistoryHere</name><description>The 50 latest additions to yourhistoryhere.com</description>\n";
	while ($result=$query->fetchrow_hashref() ) {
            my $shortwhy = $result->{shortwhy} || '';
		print "<Placemark>\n";
		print "\t<description><![CDATA[$shortwhy <a href=\"http://www.yourhistoryhere.com/comments?$result->{postid}\">more</a>.]]></description>\n";
		print "\t<name>$result->{title}</name>\n";
		print "\t<LookAt>\n";
		print "\t\t<longitude>$result->{google_long}</longitude>\n";
		print "\t\t<latitude>$result->{google_lat}</latitude>\n";
		print "\t\t<range>0</range>\n";
		print "\t\t<tilt>0</tilt>\n";
		print "\t\t<heading>3</heading>\n";
		print "\t</LookAt>\n";
                print "\t<Point>\n";
                print "\t\t<coordinates>$result->{google_long},$result->{google_lat},0</coordinates>\n";
                print "\t</Point>\n";
		print "</Placemark>\n";
	}
	print "</Folder>\n</kml>\n";
}



sub handle_search_term {
	my $search_path= param('q') || '';
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





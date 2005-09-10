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
use HTML::Entities;
use XML::RSS;
my $url_prefix= mySociety::Config::get('URL');
my $email_domain= mySociety::Config::get('EMAIL_DOMAIN');
my $site_name= mySociety::Config::get('SITE_NAME');

if ((defined $ENV{QUERY_STRING}) and ($ENV{QUERY_STRING} =~/^\d+$/)){
	print "Location: $url_prefix/cgi-bin/rss-comments.cgi?$ENV{QUERY_STRING}\r\n\r\n";
}

my %State; # State variables during display.
my $search_term = &handle_search_term(); #' 1 = 1 ';


{
        print "Content-Type: text/xml\r\n\r\n";
	my $query=$dbh->prepare("
	              select postid,
			     posts.title as title,
			     why,
			     shortwhy,google_lat, google_long,
			     date_format(posted, \"%H:%i, %e %M\") as posted_formatted 
			from posts
	 	       where validated=1
                         and hidden=0
                         and site='$site_name'
			     $search_term
		    order by posted
			desc limit 50
		       "); # XXX order by first_seen needs to change


	$query->execute;
	my $result;
	my $google_terms;
	my $comments_html;
	my $date_html;
 use XML::RSS;
 my $rss = new XML::RSS (version => '1');
 $rss->add_module(prefix=>'geo', uri=>'http://www.w3.org/2003/01/geo/wgs84_pos#');

 $rss->channel(
   title        => "$site_name",
   link         => "$url_prefix",
   description  => "New posts on $site_name",
   dc => {
     creator    => "team$email_domain",
     publisher  => "team$email_domain",
     language   => 'en-gb',
     ttl        =>  600
   },
   syn => {
     updatePeriod     => "daily",
     updateFrequency  => "1",
     updateBase       => "1901-01-01T00:00+00:00",
   },
);

	while ($result=$query->fetchrow_hashref) {
            $rss->add_item(
 		       title => "$result->{title}",
                       link => "http://en.wikipedia.org/wiki/$result->{title}",
                       description=> "$result->{shortwhy}",
		       geo => {lat => "$result->{google_lat}",
		       	       long => "$result->{google_long}"},
                );
        }

        print $rss->as_string;

}



sub date_header {
	my $date= shift; # in mysql timestampe format
	my ($thisday)= $date =~ /^(\d{8})/;

	#if ($State{"thisday"} == 'first') {
	#	$State{"thisday"}= $thisday;
	#	return ''; # we don't show the first day
	#}

	return '' if ($State{"thisday"} eq $thisday); #same as last

	my @months=('','January','February','March','April','May','June',
	            'July','August','September','October','November','December');

	# day has changed
	$State{"thisday"}= $thisday;

	my ($year, $month, $day)= $thisday =~ /^(\d{4})(\d{2})(\d{2})/;
	return "<h2>$day $months[$month] </h2>\n";
}


sub handle_search_term {
	my $search_path= $ENV{"QUERY_STRING"} || '';
	my @search_fields= ('posts.why', 
			    'posts.title');
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






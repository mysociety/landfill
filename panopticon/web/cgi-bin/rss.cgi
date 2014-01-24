#!/usr/bin/perl

BEGIN {push @INC, '/home/sebastian/sams/work/democracy.org.uk/panopticon/bin/'}

use warnings;
use strict;
use CGI::Carp qw/fatalsToBrowser warningsToBrowser/;
use HTML::Entities;
use PanopticonConfig;
my %State; # State variables during display.
my $search_term =''; # &handle_search_term(); #' 1 = 1 ';
use HTML::Entities;
use XML::RSS;

if ((defined $ENV{QUERY_STRING}) and ($ENV{QUERY_STRING} =~/^\d+$/)){
	print "Location: /?$ENV{QUERY_STRING}\n\n";
}

print "Content-Type: text/xml\n\n";
{
	my $query=$dbh->prepare("
	              select entryid,
			     entries.title as title,
			     subject,
			     link,
			     entries.feedid,
			     commentcount,
			     shortcontent,
			     first_seen,
			     date_format(first_seen, \"%H:%i, %e %M\") as first_seen_formatted ,
			     feedinfo.title as feed_name,
			     feedinfo.siteurl as feed_site_url
			from entries, feedinfo
	 	       where entries.feedid=feedinfo.feedid
		         and visible=1
			     $search_term
		    order by first_seen
			desc limit 100
		       "); # XXX order by first_seen needs to change


	$query->execute;
	my $result;
 use XML::RSS;
 my $rss = new XML::RSS (version => '1');
 $rss->channel(
   title        => "A Panopticon Feed",
   link         => "http://panopticon.mysociety.org/",
   description  => "Who is talking about mysociety.",
   dc => {
     subject    => "Government news",
     creator    => 'rss@thegovernmentsays.com',
     publisher  => 'rss@thegovernmentsays.com',
     rights     => 'Copyright The Government Says 2004-2005',
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
                       link => "$result->{link}",
                       description=> "$result->{shortcontent}"
                );
        }

        print $rss->as_string;

}




sub handle_search_term {
	my $search_path= $ENV{"QUERY_STRING"};
	my @search_fields= ('entries.content', 'entries.subject',
			    'entries.title', 'feedinfo.description',
			    'feedinfo.title');
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






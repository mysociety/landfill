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
use XML::Simple;

my $site_name= mySociety::Config::get('SITE_NAME');
my %State; # State variables during display.
my $search_term = &handle_search_term(); #' 1 = 1 ';


{
        print "Content-Type: text/xml\r\n\r\n";

	my $query=$dbh->prepare("
	              select postid, why, posted,title,commentcount,google_lat, google_long
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
	my $results=$query->fetchall_arrayref({});
	print XMLout($results, (GroupTags => {'anon' => 'post'}, KeyAttr=>"postid", NoAttr=>1, RootName=>"yourhistorythere"));
}



sub handle_search_term {
	my $search_path= $ENV{"QUERY_STRING"} || '';
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





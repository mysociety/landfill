#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;
use XML::Simple;
use mysociety::NotApathetic::Config;
use CGI qw/param/;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $site_name= $mysociety::NotApathetic::Config::site_name;
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 1});
my %State; # State variables during display.
my $search_term = &handle_search_term(); #' 1 = 1 ';


{
        print "Content-Type: text/xml\r\n\r\n";
	my $topleft_lat=param('topleft_lat');
	my $topleft_long=param('topleft_long');
	my $bottomright_lat=param('bottomright_lat');
	my $bottomright_long=param('bottomright_long');

	my $geog_limiter='';
        if ( defined($topleft_lat) and defined($topleft_long) and
             defined($bottomright_lat) and defined($bottomright_long)) {
             $topleft_lat=~ s#[^-\.\d]##g;
             $topleft_long=~ s#[^-\.\d]##g;
             $bottomright_lat=~ s#[^-\.\d]##g;
             $bottomright_long=~ s#[^-\.\d]##g;
                $geog_limiter= <<EOSQL;
        and google_lat <= $topleft_lat and google_lat >= $bottomright_lat
        and google_long >= $topleft_long and google_long <= $bottomright_long
EOSQL
        }
	my $query=$dbh->prepare("
	              select postid, why, posted,title,commentcount,google_lat, google_long
			from posts
	 	       where validated=1
			 and hidden=0
                         and site='$site_name' 
			     $geog_limiter
			     $search_term
		    order by posted
			     desc limit 25
		       "); # XXX order by first_seen needs to change


	$query->execute;
	my $result;
	my $google_terms;
	my $comments_html;
	my $date_html;
	my $show_link;
	my $more_link;
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





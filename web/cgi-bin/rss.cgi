#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;
use XML::RSS;
use mysociety::NotApathetic::Config;
use CGI::Fast;
my $url_prefix= $mysociety::NotApathetic::Config::url;
my $email_domain= $mysociety::NotApathetic::Config::email_domain;

if ((defined $ENV{QUERY_STRING}) and ($ENV{QUERY_STRING} =~/^\d+$/)){
	print "Location: $url_prefix/cgi-bin/rss-comments.cgi?$ENV{QUERY_STRING}\r\n\r\n";
}

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 1});
my %State; # State variables during display.
my $search_term = &handle_search_term(); #' 1 = 1 ';


while (new CGI::Fast()) {
print "Content-Type: text/xml\r\n\r\n";
	my $query=$dbh->prepare("
	              select postid,
			     posts.title as title,
			     why,
			     shortwhy,
			     date_format(posted, \"%H:%i, %e %M\") as posted_formatted 
			from posts
	 	       where validated=1
                             and hidden=0
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
 $rss->channel(
   title        => "NotApathetic.com",
   link         => "$url_prefix",
   description  => "New posts on $url_prefix .",
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
                       link => "$url_prefix/comments/$result->{postid}",
                       description=> "$result->{shortwhy}"
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
	my $search_path= $ENV{"QUERY_STRING"};
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






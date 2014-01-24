#!/usr/bin/perl

BEGIN {push @INC, '/home/sebastian/sams/work/democracy.org.uk/panopticon/bin/'}

use warnings;
use strict;
use CGI::Carp qw/fatalsToBrowser warningsToBrowser/;
use HTML::Entities;
use PanopticonConfig;
my %State; # State variables during display.
$State{'thisday'}='first';
my $search_term = &handle_search_term($ENV{'QUERY_STRING'} || ''); # &handle_search_term(); #' 1 = 1 ';
{
	if (defined $ENV{'GATEWAY_INTERFACE'} ) {
		 print "Content-Type: text/html\n\n";
	}
	
	if ($search_term) {
		my $human_search = $ENV{QUERY_STRING} ;
		$human_search =~ s#/# #;
		print "<strong>Results for $human_search</strong>\n";
	}
	my $query= $dbh->prepare ("
	              select entryid,
			     entries.title as title,
			     subject,
			     link,
			     entries.feedid,
			     commentcount,
			     shortcontent,
			     content,
			     first_seen,
			     tag,
			     date_format(first_seen, \"%H:%i, %e %M\") as first_seen_formatted ,
			     feedinfo.title as feed_name,
			     feedinfo.siteurl as feed_site_url
			from entries, feedinfo, feeds
	 	       where entries.feedid=feedinfo.feedid
		       	 and entries.feedid=feeds.feedid
		         and visible=1
			 $search_term
		    order by first_seen
			     desc limit 100
		       "); # XXX order by first_seen needs to change

	$query->execute;
	my $result;
	my $google_terms;
	my $show_link;
	my $more_link;
	my $count=0;
	while ($result=$query->fetchrow_hashref) {

		next if ($result->{shortcontent} =~ m#HARD NEWS#);
		next if ($result->{shortcontent} =~ m#Source: www.writetothem.com#);
		print &date_header($result->{first_seen});

		($show_link) = $result->{link} =~ m#(http://[^/]+/)#;

		#if ($result->{content} eq $result->{shortcontent}) {
		#	$more_link= $result->{link};
		#} else  {
		#	$more_link= "comments/?$result->{entryid}";
		#}

		$more_link= $result->{link};
		
		my $cache_link='';

		if (defined $ENV{"PANOPTICON_CACHE_PATH"} and $ENV{"PANOPTICON_CACHE_PATH"} ne '') {
				$cache_link= " (<a href=\"cache/$result->{entryid}.html\">cached</a>) ";
		}
		print <<EOfragment;
	<div class="entry entry_$count">
		<a href="$result->{link}" class="panopticon_title">$result->{title}</a>
		<span class="panopticon_short">$result->{shortcontent}</span>
		<a href="$more_link">Read more</a><br />
		<span align="right">
		<small>seen at  $result->{first_seen_formatted}
		for
		<a href="$result->{feed_site_url}">$result->{feed_name}</a> for <strong>$result->{tag}</strong>
		$cache_link
		.
		</small>
		</span>
	</div>
		<br />
EOfragment
		$count++;
		if ($count==2){$count=0};
	}

}



sub date_header {
	my $date= shift; # in mysql timestampe format
	my ($year, $month, $day)= $date =~ /^(\d{4})\-(\d{2})\-(\d{2})/;
	my $thisday= "$1$2$3";

	#if ($State{"thisday"} == 'first') {
	#	$State{"thisday"}= $thisday;
	#	return ''; # we don't show the first day
	#}

	return '' if ($State{"thisday"} eq $thisday); #same as last

	my @months=('','January','February','March','April','May','June',
	            'July','August','September','October','November','December');

	# day has changed
	$State{"thisday"}= $thisday;

	return "<h3>$day $months[$month] </h3>\n";
}


sub handle_search_term {
	my $search_path= $ENV{"QUERY_STRING"} || '';

	return ('') if ($search_path eq '');

	my @search_fields= ('entries.content', 'entries.subject',
			    'entries.title', 'feedinfo.description',
			    'feedinfo.title', 
			    'entries.link', 
			    'feeds.tag');

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

	# print STDERR $limiter;

        return $limiter;

}





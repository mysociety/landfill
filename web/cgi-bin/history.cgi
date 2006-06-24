#!/usr/bin/perl -w

BEGIN {push @INC, '/home/sebastian/sams/work/democracy.org.uk/panopticon/bin/'}

use strict;
use DBI;
use HTML::Entities;
use CGI::Carp qw/fatalsToBrowser/;
use PanopticonConfig;
my %State; # State variables during display.
#$State{"thisday"}='first';
#print "Content-Type: text/html\n\n";
my ($date)= $ENV{REQUEST_URI} =~ m#/([^/]+?)$#;
#$date=~ s#\D##g;
#print "$date";
if (defined $ENV{REQUEST_METHOD}) {print "Content-Type: text/html\n\n";};

my $search_term = &handle_search_term(); #' 1 = 1 ';

{

	my $days=8;
	 if ($search_term) {$days=90};

	my $query=$dbh->prepare("
	              select entryid,
			     entries.title as title,
			     subject,
			     link,
			     entries.feedid,
			     commentcount,
			     shortcontent,
			     content,
			     first_seen,
			     date_format(first_seen, \"%H:%i, %e %M\") as first_seen_formatted ,
			     feedinfo.title as feed_name,
			     feedinfo.siteurl as feed_site_url
			from entries, feedinfo
	 	       where entries.feedid=feedinfo.feedid
		         and visible=1
			 and first_seen like '$date%'
			     $search_term
		    order by first_seen
			     desc limit 200");
	$query->execute || die $dbh->errstr;
	my $result;
	my $date_html;
	my $show_link;
	my $more_link;
	my $c=0;
	while ($result=$query->fetchrow_hashref) {
		if ($c){$c=0} else {$c=1};
		$date_html= &date_header($result->{first_seen});
		($show_link) = $result->{link} =~ m#(http://[^/]+/)#;

		#if ($result->{content} eq $result->{shortcontent}) {
		#	$more_link= $result->{link};
		#} else  {
		#	$more_link= "comments/?$result->{entryid}";
		#}

		$more_link= $result->{link};
		print <<EOfragment;
<div class="entry entry_$c">
	<a href="$result->{link}" target="govtsays"><strong>$result->{title}</strong></a><br />
	<span align="right">
	<small>seen at  $result->{first_seen_formatted} in
	<a href="$result->{feed_site_url}">$result->{feed_name}</a>.
	</small>
	</span>
	<blockquote>
		$result->{shortcontent}
		<a href="$more_link" target="govtsays">Read more</a>
		(<a href="../cache/$result->{entryid}.html">cache</a>)
	</blockquote>
</div>

EOfragment
	}

}



sub handle_links {


	return '' if (defined $ENV{NO_COMMENTING});

	my $item= shift;
	my $google_terms= encode_entities($item->{title});
	$google_terms=~ s# #\+#;

	my $html.=<<EOhtml;
	<a href="/comments/$item->{entryid}">Comment ($item->{commentcount})</a>,
	<a href="/email/$item->{entryid}">Email this</a>.
EOhtml

	return ($html);
}

sub date_header {
	my $date= shift; # in mysql timestampe format
	my ($year, $month, $day)= $date =~ /^(\d{4})(\d{2})(\d{2})/;
	my $thisday= join '', $year, $month, $day;
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
	return "<h2>$day $months[$month] $year </h2>\n";
}


sub handle_search_term {
	my $search_path= $ENV{"QUERY_STRING"} || '';
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

	my $search_twfy= $search_path;
	$search_twfy=~ s#/# #g;
print <<EOhtml;
<div id="divTwfuWrapper" style="float: right; padding: 0 0 0 8px; background-color: #E8FDCB; ">
	Search <a href="http://www.TheyWorkForYou.com"><em>TheyWorkFor</em><strong>You</strong>.com</a>:
	<form id="frmHansardSearch" title="Search everything said in Parliament since 2001" action="http://www.theyworkforyou.com/search/"method="get">
		<input id="s" name="s" class="twfuTextbox" title="Type what you're looking for" type="text" tabindex="3" size="15" maxlength="100" value="$search_twfy" />
		<input id="Submit2" name="Submit2" class="twfuButton" title="Submit search" tabindex="4" type="submit" value="Search" />
		<br/>
	</form>
</div> 
EOhtml
	#print STDERR $limiter;
        return $limiter;

}





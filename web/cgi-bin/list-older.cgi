#!/usr/bin/perl

use warnings;
use strict;
use CGI::Fast;
use DBI;
use Date::Manip;
use HTML::Entities;
use mysociety::NotApathetic::Config;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});
my %State; # State variables during display.
my $search_term = &handle_search_term(); #' 1 = 1 ';
our $url_prefix=$mysociety::NotApathetic::Config::url;

while (my $q = new CGI::Fast()) {

    if (defined $ENV{REQUEST_METHOD}) {
        print "Content-Type: text/html; charset=iso-8859-1\r\n\r\n";
    };

    {
        my $type = 'summary';
	my $search_bit = $ENV{"QUERY_STRING"} || "";
	my $limit = 20;
        my $offset = $limit;
	
	if ($search_bit =~ /^\d+$/){
            $type = 'details';
		$search_term = "";
		$offset = $search_bit;
	}
	if ($search_bit =~ /^QQQ(\d+)$/){
		$search_term = "";
		$offset += $1;
	}

	my $query=$dbh->prepare("
	              	select *
					from posts
	 	      		where validated=1
			 		and hidden=0
					$search_term
					order by posted
					desc limit $offset, $limit
					"); #  The above needs cranking to 25 from 2


	$query->execute;
	my $result;
	my $google_terms;
	my $comments_html;
	my $show_link;
	my $more_link;
        my $print_head = 0;
	while ($result=$query->fetchrow_hashref) {
            if (!$print_head) {
                $print_head = 1;
                print "<h2><a name=\"older\"></a>Older items:</h2>\n";
	        if ($type eq 'summary') {
		    print "<ul>\n";
        	}
            }

		$comments_html= &handle_links($result);

		#if ($result->{content} eq $result->{shortcontent}) {
		#	$more_link= $result->{link};
		#} else  {
		#	$more_link= "comments/?$result->{entryid}";
		#}

		$more_link= $result->{link};
		if ($type eq 'summary'){
			print <<EOfragment;
		<li><a href="$url_prefix/comments/$result->{postid}">$result->{title}</a></li>
EOfragment
		}
		else
		{
			$more_link= $result->{link};
			my $someday = UnixDate($result->{posted}, "%E %b %Y");
			my $responses = ($result->{commentcount} != 1) ? 'responses' : 'response';
			print <<EOfragment;
            <div class="entry">
                    <h4><a href="$url_prefix/comments/$result->{postid}">$result->{title}</a></h4>
                    <p class="nomargin">
                            $result->{shortwhy}
                    </p>
                    <div>
                            <small>
                                    written $someday 
                                    | <a href="$url_prefix/comments/$result->{postid}">read more</a> 
                                    | <a href="$url_prefix/comments/$result->{postid}\#comments">$result->{commentcount} $responses</a> 
                                    | <a href="/abuse/?postid=$result->{postid}">abusive?</a>
                            </small>
                    </div>
            </div>
EOfragment
		}
        }
	if ($query->rows > 0) {
            my $older = 0;
            my $newer = 0;
		if ($type eq 'summary'){
			print "</ul>\n";
			print "<p align=\"right\">";
                        $older = $offset;
                        $newer = $offset - $limit*2;
		} else {
                    print "<p>";
                    $older = $offset + $limit;
                    $newer = $offset - $limit;
                }

                if ($newer > -$limit) {
                    if ($newer <= 0) {
			print "<a href=\"/\">front page</a>";
                    } else {
			print "<a href=\"/older/$newer\">newer $limit entries</a>";
                    }
                    print '<br />';
                }
                if ($type eq 'summary' || $query->rows eq $limit) {
                    print "<a href=\"/older/$older\">older $limit entries</a></p>";
                }
	}
}
}


sub handle_links {

	return '' if (defined $ENV{NO_COMMENTING});

	my $item= shift;
	my $google_terms= encode_entities($item->{title});
	$google_terms=~ s# #\+#;

	my $html.=<<EOhtml;
	<a href="$url_prefix/na/comments/$item->{postid}">Comment ($item->{commentcount}),
	Trackback</a>,
	<a href="$url_prefix/na/email/$item->{postid}">Email this</a>.
EOhtml

	return ($html);
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





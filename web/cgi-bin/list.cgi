#!/usr/bin/perl

use warnings;
use strict;
use CGI::Fast qw/param/;
use Date::Manip;
use DBI;
use HTML::Entities;
use mysociety::NotApathetic::Config;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 1});
my %State; # State variables during display.
our $url_prefix=$mysociety::NotApathetic::Config::url;

while (my $q = new CGI::Fast()) {

    if (defined $ENV{REQUEST_METHOD}) {
        print "Content-Type: text/html; charset=iso-8859-1\r\n\r\n";
    };

    {
        my $type = param('type') || 'details';
        my $search_bit = param('search') || '';
        my $page = param('page') || 0;
        if ($search_bit =~ /\|(\d+)$/) {
            $page = $1;
            $search_bit =~ s/\|(\d+)$//;
        }
        my $search_term = handle_search_term($search_bit); #' 1 = 1 ';
        my $mainlimit = 20;
        my $limit = ($type eq 'details') ? $mainlimit : $mainlimit * 5;
        my $offset = $page * $mainlimit;
        $offset += $mainlimit if ($type eq 'summary');

        my $query=$dbh->prepare("
                          select *
                            from posts
                           where validated=1
                             and hidden=0
                                 $search_term
                        order by posted
                                 desc limit $offset, $limit
                           "); # XXX order by first_seen needs to change


        $query->execute;
        my $result;
        my $someday;
        my $google_terms;
        my $comments_html;
        my $show_link;
        my $more_link;
			
        my $printed = 0;
        $search_bit =~ s/\// /g;
        my $title;
        while ($result=$query->fetchrow_hashref) {

            if ($printed==0) {
                $printed = 1;
                if ($type eq 'summary' || !$search_bit) {
                    if ($page > 0 || $type eq 'summary') {
                        print "<h2><a name=\"older\"></a>Older items:</h2>\n";
                    } else {
                        print "<h2>They're <span>not voting</span> because...</h2>\n";
                    }
                }
                if ($type eq 'summary') {
                    print "<ul>\n";
                }
                if ($type eq 'details' && $search_bit ne '') {
        	    print '<p>Your search for "'.$search_bit.'" yielded the following results:</p>';
                }
	    }

            $comments_html= &handle_links($result);

            #if ($result->{content} eq $result->{shortcontent}) {
            #	$more_link= $result->{link};
            #} else  {
            #	$more_link= "comments/?$result->{entryid}";
            #}

            $title = $result->{title} || '&lt;No subject&gt;';
            $more_link= $result->{link};
            if ($type eq 'summary') {
                print <<EOfragment;
                <li><a href="$url_prefix/comments/$result->{postid}">$title</a></li>
EOfragment
            } else {
                $someday = UnixDate($result->{posted}, "%E %b %Y");
                my $responses = ($result->{commentcount} != 1) ? 'responses' : 'response';
                print <<EOfragment;
            <div class="entry small">
                    <h4><a href="$url_prefix/comments/$result->{postid}">$title</a></h4>
                    <p>
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
            my $url = ($search_bit ne '') ? '/search/?'.$search_bit.'|' : '/?';
            if ($type eq 'summary') {
                print "</ul>\n";
                my $older = $page + 6;
                print "<p align=\"right\"><a href=\"$url$older\">Even older entries</a></p>";
            } else {
                my $older = $page + 1;
                my $newer = $page - 1;
                print '<p align="center">';
                if ($newer == 0) {
                    my $fronturl = ($search_bit ne '') ? '/search/?'.$search_bit : '/';
                    print "<a href=\"$fronturl\">front page</a> | ";
                } elsif ($newer > 0) {
                    print "<a href=\"$url$newer\">newer $mainlimit entries</a> | ";
                }
                if ($type eq 'summary' || $query->rows eq $limit) {
                    print "<a href=\"$url$older\">older $mainlimit entries</a>";
                }
                print '</p>';
            }
        } elsif ($type eq 'details' && $search_bit ne '') {
            print "<p>Your search for " . $search_bit . " yielded no results.</p>";
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
	my $search_path= shift;
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





#!/usr/bin/perl

use warnings;
use strict;
use CGI::Fast;
use Date::Manip;
use DBI;
use HTML::Entities;
use mysociety::NotApathetic::Config;


my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});
my %State; # State variables during display.
my $search_term;
our $url_prefix=$mysociety::NotApathetic::Config::url;

while (my $q = new CGI::Fast()) {
    print "Content-Type: text/html; charset=iso-8859-1\r\n\r\n";
	$search_term = &handle_search_term(); #' 1 = 1 ';
    {

            my $query=$dbh->prepare("
                          select *
                            from posts
                           where validated=1
                             and hidden=0
                                 $search_term
                        order by posted
                                 desc limit 10
                           "); # XXX order by first_seen needs to change


            $query->execute;
            my $result;
            my $someday;
            my $parsed;
            my $google_terms;
            my $comments_html;
            my $show_link;
            my $more_link;
			
            my $printed = 0;
	    my $search_bit = $ENV{"QUERY_STRING"} || "";
            $search_bit =~ s/\// /g;

            while ($result=$query->fetchrow_hashref) {

		if ($printed==0 && $ENV{"QUERY_STRING"} ne ''){
		    print '<p>Your search for "'.$search_bit.'" yielded the following results:</p>';
                    $printed = 1;
		}

                    $comments_html= &handle_links($result);

                    #if ($result->{content} eq $result->{shortcontent}) {
                    #	$more_link= $result->{link};
                    #} else  {
                    #	$more_link= "comments/?$result->{entryid}";
                    #}

                    $more_link= $result->{link};
                    $someday = UnixDate($result->{posted}, "%E %b %Y");

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

	    if ($printed == 0 && $ENV{"QUERY_STRING"} ne '') {
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





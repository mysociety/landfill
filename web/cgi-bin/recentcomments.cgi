#!/usr/bin/perl

use warnings;
use strict;
use CGI qw/param/;
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

{
    if (defined $ENV{REQUEST_METHOD}) {
        print "Content-Type: text/html; charset=iso-8859-1\r\n\r\n";
    };

    {
        my $type = param('type') || 'details';
        my $search_bit = param('search') || '';
        my $page = param('page') || 0;
        my $mainlimit = 15;
	my $brief = 2; # mainlimit x brief entries displayed in the brief listing
        my $limit = ($type eq 'details') ? $mainlimit : $mainlimit * $brief;
	
	my $offset = $page * $mainlimit;
		my $interesting = "";
        
	my $query;
	
	if ($type eq 'details'){
		$query=$dbh->prepare("
		
			select
				posts.postid,
				posts.title,
				comments.posted as commentsdate,
				comments.comment,
				comments.commentid,
				comments.name
			from
				posts, comments	 
			where
				posts.postid = comments.postid and
				posts.validated=1 and
				posts.hidden=0 and
                             	comments.visible=1
			order by
                        	comments.posted desc
                        limit
                        	$offset, $limit
                           ");
       	}else{
       		$query=$dbh->prepare("
       		
			select
				distinct posts.postid,
				posts.title
			from
				(select postid from comments order by posted desc) as d,
				posts
			where
				d.postid = posts.postid
			limit
				$offset, $limit    	
			");
	}

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
                        print "<h2><a name=\"older\"></a>Popular posts:</h2>\n";
                    } else {
                        print "<h2>Recent <span>comments</span></h2>\n";
                    }
                }
                if ($type eq 'summary') {
                    print "<ul>\n";
                } else {
					print "<dl>\n";
				}
	    	}

            $title = encode_entities($result->{title}) || '&lt;No subject&gt;';
            $more_link= $result->{link};
            if ($type eq 'summary') {
                print <<EOfragment;
<li><a href="$url_prefix/comments/$result->{postid}">$title</a></li>
EOfragment
            } else {
				$someday = UnixDate($result->{commentsdate}, "%E %b %Y");
				my $comment = $result->{comment};
				$comment =~s/(\r\n){2,}/<\/p> <p>/g;
				$comment =~s/\r\n/<br \/>/g;
				if ($comment =~ m/(.{100}.+?\b)/) {
					$comment = $1 . "...";
				}
                print <<EOfragment;
	<dt><a href="$url_prefix/comments/$result->{postid}">$title</a></dt>
	<dd><p><em>$result->{name} replies:</em> $comment</p>
	<small>
	written $someday
	| <a href="/abuse/?postid=$result->{postid}&amp;commentid=$result->{commentid}">abusive?</a>
	| <a href="$url_prefix/comments/$result->{postid}#comment_$result->{commentid}">read comment</a>
	</small>
	</dd>
EOfragment
            }
        }
        if ($query->rows > 0) {
            my $url = '/oldercomments/';
		my $older = $page;
            if ($type eq 'summary') {
                print "</ul>\n";
                $older++;
                print "<p class=\"right\"><a href=\"$url$older\">Even older entries</a></p>";
            } else {
				print "</dl>\n";
                $older += 1;
                my $newer = $page - 1;
                print '<p class="center">';
                if ($newer == 0) {
                    my $fronturl = ($search_bit ne '') ? '/oldersearch/'.$search_bit : '/';
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

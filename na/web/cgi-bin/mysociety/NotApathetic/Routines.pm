#!/usr/bin/perl

our $dbh;
our $url_prefix;
our %State;

package mysociety::NotApathetic::Routines;

require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(&setup &setup_db &die_cleanly &listing 
	     &summary_listing &handle_links &run_query &date_header 
	     &details_listing &handle_search_term
	     &get_comments &show_comments_page  &show_comment_summary);  # symbols to export on request
use warnings;
use strict;
use HTML::Entities;
use Date::Manip;

my $site_name=$mysociety::NotApathetic::Config::site_name;

sub setup {
	if (defined $ENV{REQUEST_METHOD}) {
		print "Content-Type: text/html; charset=UTF8\r\n\r\n";
	}
	&setup_db;
}

sub setup_db {
	$dbh=DBI->connect($mysociety::NotApathetic::Config::dsn, $mysociety::NotApathetic::Config::db_username, $mysociety::NotApathetic::Config::db_password);
}

sub handle_links {
	return '' if (defined $ENV{NO_COMMENTING});

	my $item= shift;
	my $google_terms= encode_entities($item->{title});
	$google_terms=~ s# #\+#;

	my $html.=<<EOhtml;
	<a class="item_comments_link" href="$url_prefix/na/comments/$item->{postid}">Comment ($item->{commentcount}),
	Trackback</a>,
	<a class="item_email_link" href="$url_prefix/na/email/$item->{postid}">Email this</a>.
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
	my $search_path= shift || '';
	my @search_fields= ('posts.q1', # field one is searchable
			    'posts.why',
			    'posts.title'
			    ); # XXX make this reach from an array in the config file
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




sub run_query {
	my $options= shift;
	$options->{'busiest'} ||= '';
	$options->{'search_term'} ||= '';
	$options->{'offset'} ||= 0;
	$options->{'limit'} ||= $mysociety::NotApathetic::Config::main_display_limit;
    	my $limiter='';

       	unless ($options->{'show_all_posts'}) {
		$limiter= " limit $options->{'offset'}, $options->{'limit'} ";
	}

	# XXX these all need dbh->quoting if not empty; either here or earlier.

        my $query=$dbh->prepare("
                          select * from posts
                           where validated=1
		 		 $options->{'busiest'}
                             and site='$site_name'
                             and hidden=0
                                 $options->{'search_term'}
                        order by posted desc
				$limiter
                           ");

        $query->execute;

	return ($query, $options);
}


sub details_listing {
	my $query= shift;
	my $options= shift;

        my $printed = 0;
        $options->{'search_term'}=~ s/\// /g;
	if ($options->{'search_term'} ne '') {
        	print '<p>Your search for "'.$options->{'search_url'}.'" yielded the following results:</p>';
	}

        while (my $result=$query->fetchrow_hashref) {

	    unless ($printed) {print "<dl>\n";}
	    $printed++;

            my $title = encode_entities($result->{title}) || '&lt;No subject&gt;';
            my $more_link= $result->{link};
            my $someday = UnixDate($result->{posted}, "%E %b %Y");
            my $responses = ($result->{commentcount} != 1) ? 'responses' : 'response';
                print <<EOfragment;
<dt><a href="$url_prefix/comments/$result->{postid}">$title</a></dt>
<dd><p>$result->{shortwhy}</p>
<small class="items_link_text">
written $someday 
| <a class="item_read_more_link" href="$url_prefix/comments/$result->{postid}">read more</a> 
| <a class="item_read_more_link item_read_more_link_comments" href="$url_prefix/comments/$result->{postid}\#comments">$result->{commentcount} $responses</a> 
| <a class="item_abuse_link" href="/abuse/?postid=$result->{postid}">abusive?</a>
</small>
</dd>
EOfragment
        }

	if ($printed) {
		print "</dl>\n";
		&page_footer($printed, $options, '');
	} else {
            print "<p>Your search for " . $options->{'search_url'} . " yielded no results.</p>";
	}

	return ($printed)
}



sub page_footer {
	my $rows_printed= shift;
	my $options= shift;
	my $subset= shift || $options->{subset} || '';

	my $older= $options->{'page'} + 1;
	my $newer = $options->{'page'} - 1;

	print '<p align="center">';
	if ($newer <= 0) {
		print "<a href=\"/?$options->{'search_url'}\">front page</a> | ";
	} elsif ($newer > 0) {
		print "<a href=\"/older$subset/$newer?$options->{'search_url'}\">newer $options->{'limit'} entries</a> | ";
	}

	if ($rows_printed == $options->{'limit'}) {
		print "<a href=\"/older$subset/$older?$options->{'search_url'}\">older $options->{'limit'} entries</a>";
	}

	print '</p>';
}


sub summary_listing {
        my $query= shift;
        my $options= shift;
        print "<ul class=\"items_summary_listing\">\n";
        while (my $result=$query->fetchrow_hashref) {
                print <<EOfragment;
<li><a href="$url_prefix/comments/$result->{postid}">$result->{'title'}</a></li>
EOfragment

        }
        print "</ul>\n";
        #my $older= $options->{'page'}+1;
        #print "<p align=\"right\"><a href=\"/older$options->{'subset'}/$older\">Even older entries</a></p>"; # XXX is this right?
}


sub listing {
	my ($ref, $options)= @_;
	if ($options->{'listing_type'} eq 'summary') {
		return &summary_listing($ref, $options);
	} else {
		return &details_listing($ref, $options);
	}
}


sub die_cleanly {
        my $error= shift || 'no error given';
        print "Location: $url_prefix/error/?$error\n\n";
	exit(0);
}



sub get_comments {
		my $options= shift;
		my $limiter='';

		if ($options->{'offset'}) {
			$limiter = " offset $options->{'offset'} ";
		}

		if ($options->{'limit'}) {
			$limiter = " limit $options->{'limit'} ";
		}

		my $query=$dbh->prepare("
			select  posts.postid,
				posts.title,
				comments.posted as commentsdate,
				comments.comment,
				comments.commentid,
				comments.name
			from    posts, comments
			where   posts.postid = comments.postid and
				posts.validated=1 and
				posts.hidden=0 and
                             	comments.visible=1 and
				posts.site='$site_name' and
				comments.site='$site_name'
		       order by comments.posted desc
				$limiter
                           ");

        	$query->execute;
		return ($query);
}

sub show_comments_page {
	my $query= shift;
	my $options= shift;
	my $rows=0;

	while (my $result= $query->fetchrow_hashref) {
        	my $title = encode_entities($result->{title}) || '&lt;No subject&gt;';
		$rows++;
            	my $more_link= $result->{link};

		my $someday = UnixDate($result->{commentsdate}, "%E %b %Y");
		my $comment = $result->{comment};
		$comment =~ s/(\r\n){2,}/<\/p> <p>/g;
		$comment =~ s/\r\n/<br \/>/g;
		if ($comment =~ m/(.{100}.+?\b)/) {
			$comment = $1 . "...";
		}

                print <<EOfragment;
	<dt class="comment_title"><a href="$url_prefix/comments/$result->{postid}">$title</a></dt>
	<dd class="comment_body"><p><em>$result->{name} replies:</em> $comment</p>
	<small class="items_link_text">
	written $someday
	| <a class="item_abuse_link" href="/abuse/?postid=$result->{postid}&amp;commentid=$result->{commentid}">abusive?</a>
	| <a class="item_read_comments_link" href="$url_prefix/comments/$result->{postid}#comment_$result->{commentid}">read comment</a>
	</small>
	</dd>
EOfragment
        }


	if ($rows == 0) {
		print "<p>Your search for " . $options->{search_url}. " yielded no results.</p>";
	}

}

sub show_comment_summary {
	my $query= shift;

	print "<ul class=\"summary_list comment_summary_list\">\n";
	while (my $result= $query->fetchrow_hashref) {
               	print <<EOfragment;
	<li class="summary_item comment_summary_item"><a href="$url_prefix/comments/$result->{postid}">$result->{title}</a></li>
EOfragment
	}
	print "</ul>\n";
}


1;


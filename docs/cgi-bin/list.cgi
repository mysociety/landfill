#!/usr/bin/perl

use warnings;
use strict;
use CGI qw/param/;
use CGI::Carp qw/fatalsToBrowser/;
use Date::Manip;
use DBI;
use HTML::Entities;
use mysociety::NotApathetic::Config;
use Text::Wrap;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username; # database username
my $db_password= $mysociety::NotApathetic::Config::db_password; # database password
my $site_name= $mysociety::NotApathetic::Config::site_name;
my $dbh=DBI->connect($dsn, $db_username, $db_password);
my %State; # State variables during display.
our $url_prefix=$mysociety::NotApathetic::Config::url;
my $Js='';

{
    my $type = param('type') || 'details';
    if (defined $ENV{REQUEST_METHOD}) {
        if ($type eq 'xml') {
            print "Content-Type: application/xml\n\n";
        } else {
            print "Content-Type: text/html\n\n";
        }
    }

    {
        my $adding = 0;
        my $search_bit = param('search') || '';
        my $page = param('page') || 0;
        if ($page eq 'add') {
            $adding = 1;
            $page = 0;
        }
	my $topleft_lat=param('topleft_lat');
	my $topleft_long=param('topleft_long');
	my $bottomright_lat=param('bottomright_lat');
	my $bottomright_long=param('bottomright_long');

        if ($search_bit =~ /\|(\d+)$/) {
            $page = $1;
            $search_bit =~ s/\|(\d+)$//;
        }
        my $search_term = handle_search_term($search_bit); #' 1 = 1 ';
	my $geog_limiter='';
        if ( defined($topleft_lat) and defined($topleft_long) and
             defined($bottomright_lat) and defined($bottomright_long)) {
             $topleft_lat=~ s#[^-\.\d]##g;
             $topleft_long=~ s#[^-\.\d]##g;
             $bottomright_lat=~ s#[^-\.\d]##g;
             $bottomright_long=~ s#[^-\.\d]##g;
             $geog_limiter = " and google_lat <= $topleft_lat and google_lat >= $bottomright_lat";
             if ($topleft_long<-180 && $bottomright_long>180) {
             } elsif ($topleft_long<-180) {
                 $topleft_long+=360;
                 $where .= " and (google_long <= $bottomright_long or google_long >= $topleft_long)";
             } elsif ($bottomright_long>180) {
                 $bottomright_long-=360;
                 $where .= " and (google_long <= $bottomright_long or google_long >= $topleft_long)";
             } else {
                 $where .= " and google_long <= $bottomright_long and google_long >= $topleft_long";
             }
        }
         my $mainlimit = 15;
		my $brief = 1; # mainlimit x brief entries displayed in the brief listing
        my $limit = ($type eq 'details') ? $mainlimit : $mainlimit * $brief;
        my $offset = $page * $mainlimit;
		my $interesting = "";
		if (defined param('interest')){
			$interesting = "and (interesting=1 or commentcount >= 5)";
		}
        $offset += $mainlimit if ($type eq 'summary');

        my $query=$dbh->prepare("
                          select *
                            from posts
                           where validated=1
		   		 $interesting
                             and hidden=0
                             and site='$site_name'
				 $geog_limiter
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
        my $pointindex=0;
        my $new_html = '';
        while ($result=$query->fetchrow_hashref) {
            if ($printed==0) {
                $printed = 1;
                if ($type eq 'summary') {
                    print "<ul>\n";
                } elsif ($type eq 'details') {
		    print "<dl>\n";
	        } elsif ($type eq 'xml') {
                    print "<results>\n";
                }
                if ($type eq 'details' && $search_bit ne '') {
        	    print '<p>Your search for "'.$search_bit.'" yielded the following results:</p>';
                }
	    }

            $comments_html= &handle_links($result);

            #if ($result->{content} eq $result->{shortcontent}) {
            #	$more_link= $result->{link};
            #} else  {
            #	$more_link= "comments?$result->{entryid}";
            #}

            $more_link= $result->{link};
            $Text::Wrap::columns = 32;
            my $bubble = $result->{title} || '<No subject>';
            $bubble =~ s/\s+/ /g;
            $bubble = wrap('', '', $bubble);
            $bubble = encode_entities($bubble);
            $bubble =~ s/\n/<br>/g;
            my $content = encode_entities($result->{shortwhy});
            $content = fill('', '', $content);
            $content =~ s/\r?\n/<br>/g;
            if ($result->{why} ne $result->{shortwhy}) {
                $content .= " <a href=\"comments.shtml?$result->{postid}\">more</a>";
            } else {
                $content .= "<br><a href=\"comments.shtml?$result->{postid}\">comment / permalink</a>";
            }
            $bubble = "<b>$bubble</b><p>$content</p>";
            my $zoomlevel = $result->{google_zoom};
            $zoomlevel = 2 unless defined($zoomlevel);
            if ($type eq 'summary') {
                $title = encode_entities($result->{title}) || '&lt;No subject&gt;';
                print <<EOfragment;
<li><a href="$url_prefix/comments?$result->{postid}">$title</a></li>
EOfragment
            } elsif ($type eq 'details') {
                print dt_entry($result, $pointindex);
                $bubble =~ s/"/\\"/g;
                $Js.=<<EOjs;
marker[$pointindex] = createPin(new GPoint($result->{google_long}, $result->{google_lat}), $zoomlevel, "$bubble")
EOjs
            } elsif ($type eq 'xml') {
                print '<result lat="'.$result->{google_lat}.'" lng="'.$result->{google_long}.'" zoom="'.$zoomlevel.'"><![CDATA['.$bubble.']]></result>';
                $new_html .= dt_entry($result, $pointindex);
            }
            $pointindex++;
        }
        if ($query->rows > 0) {
            my $url = '/older/';
			
	    if ($search_bit ne ''){
			$url = '/oldersearch/'.$search_bit.'|';
	    }elsif (defined param('interest')){
			$url = '/olderbusiest/';
	    }
			
	    my $older = $page;

            if ($type eq 'summary') {
                print "</ul>\n";
                $older += $brief+1;
                print "<p align=\"right\"><a href=\"$url$older\">Even older entries</a></p>";
            } elsif ($type eq 'details') {
		print "</dl>\n";
                $older += 1;
                my $newer = $page - 1;
                print '<p align="center">';
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

            } elsif ($type eq 'xml') {
                $new_html = "<dl>$new_html</dl>" if ($new_html);
                print '<newhtml><![CDATA['.$new_html.']]></newhtml></results>';
            }
        } elsif ($type eq 'details' && $search_bit ne '') {
            print "<p>Your search for " . $search_bit . " yielded no results.</p>";
        }
        if ($type ne 'xml') {
            print "<script type=\"text/javascript\"> var marker = [];\n$Js\n";
            print "adding = 1\n" if ($adding);
            print "</script>\n";
        }
    }
}

sub dt_entry {
    my ($result, $pointindex) = @_;
    my $title = encode_entities($result->{title}) || '&lt;No subject&gt;';
    my $someday = UnixDate($result->{posted}, "%E %b %Y");
    my $responses = ($result->{commentcount} != 1) ? 'responses' : 'response';
    my $name = encode_entities($result->{name});
    my $out = <<EOfragment;
    <dt><a href="$url_prefix/comments?$result->{postid}">$title</a></dt>
<dd><p>$result->{shortwhy}</p>
<small>
added $someday by $name
| <a href="$url_prefix/comments?$result->{postid}">read more</a> 
| <a href="$url_prefix/comments?$result->{postid}\#comments">$result->{commentcount} $responses</a> 
| <a href="/abuse/?postid=$result->{postid}">abusive?</a>
</small>
</dd>

EOfragment
    return $out;
}

sub handle_links {

	return '' if (defined $ENV{NO_COMMENTING});

	my $item= shift;
	my $google_terms= encode_entities($item->{title});
	$google_terms=~ s# #\+#;

	my $html.=<<EOhtml;
	<a href="$url_prefix/na/comments?$item->{postid}">Comment ($item->{commentcount}),
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
			    'posts.title',
			    'google_lat',
			    'google_long'
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





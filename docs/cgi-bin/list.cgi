#!/usr/bin/perl

use warnings;
use strict;
use FindBin;
use lib "$FindBin::Bin/../../perllib";
use lib "$FindBin::Bin/../../../perllib";
use mySociety::Config;
BEGIN {
    mySociety::Config::set_file("$FindBin::Bin/../../conf/general");
}
use PoP;
#use CGI qw/param/;
use CGI;
use CGI::Fast;
use CGI::Carp qw/fatalsToBrowser/;
use Date::Manip;
use HTML::Entities;
use Text::Wrap;
use URI::Escape;

my $site_name= mySociety::Config::get('SITE_NAME');
my %State; # State variables during display.
our $url_prefix=mySociety::Config::get('URL');
my $Js='';


while (my $q = new CGI::Fast()) {
    my $type = $q->param('type') || 'details';
        if ($type eq 'xml') {
            print "Content-Type: application/xml\n\n";
        } else {
            print "Content-Type: text/html\n\n";
        }

    {
        my $id = $q->param('show') || 0;
        $id = 0 unless ($id =~ /^\d+$/);
        my $search_bit = $q->param('search') || '';
        my $page = $q->param('page') || 0;
	my $topleft_lat=$q->param('topleft_lat');
	my $topleft_long=$q->param('topleft_long');
	my $bottomright_lat=$q->param('bottomright_lat');
	my $bottomright_long=$q->param('bottomright_long');

        if ($search_bit =~ /\|(\d+)$/) {
            $page = $1;
            $search_bit =~ s/\|(\d+)$//;
        }
        my $search_term = handle_search_term($search_bit); #' 1 = 1 ';
        my $where = "validated=1 and hidden=0 and site='$site_name' $search_term";
        if ( defined($topleft_lat) and defined($topleft_long) and
             defined($bottomright_lat) and defined($bottomright_long)) {
             $topleft_lat=~ s#[^-\.\d]##g;
             $topleft_long=~ s#[^-\.\d]##g;
             $bottomright_lat=~ s#[^-\.\d]##g;
             $bottomright_long=~ s#[^-\.\d]##g;
             $where .= " and lat <= $topleft_lat and lat >= $bottomright_lat";
             if ($topleft_long<-180 && $bottomright_long>180) {
             } elsif ($topleft_long<-180) {
                 $topleft_long+=360;
                 $where .= " and (lon <= $bottomright_long or lon >= $topleft_long)";
             } elsif ($bottomright_long>180) {
                 $bottomright_long-=360;
                 $where .= " and (lon <= $bottomright_long or lon >= $topleft_long)";
             } else {
                 $where .= " and lon <= $bottomright_long and lon >= $topleft_long";
             }
        }
        if (defined $q->param('interest')){
	    $where .= " and (interesting=1 or commentcount >= 5)";
        }
        if ($id) {
            $where = 'postid = ' . $dbh->quote($id);
        }
        my $mainlimit = 15;
	my $brief = 1; # mainlimit x brief entries displayed in the brief listing
        my $limit = ($type eq 'details') ? $mainlimit : $mainlimit * $brief;
        my $offset = $page * $mainlimit;
        $offset += $mainlimit if ($type eq 'summary');

        my $numposts = $dbh->selectrow_arrayref("SELECT COUNT(*) FROM posts WHERE $where");
        $numposts = $numposts->[0];
        my $notshown = $numposts - $limit > 0 ? $numposts - $limit : 0;
        $Js .= "\ndocument.getElementById('notshown').innerHTML = ', $notshown " . ($notshown==1?'entry':'entries') . " not shown';\n";
        my $query=$dbh->prepare("
                          select *, date_format(posted, \"%H:%i, %e %M\") as posted_formatted
                            from posts
                           where $where
                        order by posted
                                 desc limit $offset, $limit
                           "); # XXX order by first_seen needs to change


        $query->execute;
        my $result;
        my $more_link;
			
        my $printed = 0;
        $search_bit =~ s/\// /g;
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

            my $comments_html= &handle_links($result);

            #if ($result->{content} eq $result->{shortcontent}) {
            #	$more_link= $result->{link};
            #} else  {
            #	$more_link= "comments?$result->{entryid}";
            #}

#            $more_link= $result->{link};
            my $wikiuri = $result->{title};
            $wikiuri =~ tr/ /_/;
            $wikiuri = uri_escape($wikiuri);
            $Text::Wrap::columns = 32;
            my $bubble = $result->{title} || '<No subject>';
            $bubble =~ s/\s+/ /g;
            $bubble = wrap('', '', $bubble);
            $bubble = encode_entities($bubble, '<&>');
            $bubble =~ s/\n/<br>/g;
            $bubble = "<b>$bubble</b><p><a href=\"http://en.wikipedia.org/wiki/$wikiuri\">Wikipedia article</a></p>";
            my $zoomlevel = $result->{google_zoom};
            $zoomlevel = 2 unless defined($zoomlevel);
            if ($type eq 'summary') {
                my $title = encode_entities($result->{title}, '<&>') || '&lt;No subject&gt;';
                print <<EOfragment;
<li><a href="#needsJS" onclick="show_post(marker[$pointindex]); return false;">$title</a></li>
EOfragment
            } elsif ($type eq 'details') {
                print dt_entry($result, $wikiuri, $pointindex);
                $bubble =~ s/"/\\"/g;
                $Js.=<<EOjs;
marker[$pointindex] = createPin($result->{postid}, new GPoint($result->{google_long}, $result->{google_lat}), $zoomlevel, "$bubble")
EOjs
            } elsif ($type eq 'xml') {
                print '<result id="'.$result->{postid}.'" lat="'.$result->{google_lat}.'" lng="'.$result->{google_long}.'" zoom="'.$zoomlevel.'"><![CDATA['.$bubble.']]></result>';
                $new_html .= dt_entry($result, $wikiuri, $pointindex)
            }
            $pointindex++;
        }
        if ($query->rows > 0) {
            my $url = '/older/';
			
	    if ($search_bit ne ''){
			$url = '/oldersearch/'.$search_bit.'|';
	    }elsif (defined $q->param('interest')){
			$url = '/olderbusiest/';
	    }
			
#	    my $older = $page;

            if ($type eq 'summary') {
                print "</ul>\n";
#                $older += $brief+1;
#                print "<p align=\"right\"><a href=\"$url$older\">Even older entries</a></p>";
            } elsif ($type eq 'details') {
                print "</dl>\n";
#                $older += 1;
#                my $newer = $page - 1;
#                print '<p align="center">';
#                if ($newer == 0) {
#                    my $fronturl = ($search_bit ne '') ? '/oldersearch/'.$search_bit : '/';
#                    print "<a href=\"$fronturl\">front page</a> | ";
#                } elsif ($newer > 0) {
#                    print "<a href=\"$url$newer\">newer $mainlimit entries</a> | ";
#                }
#                if ($type eq 'summary' || $query->rows eq $limit) {
#                    print "<a href=\"$url$older\">older $mainlimit entries</a>";
#                }
#                print '</p>';

            } elsif ($type eq 'xml') {
                print '<notshown>' . $notshown . '</notshown>';
                print '<newhtml><![CDATA[<dl>'.$new_html.'</dl>]]></newhtml>';
                print '</results>';
            }
        } elsif ($type eq 'details' && $search_bit ne '') {
            print "<p>Your search for " . $search_bit . " yielded no results.</p>";
        }
        if ($type ne 'xml') {
            print "<script type=\"text/javascript\">//<![CDATA[\n var marker = [];\n$Js\n//]]> </script>";
        }
    }
}

sub dt_entry {
    my ($result, $wikiuri, $pointindex) = @_;
    my $title = encode_entities($result->{title}, '<&>') || '&lt;No subject&gt;';
    my $someday = UnixDate($result->{posted}, "%H:%M, %E %B %Y");
    my $responses = ($result->{commentcount} != 1) ? 'responses' : 'response';
    my $name = encode_entities($result->{name}, '<&>');
    my $shortwhy = $result->{shortwhy} || '';
    my $out = <<EOfragment;
<dt><strong><a href="#needsJS" onclick="show_post(marker[$pointindex]); return false;">$title</a></strong>
<small>(<a href="http://en.wikipedia.org/wiki/$wikiuri">Wikipedia Article</a>)</small></dt>
<dd><p>$shortwhy</p>
<small>
added $someday by $name
| <a href="/?$result->{postid}">Permalink</a>
| <a href="#needsJS" onclick="report_post_form(marker[$pointindex]); return false;">Incorrect?</a>
<!-- | <a href="../email/$result->{postid}">Email this to a friend</a>
| <a href="/abuse/?postid=$result->{postid}">abusive?</a> -->
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
	<a href="$url_prefix/?$item->{postid}">Comment ($item->{commentcount}),
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





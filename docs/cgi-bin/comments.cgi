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
use CGI qw/param/;
use HTML::Entities;
use Date::Manip;
use Text::Wrap;
use URI::Escape;

my $commentcount;
my $Entry;

my $url_prefix= mySociety::Config::get('URL');
my $site_name= mySociety::Config::get('SITE_NAME');
my %State; # State variables during display.

{
        $Entry=$ENV{QUERY_STRING} || '';
        if ($Entry !~ /^\d+$/) {
                &die_cleanly ("$Entry Error - no entry id passed in\r\n\r\n");
        }

        my $query=$dbh->prepare("
                      select  *,
                             date_format(posted, \"%H:%i, %e %M\") as posted_formatted
                        from posts
                       where postid=$Entry
		       	     and site='$site_name'
                             and not hidden");

	$query->execute;
	my $result;
	my $google_terms;
	my $someday;

	if ($query->rows == 0) {
		print "Location: $url_prefix\r\n\r\n";
	}

        print "Content-Type: text/html; charset=iso-8859-1\r\n\r\n";

	$result=$query->fetchrow_hashref;
	my $why = $result->{why};
	$why =~ s/(\r\n){2,}/<\/p> <p>/g;
	$why =~ s/\r\n/<br \/>/g;
	$someday = UnixDate($result->{posted}, "%E %b %Y");
        $Text::Wrap::columns = 32;
        my $title = $result->{title} || '<No subject>';
        $title =~ s/\s+/ /g;
        $title = wrap('', '', $title);
        $title = encode_entities($title);
        $title =~ s/\n/<br>/g;
        my $zoomlevel = $result->{google_zoom} || 2;
        my $wikiuri = $result->{title};
        $wikiuri =~ tr/ /_/;
        $wikiuri = uri_escape($wikiuri);
        my $bubble = "<b>$title</b><p><a href=\\\"http://en.wikipedia.org/wiki/$wikiuri\\\">Wikipedia article</a></p>";

        $title = encode_entities($result->{title}) || '&lt;No subject&gt;';

	print <<EOfragment;
<!--
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:trackback="http://madskills.com/public/xml/rss/module/trackback/"
         xmlns:dc="http://purl.org/dc/elements/1.1/">
<rdf:Description
    rdf:about="$url_prefix/comments?$result->{postid}"
    trackback:ping="$url_prefix/cgi-bin/trackback.cgi/$result->{postid}"
    dc:title="$title"
    dc:identifier="$url_prefix/comments?$result->{postid}"
    dc:description="$result->{shortwhy}"
    dc:creator="$site_name"
	/>
</rdf:RDF>
-->
	
	<dl>
	<dt><a href="http://en.wikipedia.org/wiki/$wikiuri">$title</a></dt>
	<dd><p>$why</p>
        <small>
        Lat: <span id="google_lat">$result->{google_lat}</span> | Long: <span id="google_long">$result->{google_long}</span>
        <br /> added on $someday by $result->{name}<!-- | <a href="../email/$result->{postid}">Email this to a friend</a> | <a href="/abuse/?postid=$result->{postid}">abusive?</a> --><br />
	</small>
	</dd>
	</dl>
<script type="text/javascript">
    marker = createPin(new GPoint($result->{google_long}, $result->{google_lat}), $zoomlevel, "$bubble")
</script>
EOfragment
	if ($result->{commentcount} > 0) {print &show_comments();}
}

sub show_comments {

	my $html="<h2><a name=\"comments\"></a>Responses</h2>\n<dl>";

	my $query=$dbh->prepare(
	  " select * from comments where postid=$Entry and visible=1");

	my $result;

	$query->execute;
	while ($result=$query->fetchrow_hashref) {
		my $someday = UnixDate($result->{posted}, "%E %b %Y");
		my $comment = $result->{comment};
		$comment =~s/(\r\n){2,}/<\/p> <p>/g;
		$comment =~s/\r\n/<br \/>/g;

		$html.= <<EOhtml;
	<dd><a name="comment_$result->{commentid}" ></a>
	<p><em><strong>$result->{name}</strong> replies:</em> $comment</p>
	<small>
	written $someday | <a href="/abuse/?postid=$result->{postid}&amp;commentid=$result->{commentid}">abusive?</a>
	</small>
	</dd>
EOhtml
	}

	$html .="</dl>";
	return $html;
}

sub die_cleanly {
        my $error= shift || 'no error given';
        print "Location: $url_prefix/error/?$error\n\n";
	exit(0);
}

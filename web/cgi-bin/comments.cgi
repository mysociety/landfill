#!/usr/bin/perl

use warnings;
use strict;
use HTML::Entities;
use Date::Manip;
use mysociety::NotApathetic::Config;
use mysociety::NotApathetic::Routines;
my $site_name= $mysociety::NotApathetic::Config::site_name;

my %State; # State variables during display.
my $Entry;


{
	&setup;
	my $result;

	$Entry=$ENV{QUERY_STRING} || '';

        if ($Entry !~ /^\d+$/) {
                &die_cleanly("$Entry Error - no entry id passed in\r\n\r\n");
        }

        my $query= $dbh->prepare("
                      select postid,
                             why,
                             title,
                             posted,
                             commentcount,
			     shortwhy,
                             date_format(posted, \"%D %M %Y\") as posted_formatted
                        from posts
                       where postid=$Entry
		             and site='$site_name'
                             and not hidden");

	$query->execute();


	unless ($result= $query->fetchrow_hashref()){
		print "Location: $url_prefix\r\n\r\n";
		exit(0);
	}

	my $why = $result->{why};

	$why=~s/(\r\n){2,}/<\/p> <p>/g;
	$why=~s/\r\n/<br \/>/g;

	# $why =~ s/(\r?\n){2,}/</p> <p>/g;
        # $why =~ s/\r?\n/<br />/g;

	print <<EOfragment;
<!--
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:trackback="http://madskills.com/public/xml/rss/module/trackback/"
         xmlns:dc="http://purl.org/dc/elements/1.1/">
<rdf:Description
    rdf:about="$url_prefix/comments/$result->{postid}"
    trackback:ping="$url_prefix/cgi-bin/trackback.cgi/$result->{postid}"
    dc:title="$result->{title}"
    dc:identifier="$url_prefix/comments/$result->{postid}"
    dc:description="$result->{shortwhy}"
	/>
</rdf:RDF>
-->
	
	<dl>
	<dt>$result->{title}</dt>
	<dd><p>$why</p>
	<small>
	written $result->{posted_formatted} | <a href="../email/$result->{postid}">Email this to a friend</a> | <a href="/abuse/?postid=$result->{postid}">abusive?</a>
	</small>
	</dd>
	</dl>

EOfragment
	if ($result->{commentcount} > 0) {print &show_comments();}
}

sub show_comments {

	my $html="<h2><a name=\"comments\"></a>Responses</h2>\n<dl>";

	my $query=$dbh->prepare( " select *,
                        date_format(posted, \"%D %M %Y\") as posted_formatted
			from comments where postid=$Entry and visible=1");

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
	written $result->{posted_formatted} | <a href="/abuse/?postid=$result->{postid}&amp;commentid=$result->{commentid}">abusive?</a>
	</small>
	</dd>
EOhtml
	}
	
	$html .="</dl>";
	return $html;
}


#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use CGI qw/param/;
use HTML::Entities;
use Date::Manip;
use mysociety::NotApathetic::Config;

my $commentcount;
my $Entry;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $url_prefix= $mysociety::NotApathetic::Config::url;
my $site_name= $mysociety::NotApathetic::Config::site_name;              # database username
my $dbh;
my %State; # State variables during display.


{
        if (!defined($dbh) || !eval { $dbh->ping() }) {
            $dbh = DBI->connect($dsn, $db_username, $db_password, {RaiseError => 1});
        }

        $Entry=$ENV{QUERY_STRING} || '';

        if ($Entry !~ /^\d+$/) {
                &die_cleanly ("$Entry Error - no entry id passed in\r\n\r\n");
        }

        my $query=$dbh->prepare("
                      select  *,
                             date_format(posted, \"%H:%i, %e %M\") as posted_formatted
                        from posts
                       where postid=$Entry
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
	$someday = UnixDate($result->{posted}, "%E %b %Y");

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
    dc:creator="$site_name"
	/>
</rdf:RDF>
-->
	
	<dl>
	<dt>$result->{title}</dt>
	<dd><p>$why</p>
	<small>
        Lat: <span id="google_lat">$result->{google_lat}</span> | Long: <span id="google_long">$result->{google_long}</span> <br />
	written on $someday by $result->{name} | <a href="../email/$result->{postid}">Email this to a friend</a> | <a href="/abuse/?postid=$result->{postid}">abusive?</a>
	</small>
	</dd>
	</dl>
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
      &mysociety::NotApathetic::Config::die_cleanly(@_);

}

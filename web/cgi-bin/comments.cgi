#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;

my $Entry=$ENV{QUERY_STRING} || '';

if ($Entry !~ /^\d+$/) {
	print "Content-Type: text/html/\n\n";
	print "$Entry Error - no entry id passed in\n\n";
	exit (0);
}

use mysociety::NotApathetic::Config;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $url_prefix= $mysociety::NotApathetic::Config::url;
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});
my %State; # State variables during display.

print "Content-Type: text/html\n\n";
{
	my $query=$dbh->prepare("
	              select postid,
		             email,
			     age,
			     sex,
			     region,
			     evervoted,
			     why,
			     nochildren,
			     title,
			     posted,
			     commentcount,
			     ethgroup,
			     date_format(posted, \"%H:%i, %e %M\") as posted_formatted
			from posts
		       where postid=$Entry
		    order by posted desc
		       "); # XXX order by first_seen needs to change


	$query->execute;
	my $result;
	my $google_terms;

	if ($query->rows == 0) {
		print "Location: $url_prefix\n";
	}
	$result=$query->fetchrow_hashref;
	my $why = $result->{why};

	$why=~ s#\n#</p>\n\n<p>\n#g;

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
    dc:description="$result->{shortcontent}"
    dc:creator="NotApathetic.com"
	/>
</rdf:RDF>
-->

	<div class="entry">
		<strong>$result->{title}</strong>
		<p>$why</p>
		<span class="rightalign">
		Posted at $result->{posted}
		<br />
		<a href="../email/$result->{postid}">Email this to a friend</a>.
		</span>
	</div>
		<br />

EOfragment
	if ($result->{commentcount} > 0) {print &show_comments();}
	print &comment_form();
}

sub show_comments {
	my $html=" <h2>Comments</h2>";

	my $query=$dbh->prepare(
	  " select * from comments where postid=$Entry");

	my $result;

	$query->execute;

	while ($result=$query->fetchrow_hashref) {

		$html.= <<EOhtml;
	<hr width="80%" />
	<a name="comment_$result->{commentid}" />
	<p>
	$result->{comment}
	</p>
	<small>
		Posted by $result->{name} on $result->{posted}.
	<a
	href="$url_prefix/comments/?$Entry#comment_$result->{commentid}">Link</a>.
		Report abuse to abuse\@notapathetic.com
	</small>
EOhtml
	}
	

	return $html;
}


sub comment_form {

	my $html= <<EOhtml;

<h2>Comment</h2>

<form method="post" action="../cgi-bin/comment.cgi">
	<input type="hidden" name="postid" value="$Entry" />
	<table class="commentsformtable">
	<tr>

    		<th><label for="author">Your Name</label></th>
    		<td><input id="author" name="author" /></td>
	</tr>
	<tr>
    		<th><label for="email">Email</label></th>
    		<td><input id="email" name="email" /><br /></td>
	</tr>
	<tr>
    		<td colspan="2">
    		<span class="smallprint">(You must give a valid email address, but it will <em>not</em> be displayed to the public.)</span>

    		</td>
	</tr>
</table>

<p class="commentsformlabel"><label for="text">Comments:</label></p>

<textarea id="text" name="text" rows="15" cols="50"></textarea>

<p>
<span class="smallprint">We only allow the following html tags <tt>a cite em strong p br</tt>. After posting, there may be a short delay before your comment appears on the site</span><br />
<input style="font-weight: bold;" type="submit" name="post" value="Post" />
</p>


EOhtml
	return ($html);
}



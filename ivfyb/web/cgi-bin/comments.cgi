#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use CGI::Fast;
use HTML::Entities;
use Date::Manip;
use mysociety::IVotedForYouBecause::Config;

my $commentcount;
my $Entry;

my $dsn = $mysociety::IVotedForYouBecause::Config::dsn; # DSN connection string
my $db_username= $mysociety::IVotedForYouBecause::Config::db_username;              # database username
my $db_password= $mysociety::IVotedForYouBecause::Config::db_password;         # database password
my $url_prefix= $mysociety::IVotedForYouBecause::Config::url;
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});
my %State; # State variables during display.


while (my $q = new CGI::Fast()) {
        $Entry=$ENV{QUERY_STRING} || '';

        if ($Entry !~ /^\d+$/) {
                print "Content-Type: text/html; charset=iso-8859-1\r\n\r\n";
                print "$Entry Error - no entry id passed in\r\n\r\n";
                next;
        }

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
                             shortwhy,
                             date_format(posted, \"%H:%i, %e %M\") as posted_formatted
                        from posts
                       where postid=$Entry
                    order by posted desc
                       "); # XXX order by first_seen needs to change

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
    dc:creator="NotApathetic.com"
	/>
</rdf:RDF>
-->
	
	<div class="entry">
		<h4>$result->{title}</h4>
		<p>$why</p>
		<div>
		<small>
		written $someday | <a href="../email/$result->{postid}">Email this to a friend</a> | <a href="/abuse/?postid=$result->{postid}">abusive?</a>
		</small>
		</div>
	</div>

EOfragment
	if ($result->{commentcount} > 0) {print &show_comments();}
	$commentcount = $result->{commentcount};
	print &comment_form();
}

sub show_comments {
	my $html="<h2><a name=\"comments\"></a>Responses</h2>";

	my $query=$dbh->prepare(
	  " select * from comments where postid=$Entry and visible=1");

	my $result;

	$query->execute;
	while ($result=$query->fetchrow_hashref) {
		my $someday = UnixDate($result->{posted}, "%E %b %Y");
		my $comment = $result->{comment};
		$comment =~s/(\r\n){2,}/<\/p> <p>/g;
		$comment =~s/\r\n/<br \/>/g;
		if(!(substr($comment, 0, 6) eq "  <div")){$comment="<p>".$comment."</p>";} ## remove later ##

		$html.= <<EOhtml;
	<div class="entry">
		<a name="comment_$result->{commentid}" ></a>
		$comment
		<div>
		<small>
		written by $result->{name} on $someday | <a href="/abuse/?postid=$result->{postid}&amp;commentid=$result->{commentid}">abusive?</a>
		</small>
		</div>
	</div>
EOhtml
	}
	

	return $html;
}


sub comment_form {
	my $anchor = "";
	if($commentcount==0){
		$anchor = "<a name=\"comments\"></a>";
	}

	my $html= <<EOhtml;

<h2>$anchor Respond</h2>

<form method="post" action="../cgi-bin/comment.cgi" id="comment">
	<input type="hidden" name="postid" value="$Entry" />
	
	<div>
	<label for="author">Your Name</label><input id="author" name="author" type="text" size="20" />
	</div>
	
	<div>
	<label for="email">Email</label><input id="email" name="email" type="text" size="20" />
	<small>You must give a valid email address, but it will
    	<em>not</em> be displayed to the public.</small>
	</div>

	<div>
    	
	<textarea id="commenttext" name="text" rows="15" cols="30">Write your response...</textarea>
	<small>We only allow the following html tags:
		<tt>a cite em strong p br</tt>. Newlines automatically
                become new paragraphs or line breaks. After posting,
		there may be a short delay before your comment
	appears on the site</small>
	</div>
	<div>

	<input type="submit" name="post" value="Post" id="commentsubmit" />
	</div>

	
</form>


EOhtml
	return ($html);
}



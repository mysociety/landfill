#!/usr/bin/perl

BEGIN {push @INC, '/home/sebastian/sams/work/democracy.org.uk/panopticon/bin/'}

use warnings;
use strict;
use PanopticonConfig;
use CGI qw/param/;
{
	print "Content-Type: text/html\n\n";
	if (param() ){
		&process_request(); # can output messages.
	}
	&output_status();
}


sub process_request {
	my %Passed_Values;
	foreach my $p (param()){
		$Passed_Values{$p}=param($p);
	}
	$Passed_Values{'tag'} ||='';
	$Passed_Values{'url'} ||='';
	$Passed_Values{'feedid'} ||='';

	if ($Passed_Values{'job'} eq 'add') {
		if (($Passed_Values{'tag'} =~ /^$/) or ($Passed_Values{'url'} =~ /^$/) ) {
			print "<h3>Addition failed: fields missing<h3>\n";
		} else {
			my $q= $dbh->prepare("insert into feeds set feedurl=?, tag=?");
			$q->execute($Passed_Values{'url'}, $Passed_Values{'tag'}) || die $dbh->errstr;
			print "<h3>Feed added</h3>\n";
			print "<p>Feeds don't appear below until they have been successfully fetched once</p>";
		}
	} elsif ($Passed_Values{'job'} eq 'cancel') {
		if ($Passed_Values{'feedid'} =~ /^$/) {
			print "<h3>Cancel failed: missing feedid</h3>\n";
		} else {
			my $q= $dbh->prepare("update feeds set active=0 where feedid=?");
			$q->execute($Passed_Values{'feedid'}) || die $dbh->errstr;
			print "<h3>Feed cancelled</h3>\n";
		}
	}
}

sub output_status {
	print <<EOhtml;
<h3>Add</h3>
<form method="post" action="./admin.cgi">
	URL: <input type="text" name="url" /><br />
	Tag: <input type="text" name="tag" />
	<input type="hidden" name="job" value="add" />
	<input type="submit" value="add new rss feed" />
</form>
EOhtml

	my $query= $dbh->prepare("select feedinfo.title, feedinfo.description,feedinfo.siteurl,
				 	 feeds.feedid, feeds.tag, feeds.last_successful_fetch, 
					 feeds.last_failed_fetch, feeds.reason, feeds.feedurl
			            from feeds, feedinfo 
				   where feeds.feedid= feedinfo.feedid and feeds.active=1
				");
	$query->execute() || die $dbh->errstr;


	print <<EOhtml;
<h3>Status</h3>
	<table>
	<tr>
		<td>
			Tag
		</td>
		<td>
			Site
		</td>
		<td>
			Description
		</td>
		<td>
			Last Successful fetch
		</td>
		<td>
			Last Failed fetch
		</td>
		<td>
			Reason
		</td>
	</tr>
EOhtml

	while (my $r =$query->fetchrow_hashref) {
		print <<EOhtml;

	<tr>
		<td>
			<a href="$r->feedurl">$r->{tag}</a>
		</td>
		<td>
			<a href="$r->{siteurl}">$r->{title}</a>
		</td>
		<td>
			$r->{description}
		</td>
		<td>
			$r->{last_successful_fetch}
		</td>
		<td>
			$r->{last_failed_fetch}
		</td>
		<td>
			$r->{reason}
		</td>
		<td>
			<a href="./admin.cgi?feedid=$r->{feedid};job=cancel">cancel</a>
		</td>
	</tr>
EOhtml
	}


}



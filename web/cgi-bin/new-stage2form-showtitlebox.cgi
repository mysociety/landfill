#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;
use mysociety::NotApathetic::Config;
use mysociety::NotApathetic::Routines;
use CGI qw/param/;
my $site_name= $mysociety::NotApathetic::Config::site_name;

{
	#print "Content-Type: text/html; charset=iso-8859-1\r\n\r\n";
	&setup;
	my $param_c= param("c") || exit(0);
	my $param_r= param("u") || exit(0);

	$param_r =~ s#'##g;

	my $authcode_q= $dbh->quote($param_c);
	my $postid_q= $dbh->quote($param_r);
	my $query=$dbh->prepare("
	              select title, authcode from posts
	 	       where validated=1
			 and hidden=0
			 and authcode=$authcode_q
			 and postid=$postid_q
			 and site='$site_name'
		       "); 

	$query->execute;
	my ($title, $authcode);
	if (($title, $authcode) = $query->fetchrow_array) {
		$title = encode_entities($title);
        	$param_r = encode_entities($param_r);
        	$authcode = encode_entities($authcode);
		print <<EOfragment;

	<tr>
		<td>
			Subject
		</td>
		<td>
			<input type="hidden" name="postid" value="$param_r" />
			<input type="hidden" name="authcode" value="$authcode" />
			<input type="text" name="title" value="$title" />
		</td>
	</tr>
EOfragment
	}

}



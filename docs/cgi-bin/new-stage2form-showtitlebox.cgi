#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;
use mysociety::NotApathetic::Config;
use CGI qw/param/;
my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $site_name= $mysociety::NotApathetic::Config::site_name;
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 1});

{
	print "Content-Type: text/html; charset=iso-8859-1\r\n\r\n";
	my $param_c= param("c") || next;
	my $param_r= param("u") || next;

	$param_r =~ s#'##g;

	my $authcode_q= $dbh->quote($param_c);
	my $postid_q= $dbh->quote($param_r);
	my $query=$dbh->prepare("
	              select title, authcode,shortwhy from posts
	 	       where validated=0
			 and hidden=0
			 and authcode=$authcode_q
			 and postid=$postid_q
			 and site='$site_name'
		       "); 

	$query->execute;
	my ($title, $authcode, $shortwhy)= $query->fetchrow_array;
	$title = encode_entities($title);
        $param_r = encode_entities($param_r);
        $authcode = encode_entities($authcode);
        $shortwhy = encode_entities($shortwhy);
	print <<EOfragment;

	<input type="hidden" name="postid" value="$param_r" />
	<input type="hidden" name="authcode" value="$authcode" />
	<tr>
		<td>
			Title (1-4 words)
		</td>
		<td>
			<input type="text" width="30" name="title" value="$title" />
		</td>
	</tr>
EOfragment

}



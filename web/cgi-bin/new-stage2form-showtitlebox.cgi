#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;
use mysociety::NotApathetic::Config;
use CGI::Fast qw/param encode_entities/;
my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});

while (new CGI::Fast()) {
	print "Content-Type: text/html\n\n";
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
		       "); 

	$query->execute;
	my ($title, $authcode)= $query->fetchrow_array;
	encode_entities($title);
	print <<EOfragment;

	<input type="hidden" name="postid" value="$param_r" />
	<input type="hidden" name="authcode" value="$authcode" />
	<tr>
		<td>
			Title
		</td>
		<td>
			<input type="text" name="title" value="$title" />
		</td>
	</tr>
EOfragment

}



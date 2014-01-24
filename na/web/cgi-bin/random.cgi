#!/usr/bin/perl

use warnings;
use strict;
use CGI;
use DBI;
use mysociety::NotApathetic::Config;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
our $url_prefix=$mysociety::NotApathetic::Config::url;

{
            my $dbh = DBI->connect($dsn, $db_username, $db_password, {RaiseError => 1});

            my $query=$dbh->prepare("
                          select postid from posts
                           where validated=1
                             and hidden=0
			     and site='$site_name'
                           order by rand() desc limit 1
                           ");
            $query->execute();
            my ($postid)= $query->fetchrow_array;
            print "Location: $url_prefix/comments/$postid\n\n";
}

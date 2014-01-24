#!/usr/bin/perl

use warnings;
use strict;
use CGI::Fast;
use DBI;
use mysociety::IVotedForYouBecause::Config;


my $dsn = $mysociety::IVotedForYouBecause::Config::dsn; # DSN connection string
my $db_username= $mysociety::IVotedForYouBecause::Config::db_username;              # database username
my $db_password= $mysociety::IVotedForYouBecause::Config::db_password;         # database password
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});
our $url_prefix=$mysociety::IVotedForYouBecause::Config::url;

while (my $q = new CGI::Fast()) {
    eval {

            my $query=$dbh->prepare("
                          select postid
                            from posts
                           where validated=1
                             and hidden=0
                           order by rand() desc limit 1
                           ");
            $query->execute();
            my ($postid)= $query->fetchrow_array;
            print "Location: $url_prefix/comments/$postid\n\n";
    };
    if ($@) {
        print "Oh dear.\n$@";
    }
}

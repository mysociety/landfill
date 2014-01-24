#!/usr/bin/perl

use warnings;
use strict;
use CGI qw/param/;
use Date::Manip;
use DBI;
use HTML::Entities;
use mysociety::NotApathetic::Config;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username; # database username
my $db_password= $mysociety::NotApathetic::Config::db_password; # database password
my $site_name= $mysociety::NotApathetic::Config::site_name;
my $dbh=DBI->connect($dsn, $db_username, $db_password);
my %State; # State variables during display.
our $url_prefix=$mysociety::NotApathetic::Config::url;


{
     if (defined $ENV{REQUEST_METHOD}) {
         print "Content-Type: text/html; charset=iso-8859-1\r\n\r\n";
     }

        my $query=$dbh->prepare("select * from posts where validated=1 and hidden=0 and site='$site_name' "); 

        $query->execute;
        my $result;
        while ($result=$query->fetchrow_hashref) {
                print <<EOfragment;
<dt><a href="$url_prefix/comments?$result->{postid}">$result->{title}</a></dt><dd><p>$result->{why}</p></dd>
EOfragment
            }
}



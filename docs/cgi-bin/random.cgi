#!/usr/bin/perl

use warnings;
use strict;
use FindBin;
use lib "$FindBin::Bin/../../perllib";
use lib "$FindBin::Bin/../../../perllib";
use mySociety::Config;
BEGIN {
    mySociety::Config::set_file("$FindBin::Bin/../../conf/general");
}
use PoP;

my $site_name= mySociety::Config::get('SITE_NAME');
our $url_prefix=mySociety::Config::get('URL');

{
            my $query=$dbh->prepare("
                          select postid
                            from posts
                           where validated=1
                             and hidden=0
                             and site='$site_name'
                           order by rand() desc limit 1
                           ");
            $query->execute();
            my ($postid)= $query->fetchrow_array;
            print "Location: $url_prefix/?$postid\n\n";
}

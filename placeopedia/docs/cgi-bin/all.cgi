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
use CGI qw/param/;
use HTML::Entities;

my $site_name= mySociety::Config::get('SITE_NAME');
our $url_prefix=mySociety::Config::get('URL');

{
    if (defined $ENV{REQUEST_METHOD}) {
        print "Content-Type: text/html; charset=iso-8859-1\r\n\r\n";
    }

    my $query=$dbh->prepare("select * from posts where validated=1 and hidden=0 and site='$site_name' "); 

    $query->execute;
    my $result;
    while ($result=$query->fetchrow_hashref) {
        my $why = encode_entities($result->{why}) || '';
        my $title = encode_entities($result->{title}) || '&lt;No subject&gt;'; 
        print <<EOfragment;
<dt><a href="$url_prefix/?$result->{postid}">$title</a></dt><dd><p>$why</p></dd>
EOfragment
    }
}


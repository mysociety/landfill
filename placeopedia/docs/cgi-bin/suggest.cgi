#!/usr/bin/perl -w
#
# suggest.cgi:
# Server side of the suggest-article-titles interface.
#
# $Id: suggest.cgi,v 1.19 2008-02-04 23:25:27 matthew Exp $
#

use strict;
require 5.8.0;

use FindBin;
use lib "$FindBin::Bin/../../perllib";
use lib "$FindBin::Bin/../../../perllib";
use mySociety::Config;
BEGIN {
    mySociety::Config::set_file("$FindBin::Bin/../../conf/general");
}

use mySociety::CGIFast;
use HTML::Entities;
use PoP;
$dbh->{RaiseError} = 1;
$dbh->do("set character set 'utf8'");
$dbh->do("set names 'utf8'");

binmode(STDOUT, ':bytes');

my $site_name= mySociety::Config::get('SITE_NAME');
our $url_prefix=mySociety::Config::get('URL');

while (my $q = new mySociety::CGIFast()) {
    my $res = '';
    my $entry = $q->param('qu') || 'West';
    $entry =~ s#^http://en\.wikipedia\.org/wiki/##i;
    utf8::decode($entry);
    if ($entry) {
        my $n_entry;
        foreach (split /[\s_]/, $entry) {
            $n_entry .= $_ . ' ';
        }
        $entry = $n_entry;
        $entry =~ s/\s+$//;
        $entry =~ s/^\s+//;
        $entry = ucfirst($entry);
        $entry =~ tr/ /_/;
        
        my $q_entry = $entry;
        $q_entry =~ s/_/\\_/g;
        $q_entry =~ tr/;/?/;    # ?
        my $query=$dbh->prepare("
                select distinct title
                from wikipedia_article
                where title like ?
                limit 10
                ");
        $query->execute("$q_entry%");
        
        my @titles;
        
        while ((my $title) = $query->fetchrow_array())  {
            utf8::decode($title);
            $title =~ tr/_/ /;
            $title =~ s/(["\\])/\\$1/g;
            push(@titles, '"' . encode_entities($title, '<&>') . '"');
        }
        
        $entry =~ s/(["\\])/\\$1/g;
        $res = qq#sendRPCDone(frameElement,"$entry",new Array(#
                    . join(',', @titles)
                    . '), new Array("","","","","","","","","",""), new Array(""));';
        utf8::encode($res);
    }

    print $q->header(
                -content_type => 'text/html; charset=utf-8',
                -content_length => length($res))
            , $res;
}

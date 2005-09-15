#!/usr/bin/perl -w
#
# suggest.cgi:
# Server side of the suggest-article-titles interface.
#
# $Id: suggest.cgi,v 1.11 2005-09-15 11:23:24 matthew Exp $
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

use CGI::Fast;

use PoP;
$dbh->{RaiseError} = 1;
$dbh->do("set character set 'utf8'");

binmode(STDOUT, ':utf8');

my $site_name= mySociety::Config::get('SITE_NAME');
our $url_prefix=mySociety::Config::get('URL');

while (my $q = new CGI::Fast()) {
    my $res = '';
    my $entry = $q->param('qu') || 'West';
    if ($entry) {
        my $n_entry;
        foreach (split /[\s_]/, $entry) {
            # XXX is this right?
            $n_entry .= ucfirst($_) . ' ';
        }
        $entry = $n_entry;
        $entry =~ s/\s+$//;
        $entry =~ s/^\s+//;
        $entry =~ tr/ /_/;
        
        my $q_entry = $entry;
        $q_entry =~ s/_/\\_/g;
        $q_entry =~ tr/;/?/;    # ?
        my $query=$dbh->prepare("
                select title
                from wikipedia_article
                where title like ?
                limit 10
                ");
        $query->execute("$q_entry%");
        
        my @titles;
        
        while ((my $title) = $query->fetchrow_array())  {
            $title =~ tr/_/ /;
            $title =~ s/(["\\])/\\$1/g;
            push(@titles, qq("$title"));
        }
        
        $entry =~ s/(["\\])/\\$1/g;
        $res = qq#sendRPCDone(frameElement,"$entry",new Array(#
                    . join(',', @titles)
                    . '), new Array("","","","","","","","","",""), new Array(""));';
    }

    print $q->header(
                -content_type => 'text/html; charset=utf-8',
                -content_length => length($res))
            , $res;
}

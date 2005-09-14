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
use Encode qw/encode/;

my $site_name= mySociety::Config::get('SITE_NAME');
our $url_prefix=mySociety::Config::get('URL');

{
   	print "Content-type: text/html\n\n";
	my $entry= param('qu') || 'West';
	my $n_entry;
	foreach (split /[\s_]/, $entry) {
		$n_entry.= ucfirst($_) . ' ';
	}
	$entry=$n_entry;
	$entry =~ s/\s+$//;
	$entry =~ s/^\s+//;
	$entry =~ tr/ /_/;
	my $q_entry = $entry;
	$q_entry =~ s/_/\\_/g;
	$q_entry =~ tr/;/?/;
	$q_entry = $dbh->quote($q_entry . '%');
        my $query=$dbh->prepare("
                          select title
                            from wikipedia_article
                           where title like $q_entry
                           limit 10
                           ");
        $query->execute();
	my $title;
	my @titles;
        while ($title= $query->fetchrow_hashref)  {
            $title->{cur_title} =~ tr/_/ /;
		push @titles, '"' . encode('utf-8', $title->{cur_title}) . '"';
	}
    
    print "sendRPCDone(frameElement,\"$entry\",new Array(";
    print join ',', @titles;
    print '), new Array("", "", "", "","","","","","",""), new Array(""));';

}

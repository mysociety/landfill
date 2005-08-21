#!/usr/bin/perl

# this is a candidate for Fast::CGI

use warnings;
use strict;
use DBI;
use CGI qw/param/;
use mysociety::NotApathetic::Config;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $site_name= $mysociety::NotApathetic::Config::site_name;
our $url_prefix=$mysociety::NotApathetic::Config::url;

{
   	print "Content-type: text/html\n\n";
        my $dbh = DBI->connect($dsn, $db_username, $db_password, {RaiseError => 1});
	my $entry= param('qu') || 'West';
	$entry =~ tr/ /_/;
        my $query=$dbh->prepare("
                          select cur_title
                            from cur
                           where cur_title like '$entry%'
                           limit 10
                           ");
        $query->execute();
	my $title;
	my @titles;
        while ($title= $query->fetchrow_hashref)  {
		push @titles, "\"$title->{cur_title}\"";
	}
    
    print "sendRPCDone(frameElement,\"$entry\",new Array(";
    print join ',', @titles;
    print '), new Array("", "", "", "","","","","","",""), new Array(""));';

}

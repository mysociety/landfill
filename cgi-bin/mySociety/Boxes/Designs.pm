#!/usr/bin/perl

use CGI qw/param/;
use CGI::Carp qw/warningsToBrowser fatalsToBrowser/;
use DBI;
our $dbh;
package mySociety::Boxes::Config;
require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(%Site_Feed_Names $dbh);  # symbols to export on request
my $dsn = 'DBI:mysql:mysoc_boxes:localhost'; # DSN connection string
my $db_username= '****';              # database username
my $db_password= '****';         # database password
our $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});


our %Site_Feed_Name;

# use POSTCODE as a placeholder for the postcode
# use CONSTITUENCY_ID as a placeholder for the PW constituencyid
$Site_Feed_Names{'pledgebank'}= 'http://www.en-gb.pledgebank.com/rss/search?q=POSTCODE&far=50';
$Site_Feed_Names{'hearfromyourmp'}= 'http://www.hearfromyourmp.com/'; # XXX this isn't an RSS URL

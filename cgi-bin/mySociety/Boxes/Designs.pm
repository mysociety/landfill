#!/usr/bin/perl

package mySociety::Boxes::Config;
require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(%Site_Feed_Names);  # symbols to export on request

our %Site_Feed_Name;

# use POSTCODE as a placeholder for the postcode
# use CONSTITUENCY_ID as a placeholder for the PW constituencyid
$Site_Feed_Names{'pledgebank'}= 'http://www.en-gb.pledgebank.com/rss/search?q=POSTCODE&far=50';
$Site_Feed_Names{'hearfromyourmp'}= 'http://www.hearfromyourmp.com/'; # XXX this isn't an RSS URL

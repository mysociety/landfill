#!/usr/bin/perl -w -I../../perllib -I../perllib
#
# volunteertasks.cgi:
# Simple interface for viewing volunteer tasks from the CVSTrac database.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#

my $rcsid = ''; $rcsid .= '$Id: index.cgi,v 1.3 2007-08-02 11:45:06 matthew Exp $';

use strict;
require 5.8.0;

use CGI qw(-no_xhtml);
use CGI::Fast;
use DBI;
use HTML::Entities;
use POSIX;
use WWW::Mechanize;
use Data::Dumper;
use LWP::Simple;

use mySociety::Config;
mySociety::Config::set_file('../conf/general');

use mySociety::EvEl;

use CVSWWW;

my $dbh = CVSWWW::dbh();

my %ticket_titles = map { $_->[0] => $_->[1] }
                @{$dbh->selectall_arrayref(
                    "select tn, title from ticket")
                };

sub do_view_volunteers ($) {
    my $q = shift;

    my $tn = $q->param('tn');

    print $q->header(-type => 'text/html; charset=utf-8');
    print CVSWWW::start_html_admin($q, "Signed up volunteers");
    print $q->h1("Signed up volunteers");

    my @volunteers = @{$dbh->selectall_arrayref(
                    "select ticket_num, name, email, whenregistered from volunteer_interest order by ticket_num, whenregistered desc")};
    my %bytask;
    foreach my $volunteer (@volunteers) {
        push @{$bytask{$volunteer->[0]}}, $volunteer;
    }

    print map { 
        $q->h2($q->a({ -href => "https://secure.mysociety.org/cvstrac/tktview?tn=$_" }, $ticket_titles{$_})),
        $q->table ( { -border => 1 },
            map { $q->Tr({}, $q->td([$_->[1],$_->[2], strftime('%e %B %Y', localtime($_->[3]))])) } @{$bytask{$_}}
        )
        } sort keys %bytask;

    print CVSWWW::end_html_admin($q);
}

while (my $q = new CGI::Fast()) {
#    $q->encoding('utf-8');
    $q->autoEscape(0);
    do_view_volunteers($q);
}

$dbh->disconnect();

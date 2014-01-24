#!/usr/bin/perl
#
# CVSWWW.pm:
# Stuff for cvs.mysociety.org (e.g. volunteer tasks)
#
# Copyright (c) 2006 UK Citizens Online Democracy. All rights reserved.
# Email: francis@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: CVSWWW.pm,v 1.3 2007-08-07 15:53:55 matthew Exp $
#

package CVSWWW;

use strict;

use LWP::Simple;

our $html_nav_menu = get("http://www.mysociety.org/menu.html");
our $dbh;

sub dbh () {
    if (!$dbh) {
        $dbh = DBI->connect(
                        "dbi:SQLite:dbname=/usr/local/cvs/mysociety/mysociety.db",
                        "", "", { 
                            # We don't want to enter a transaction which would lock the
                            # database.
                            AutoCommit => 1,
                            RaiseError => 1,
                            PrintError => 0,
                            PrintWarn => 0
                        });
    }
    return $dbh;
}

sub start_html ($$) {
    my ($q, $title) = @_;
    return $q->start_html(
                -encoding => 'utf-8',
                -title => ($title ? "$title - mySociety" : "mySociety"),
                -style => {
                    -src => 'http://www.mysociety.org/global.css',
                    -media => 'screen',
                    -type => 'text/css'
                }
            ),
            $q->div({ -class => 'top' },
                $q->div({ -class => 'masthead' },
                    $q->a({ -href => '/'},
                        $q->img({
                            -src => 'http://www.mysociety.org/mslogo.gif',
                            -alt => 'mySociety.org',
                            -border => 0
                        })
                    )
                )
            ),
            $q->start_div({ -class => 'page-body' }),
            $html_nav_menu,
            $q->div({ -class => 'item_head' }, ''), #, $title),
            $q->start_div({ -class => 'item' });
}

sub end_html ($) {
    my $q = shift;
    return $q->end_div(),
            $q->div({ -class => 'item_foot' }),
            $q->end_div(),
            $q->end_div(),
            $q->end_html();
}

sub start_html_admin ($$) {
    my ($q, $title) = @_;
    return $q->start_html(
                -encoding => 'utf-8',
                -title => ($title ? "mySociety admin: $title" : "mySociety admin"),
            ),
            $q->start_div({ -class => 'page-body' }),
}

sub end_html_admin ($) {
    my $q = shift;
    return 
        $q->p($q->img({
            -src => 'http://www.mysociety.org/mysociety_sm.gif',
            -alt => 'mySociety.org',
            -border => 0
        })),
        $q->end_html();
}

my %ticket_cache;  # id -> [date, heading, text]
my $M;
sub html_format_ticket ($) {
    my $ticket_num = shift;

    die "bad NUM '$ticket_num' in html_format_ticket"
        unless ($ticket_num =~ m#^(0|[1-9]\d*)$#);
    
    # Hideous. Cvstrac's formatting rules are quite complex, so better to
    # exploit the implementation that already exists.
    $M ||= new WWW::Mechanize();

    my $changetime = dbh()->selectrow_array('select changetime from ticket where tn = ?', {}, $ticket_num);

    return () if (!defined($changetime));

    if (exists($ticket_cache{$ticket_num})
        && $ticket_cache{$ticket_num}->[0] >= $changetime) {
        return ($ticket_cache{$ticket_num}->[1], $ticket_cache{$ticket_num}->[2]);
    }

    my $url = "https://secure.mysociety.org/cvstrac/tktview?tn=$ticket_num";
    my $resp = $M->get($url) || die "GET $url: failed; system error = $!";
    die "GET $url: " . $resp->status_line() if (!$resp->is_success());

    # Have the response. We want the first <h2>...</h2> and the first
    # <blockquote>...</blockquote>

    my ($heading) = ($M->content() =~ m#<h2>Ticket \Q$ticket_num\E: (.*?)</h2>#s);
    die "GET $url: can't find ticket heading" if (!$heading);
    my ($content) = ($M->content() =~ m#<blockquote>(.*?)</blockquote>#s);
    die "GET $url: can't find ticket text" if (!$content);

    $ticket_cache{$ticket_num} = [ $changetime, $heading, $content ];
    return ($heading, $content);
}


1;

#!/usr/bin/perl -w -I../../perllib
#
# volunteertasks.cgi:
# Simple interface for viewing volunteer tasks from the CVSTrac database.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#

my $rcsid = ''; $rcsid .= '$Id: volunteertasks.cgi,v 1.2 2006-01-27 03:47:24 francis Exp $';

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

use mySociety::Util;
use mySociety::EvEl;

my $dbh = DBI->connect(
                "dbi:SQLite2:dbname=/usr/local/cvs/mysociety/mysociety.db",
                "", "", { 
                    # We don't want to enter a transaction which would lock the
                    # database.
                    AutoCommit => 1,
                    RaiseError => 1,
                    PrintError => 0,
                    PrintWarn => 0
                });

my %skills = map { $_->[0] => $_->[1] }
                @{$dbh->selectall_arrayref(
                    "select name, value from enums where type = 'extra1'")
                };
my %times = map { $_->[0] => lc($_->[1]) }
                @{$dbh->selectall_arrayref(
                    "select name, value from enums where type = 'extra2'")
                };

my $html_nav_menu = get("http://www.mysociety.org/menu.html");

my %ticket_cache;  # id -> [date, heading, text]
my $M;
sub html_format_ticket ($) {
    my $ticket_num = shift;

    die "bad NUM '$ticket_num' in html_format_ticket"
        unless ($ticket_num =~ m#^(0|[1-9]\d*)$#);
    
    # Hideous. Cvstrac's formatting rules are quite complex, so better to
    # exploit the implementation that already exists.
    $M ||= new WWW::Mechanize();

    my $changetime = $dbh->selectrow_array('select changetime from ticket where tn = ?', {}, $ticket_num);

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

sub start_html ($$) {
    my ($q, $title) = @_;
    return $q->start_html(
                -encoding => 'utf-8',
                -title => ($title ? "mySociety: $title" : "mySociety"),
                -style => {
                    -src => 'http://www.mysociety.org/global.css',
                    -media => 'screen',
                    -type => 'text/css'
                }
            ),
            $q->div({ -class => 'top' },
                $q->div({ -class => 'masthead' },
                    $q->img({
                        -src => 'http://www.mysociety.org/mslogo.gif',
                        -alt => 'mySociety.org'
                    })
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

sub do_list_page ($) {
    my $q = shift;
    my $pagelen = 20;

    my $skills_needed = $q->param('skills');
    $skills_needed = undef if (!$skills_needed || !exists($skills{$skills_needed}));

    my %skills_desc = (
            nontech => 'anyone',
            programmer => 'a programmer',
            designer => 'a graphic designer'
        );

    # Generic blurb.
    my $blurb = <<EOF;
<img style="float: right;" src="mysociety-at-work.jpg" alt="" title="mySociety at work">

<h2>Getting involved in mySociety &mdash; the First Step</h2>

<p>mySociety needs volunteers with all sorts of skills to help build, maintain
and get the most out of our sites and services.  Which of these descriptions
fits your skills best?</p>

<ul>
<li><a href="?skills=nontech">I am not a techy, although I do use a
computer</a></li>
<li><a href="?skills=programmer">I am a programmer</a></li>
<li><a href="?skills=designer">I am a graphic designer</a></li>
</ul>
EOF

    my %blurb = (
        nontech => <<EOF,
<p>People sometimes think that we're only interested in programmers, but
nothing could be further from the truth. In 2006 we want our various
sites to grow, and to be more and more widely used by people in as
many places as possible: people who can tell their friends, put up
posters, hold dinners, write to newspapers, design brochures and so on
are invaluable to us. Just look at the list below and click to tell us
how you can help!</p>
EOF
        programmer => <<EOF,
<p>We're an Open Source languages shop, but beyond that we're very free and
easy about what people use. Perl, PHP, Python, Javascript and Ruby, have all
been used on mySociety's projects, and there's no reason to stop there. Our
servers run on Linux and FreeBSD, and we prefer PostgreSQL to MySQL, but none
of that is set in stone.</p>

<p>If you just want to get straight   
into hacking our code, you can get everything from CVS
<a href="http://www.mysociety.org/moin.cgi/CvsRepository">as described
here</a>, but don't forget to put your name to one of the tasks below so that
we know who's working on what.</p>
EOF
        designer => <<EOF,
<p>mySociety has never had enough people skilled in graphic design on board. We
have a near-insatiable appetite for posters, presentations, animations,
leaflets and documents of other kinds. Please help us &mdash; Tom got a D in
GCSE art!</p>
EOF
    );

    print $q->header(-type => 'text/html; charset=utf-8'),
            start_html($q, 'Getting involved');

    my $choose = "Tasks for:";
    foreach (qw(nontech programmer designer)) {
        my $u = $q->url(-relative => 1) . "?skills=$_";
        $choose .= ($_ eq 'nontech' ? ' ' : '; ')
                    . $q->a({ -href => $u }, $skills_desc{$_});
    }
    
    if ($skills_needed) {
        $blurb = $blurb{$skills_needed};
        print $q->h2("Tasks suitable for $skills_desc{$skills_needed}"),
                $q->p($choose);
    }

    print $blurb;
    
    if (!$skills_needed) {
        print end_html($q);
        return;
    }

    my $ntasks = $dbh->selectrow_array("
                    select count(tn) from ticket where extra1 = ?
                ", {}, $skills_needed);

    if ($ntasks == 0) {
        print $q->p("Unfortunately, there aren't any volunteer tasks suitable for $skills_desc{$skills_needed} at the moment. Please drop us a mail to", $q->a({ -href => 'mailto:team@mysociety.org' }, 'team@mysociety.org'), "to ask how you can get involved");
    } else {
        my %howlong_desc = (
                '30mins' => 'up to half an hour',
                '3hours' => 'up to three hours',
                'more' => 'longer than three hours'
            );

        foreach my $howlong (qw(30mins 3hours more dunno)) {
            my $desc = $howlong_desc{$howlong};
            if ($desc) {
                $desc = "Tasks which will take $desc";
            } else {
                $desc = "Tasks of unknown duration";
            }

            my $ntasks = $dbh->selectrow_array("
                            select count(tn) from ticket
                            where extra1 = ? and extra2 = ?
                        ", {}, $skills_needed, $howlong);
            next if ($ntasks == 0);
#            print $q->h3($desc),
#                    $q->start_ul({ -class => 'tasklist' });

            print '</div>',     # end item
                    '<div class="item_foot"></div>',
                    '<div class="item_head"></div>',
                    '<div class="item">', # start item
                    $q->h3($desc),
                    $q->start_ul();

            my $s = $dbh->prepare("
                            select tn, origtime, changetime, extra1, extra2
                            from ticket
                            where extra1 = ?
                                and extra2 = ?
                            order by max(origtime, changetime) desc
                        ");

            $s->execute($skills_needed, $howlong);
            while (my ($tn, $orig, $change, $extra1, $extra2) = $s->fetchrow_array()) {
                my ($heading, $content) = html_format_ticket($tn);
                my $timestamp = strftime('%A, %e %B %Y', localtime($orig));
                $timestamp .= " (changed "
                                . strftime('%A, %e %B %Y', localtime($change))
                                . ")"
                    if (strftime('%A, %e %B %Y', localtime($change)) ne $timestamp);
                print $q->li(
                        $q->h4($heading, $q->a({ -class => 'permalink', -href => $q->url(-relative => 1) . "?tn=$tn;register=1" }, "#")),
                        #$q->span({ -class => 'when' }, "Created: " . $timestamp),
                        $q->div($content),
                        $q->div({ -class => 'signup' },
                            $q->start_form(-method => 'GET'),
                            $q->hidden(-name => 'tn', -value => $tn),
                            $q->hidden(-name => 'prevurl', -value => $q->self_url()),
                            $q->submit(-name => 'register', -value => "I'm interested >>>"),
                            $q->end_form()
                        ));
            }

            print $q->end_ul();
        }
    }

    print end_html($q);
}

sub do_register_page ($) {
    my $q = shift;

    my $tn = $q->param('tn');

    my ($orig, $change, $extra1, $extra2)
        = $dbh->selectrow_array("
                        select origtime, changetime, extra1, extra2
                        from ticket
                        where tn = ?", {}, $tn);
    my ($heading, $content) = html_format_ticket($tn);

    if (!$extra1 || $extra1 !~ /^(nontech|programmer|designer)$/
        || !$extra2 || $extra2 !~ /^(dunno|30mins|3hours|more)/
        || !$heading || !$content) {
        print $q->header(
                    -status => 400,
                    -type => 'text/html; charset=utf-8'),
                start_html($q, 'Oops'),
                $q->p("We couldn't find a volunteer task for the link you clicked on."),
                end_html($q);
        return;
    }

    my $name = $q->param('name');
    $name ||= $q->cookie('mysociety_volunteer_name');
    $name ||= '';
    $q->param('name', $name);
    my $email = $q->param('email');
    $email ||= $q->cookie('mysociety_volunteer_email');
    $email ||= '';
    $q->param('email', $email);

    my @errors = ( );
    push(@errors, "Please enter your name so that we know who you are") if (!$name);
    if ($email) {
        push(@errors, "The email address '$email' doesn't appear to be valid; please check it and try again")
            if (!mySociety::Util::is_valid_email($email))
    } else {
        push(@errors, "Please enter your email address so we can get in touch with you");
    }

    if ($q->param('edited') && !@errors) {
        # XXX should be in transaction.
        $dbh->do('
                delete from volunteer_interest
                where ticket_num = ? and email = ?', {},
                $tn, $email);
        $dbh->do('
                insert into volunteer_interest
                    (ticket_num, name, email, whenregistered)
                values (?, ?, ?, ?)', {},
                $tn, $name, $email, time());
        my $mysociety_email = "volunteers\@mysociety.org";
        mySociety::EvEl::send(
            { 'To' => $email, 'Cc' => $mysociety_email, 
              'From' => $mysociety_email, 
              'Subject' => "mySociety task: $heading",
              '_unwrapped_body_' => "$name,

Thanks for expressing an interesting in helping with this task:
$heading

First thing to do is drop us an email telling us a bit about who
and where you are, and what ideas you have for doing this task in
interested and exciting ways.

You can find the task in our ticket tracking system here. Please add
remarks with any information you find out, and as you make progress.
https://secure.mysociety.org/cvstrac/tktview?tn=$tn

Good luck!

-- the mySociety team
"
            }, [ $email, $mysociety_email ]);
        my $url = $q->param('prevurl');
        $url ||= 'http://www.mysociety.org/';
        print $q->header(
                -type => 'text/html; charset=utf-8' ,
                -cookie => [
                    $q->cookie(
                        -name => 'mysociety_volunteer_name',
                        -value => $name,
                        -expires => '+1y',
                        -domain => 'mysociety.org'
                    ),
                    $q->cookie(
                        -name => 'mysociety_volunteer_email',
                        -value => $email,
                        -expires => '+1y',
                        -domain => 'mysociety.org'
                    ),
                ]
            );

        print start_html($q, "Express an interest in task #$tn");
        print $q->p("Thanks for expressing an interest in the task '$heading'.");
        print $q->p("You've been sent an email putting you in touch
        with people at mySociety, so you can ask any questions.");
        print $q->p( $q->a({ -href => $url }, 'Back to task list'));
        print end_html($q);
        return;
    }

    print $q->header(-type => 'text/html; charset=utf-8'),
            start_html($q, "Express an interest in task #$tn");

    # Reproduce the task.
    my $timestamp = strftime('%A, %e %B %Y', localtime($orig));
    $timestamp .= " (changed "
                    . strftime('%A, %e %B %Y', localtime($change))
                    . ")"
        if (strftime('%A, %e %B %Y', localtime($change)) ne $timestamp);

    print $q->div("Fill in the form below to express your interest in this task.");

    print $q->ul($q->li(
            $q->h4($heading),
            # $q->span({ -class => 'when' }, $timestamp),
            $q->div($content)
        ));

    if ($q->param('edited') && @errors) {
        print $q->ul($q->li([map { encode_entities($_) } @errors]));
    }

    print $q->start_form(-method => 'POST'),
            $q->hidden(-name => 'edited', -value => '1'),
            $q->hidden(-name => 'tn'),
            $q->hidden(-name => 'prevurl'),
            $q->start_table({ -id => 'volunteerform' }),
            $q->Tr(
                $q->th('Name'),
                $q->td($q->textfield(-name => 'name', -size => 30))
            ),
            $q->Tr(
                $q->th('Email'),
                $q->td($q->textfield(-name => 'email', -size => 30))
            ),
            $q->Tr(
                $q->th(),
                $q->td($q->submit(-name => 'register',
                        -value => 'Register my interest'))
            ),
            $q->end_table(),
            $q->end_form(),
            end_html($q);
}

while (my $q = new CGI::Fast()) {
#    $q->encoding('utf-8');
    $q->autoEscape(0);
    my $tn = $q->param('tn');
    if ($q->param('register') && defined($tn) && $tn =~ /^[1-9]\d*$/) {
        do_register_page($q);
    } else {
        do_list_page($q);
    }
}

$dbh->disconnect();

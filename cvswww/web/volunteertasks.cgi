#!/usr/bin/perl -w -I../../perllib -I../perllib
#
# volunteertasks.cgi:
# Simple interface for viewing volunteer tasks from the CVSTrac database.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#

my $rcsid = ''; $rcsid .= '$Id: volunteertasks.cgi,v 1.27 2008-02-04 22:50:28 matthew Exp $';

use strict;
require 5.8.0;

use CGI qw(-no_xhtml);
use mySociety::CGIFast;
use DBI;
use HTML::Entities;
use POSIX;
use WWW::Mechanize;
use Data::Dumper;
use LWP::Simple;

use mySociety::Config;
mySociety::Config::set_file('../conf/general');

use mySociety::EmailUtil;
use mySociety::EvEl;

use CVSWWW;

my $mysociety_email = "volunteers" . chr(64) . "mysociety.org";

my $dbh = CVSWWW::dbh();

my %skills = map { $_->[0] => $_->[1] }
                @{$dbh->selectall_arrayref(
                    "select name, value from enums where type = 'extra1'")
                };
my %times = map { $_->[0] => lc($_->[1]) }
                @{$dbh->selectall_arrayref(
                    "select name, value from enums where type = 'extra2'")
                };

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

<p>
Come and hang out in our <a href="http://www.irc.mysociety.org">public internet chat room</a>!
</p>
EOF

    my %blurb = (
        nontech => <<EOF,
<p>People sometimes think that we're only interested in programmers, but
nothing could be further from the truth. We want our various
sites to grow, and to be more and more widely used by people in as
many places as possible: people who can tell their friends, put up
posters, hold dinners, write to newspapers, design brochures and so on
are invaluable to us. Just look at the list below and click to tell us
how you can help!</p>
EOF
        programmer => <<EOF,
<p>We're an Open Source languages shop, but beyond that we're very free and
easy about what people use. Perl, PHP, Python, Javascript and Ruby have all
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
            CVSWWW::start_html($q, 'Getting involved');

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
        print CVSWWW::end_html($q);
        return;
    }

    my $ntasks = $dbh->selectrow_array("
                    select count(tn) from ticket where extra1 = ? and status = 'new'
                ", {}, $skills_needed);

    if ($ntasks == 0) {
        print $q->p("Unfortunately, there aren't any volunteer tasks suitable for $skills_desc{$skills_needed} at the moment. Please drop us a mail to", $q->a({ -href => 'mailto:team&#64;mysociety.org' }, 'team&#64;mysociety.org'), "to ask how you can get involved");
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
                            where extra1 = ? and extra2 = ? and status = 'new'
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
                                and extra2 = ? and status = 'new'
                            order by max(origtime, changetime) desc
                        ");

            $s->execute($skills_needed, $howlong);
            while (my ($tn, $orig, $change, $extra1, $extra2) = $s->fetchrow_array()) {
                my ($heading, $content) = CVSWWW::html_format_ticket($tn);
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
                            $q->hidden(-name => 'prevurl', -value => $q->url(-relative => 1, -path => 1, -query => 1)),
                            $q->submit(-name => 'register', -value => "I'm interested >>>"),
                            $q->end_form()
                        ));
            }

            print $q->end_ul();
        }
    }

    print CVSWWW::end_html($q);
}

sub do_register_page ($) {
    my $q = shift;

    my $tn = $q->param('tn');

    my ($orig, $change, $extra1, $extra2, $status)
        = $dbh->selectrow_array("
                        select origtime, changetime, extra1, extra2, status
                        from ticket
                        where tn = ?", {}, $tn);
    my ($heading, $content) = CVSWWW::html_format_ticket($tn);

    if (!$extra1 || $extra1 !~ /^(nontech|programmer|designer)$/
        || !$extra2 || $extra2 !~ /^(dunno|30mins|3hours|more)/
        || !$heading || !$content) {
        print $q->header(
                    -status => 400,
                    -type => 'text/html; charset=utf-8'),
                CVSWWW::start_html($q, 'Oops'),
                $q->p("We couldn't find a volunteer task for the link you clicked on.",
        	    $q->a({ -href => '/volunteertasks' }, 'Back to task list')),
                CVSWWW::end_html($q);
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
            if (!mySociety::EmailUtil::is_valid_email($email))
    } else {
        push(@errors, "Please enter your email address so we can get in touch with you");
    }

    if ($status eq 'new' && $q->param('edited') && !@errors) {
        # all go - perform the signup


        # don't want dupes...
        $dbh->do('
                delete from volunteer_interest
                where ticket_num = ? and email = ?', {},
                $tn, $email);

        # build up list of people to CC the "hello" email
        my @ccers = ( $mysociety_email );
#        my $sth = $dbh->prepare("select name, email
#            from volunteer_interest
#            where ticket_num = ? and ccme = 'TRUE'");
#        $sth->execute( $tn );
#        while (my ($vname, $vemail)  = $sth->fetchrow_array()) {
#            push @ccers, $vemail;
#        }
#        $sth->finish;

        # Insert volunteer into the DB
        $dbh->do('
                insert into volunteer_interest
                    (ticket_num, name, email, whenregistered, ccme )
                values (?, ?, ?, ?, ? )', {},
                $tn, $name, $email, time(),
                $q->param('ccme') ? 'TRUE' : 'FALSE'
                );

        my @recipients = ($email, @ccers);
        my $s = decode_entities($heading);

        mySociety::EvEl::send(
            { 'To' => [[$email,$name]], 'Cc' => \@ccers, 
              'From' => $mysociety_email, 
              'Subject' => "mySociety task: $s",
              '_unwrapped_body_' => "$name,

Thanks for expressing an interest in helping with this task:
'$s'.

If you need any information before you can get going, or after you've begun, please join our public developers' email list:

    https://secure.mysociety.org/admin/lists/mailman/listinfo/developers-public

where other mySociety volunteers and developers will be happy to help you. Or, if your request or question isn't suitable for a public mailing list, just hit reply to this email.

You could also pop by our public internet chat room and say hi to us:

    http://www.irc.mysociety.org/


The task also has a page in our internal system, where we can track its progress.  To add any information you have, click the \"Add remarks\" link on this page:

    https://secure.mysociety.org/cvstrac/tktview?tn=$tn

Or again, post any results you get to our mailing list or email us.
 
Good luck, and we hope to hear from you soon!

-- the mySociety team
"
            }, \@recipients );

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

        print CVSWWW::start_html($q, "Express an interest in task #$tn");
        print $q->p("Thanks for expressing an interest in the task '$heading'.");
        print $q->p("You've been sent an email putting you in touch
        with people at mySociety, so you can ask any questions.");
        print $q->p( $q->a({ -href => $url }, 'Back to task list'));


    
        print CVSWWW::end_html($q);
        return;
    }

    print $q->header(-type => 'text/html; charset=utf-8'),
            CVSWWW::start_html($q, "Express an interest in task #$tn");

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

    if ($status eq 'new') {
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
                $q->th(''),
                $q->td($q->checkbox(-name => 'ccme', -checked => 1,
                    -label => "Let me know when other volunteers sign up to this task"))
            ),
            $q->Tr(
                $q->th(),
                $q->td($q->submit(-name => 'register',
                        -value => 'Register my interest'))
            ),
            $q->end_table(),
            $q->end_form();
    } else {
        print $q->p($q->big($q->em('This task has now been closed; use the Comment link below if you wish to add a comment.')));
	print $q->p($q->a({ -href => '/volunteertasks' }, 'Back to task list'));
    }

    print $q->p( $q->a({ -href => "https://secure.mysociety.org/cvstrac/tktview?tn=$tn" }, "Comment on or update this task")),
        CVSWWW::end_html($q);
}

while (my $q = new mySociety::CGIFast()) {
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

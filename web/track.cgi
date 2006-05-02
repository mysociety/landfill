#!/usr/bin/perl -w -I../../perllib -I../perllib
#
# track.cgi:
# Return a web bug image, with a tracking cookie; log any associated
# information to the database.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#

my $rcsid = ''; $rcsid .= '$Id: track.cgi,v 1.27 2006-05-02 14:49:44 chris Exp $';

use strict;

require 5.8.0;

BEGIN {
    use mySociety::Config;
    mySociety::Config::set_file('../conf/general');
}

use CGI::Fast;
use Digest::SHA1;
use Error qw(:try);
use HTML::Entities;
use IO::Handle;
use POSIX qw();

use mySociety::DBHandle qw(dbh);
use mySociety::Tracking;

use Track;

binmode(STDOUT, ':bytes');
STDOUT->autoflush(1);

# Private secret for cookie verificatioon.
my $private_secret = Track::DB::secret();

# Secret shared with tracking sites.
my $secret = mySociety::Config::get('TRACKING_SECRET');

# Details of cookie presented to user.
my $cookiename = mySociety::Config::get('TRACKING_COOKIE_NAME', 'track_id');
my $cookiepath = mySociety::Config::get('TRACKING_COOKIE_PATH', '/');
my $cookiedomain = mySociety::Config::get('TRACKING_COOKIE_DOMAIN');
my $cookiesecure = mySociety::Config::get('TRACKING_COOKIE_SECURE', 0);

sub id_from_cookie ($) {
    my $cookie = shift;
    my ($salt, $id, $sign) = split(/,/, $cookie);
    return undef unless ($salt && $id && $sign);
    return undef unless ($id =~ /^[1-9]\d*$/);
    return undef unless (Digest::SHA1::sha1_base64("$private_secret,$salt,$id") eq $sign);
    return $id;
}

sub cookie_from_id ($) {
    my $id = shift;
    my $salt = sprintf('%08x', rand(0xffffffff));
    my $sign = Digest::SHA1::sha1_base64("$private_secret,$salt,$id");
    return "$salt,$id,$sign";
}

my %useragent_cache;
my $n_useragents_cached = 0;        # XXX avoid dumb DoS problem

my $lastcommit = time();
my $n_since_lastcommit = 0;
sub do_commit () {
    dbh()->commit();
    $lastcommit = time();
    $n_since_lastcommit = 0;
}

sub do_web_bug ($$$) {
    my ($q, $track_id, $track_cookie) = @_;

    print $q->header(
                # This is bad evil and wrong, but after spending a while trying
                # to understand the P3P spec I gave up in despair....
                -p3p => 'CP="IDC DSP COR CURa ADMa OUR IND PHY ONL COM STA"',
                -cookie => $q->cookie(
                        -name => $cookiename,
                        -value => $track_cookie,
                        -expires => '+30d',
                        -path => $cookiepath,
                        -domain => $cookiedomain,
                        -secure => $cookiesecure
                    ),
                -type => 'image/png',
                -content_length => Track::transparent_png_image_length
            ), Track::transparent_png_image;

    STDOUT->flush();

    return if ($track_cookie eq 'do-not-track-me');

    # Got that over with; now do any data recording we want.
    my $ipaddr = $q->remote_addr();
    my $ua = $q->user_agent();

    # Required parameters.
    my $salt = $q->param('salt');
    my $url = $q->param('url');
    my $sign = $q->param('sign');
    return if (!$salt || !$url || !$sign);
    
    my $extra = $q->param('extra');
    
    my $D = new Digest::SHA1();
    $D->add("$secret\0$salt\0$url");
    $D->add("\0$extra") if ($extra);

    my $sign2 = $D->hexdigest();
    if (lc($sign2) ne lc($sign)) {
        # May as well warn here as it may help with debugging.
        warn "signature does not match (passed $sign vs computed $sign2)\n"
                . "  passed salt = $salt\n"
                . "          url = $url\n"
                . "        extra = " . ($extra ? $extra : '(none)') . "\n";
        return;
    }

    # OK, the data are valid.
    my $docommit = 0;
    my $ua_id;
    if ($ua) {
        $ua_id = $useragent_cache{$ua};
        if (!$ua_id) {
            $ua_id = dbh()->selectrow_array('select id from useragent where useragent = ?', {}, $ua);
            if (!$ua_id) {
                local dbh()->{HandleError};
                $ua_id = dbh()->selectrow_array("select nextval('useragent_id_seq')");
                dbh()->do('insert into useragent (id, useragent) values (?, ?)', {}, $ua_id, $ua);
                dbh()->commit();
            }
        }
    }
    
    dbh()->do('insert into event (tracking_id, ipaddr, useragent_id, url, extradata) values (?, ?, ?, ?, ?)', {}, $track_id, $ipaddr, $ua_id, $url, $extra);
    ++$n_since_lastcommit;

    # XXX Commits are slow but their cost does not depend very much on the
    # number of rows committed. So ideally we want to batch them up; however,
    # might we hit some nasty concurrency issue?
    try {
        do_commit(); # if ($docommit || $n_since_lastcommit > 10 || $lastcommit < time() - 10);
    } catch mySociety::DBHandle::Error with {
        my $E = shift;
        warn "database error: " . $E->text() . " committing rows to tracking database";
    };
}

sub start_html ($;$) {
    my ($q, $title) = @_;
    return $q->start_html(
                -title => 'mySociety User Tracking'
                            . ($title ? ": " . encode_entities($title) : ''),
                -style => { src => 'global.css' },
                -encoding => 'utf-8'
            ) . <<EOF
<div class="top">
<div class="masthead"><img src="mslogo.gif" alt="mySociety.org"/></div>
</div>
<div class="page-body">

<div id="content" class="narrowColumn">
EOF
            . $q->h1(
                    "User Tracking"
                    . ($title ? ": " . encode_entities($title) : '')
            );
}

sub end_html ($) {
    return <<EOF;
</div></div></body></html>
EOF
}

sub do_ui_page ($$$) {
    my ($q, $track_id, $track_cookie) = @_;

    if ($q->param('optout')) {
        if ($track_id) {
            dbh()->do('delete from event where tracking_id = ?', {}, $track_id);
            do_commit();
        }
        print $q->header(
                -cookie => $q->cookie(
                        -name => $cookiename,
                        -value => 'do-not-track-me',
                        -expires => '+3650d',
                        -path => $cookiepath,
                        -domain => $cookiedomain,
                        -secure => $cookiesecure
                    ),
                -type => 'text/html; charset=utf-8'
            ),
            start_html($q, 'Opt Out'),
            <<EOF,
<p>You have now opted out of our user tracking, <em>on this computer and web
browser</em> &mdash; if you use other computers you will need to opt out on
them as well to ensure that we don't record any more data about you.</p>

<h2>Details</h2>

<p>We have (tried to) set a cookie in your browser with the value
"do-not-track-me". The user tracking service does not record any data about
users whose browsers present a cookie with that value. We have also deleted
all of the data we held which was associated with your old tracking ID.</p>

<h2>Other ways to opt out</h2>

<p>If you'd prefer not to have a cookie from us at all, you can just remove it
and tell your browser not to accept another one. Alternatively, if you are
using a browser which lets you block images from specific sites, you can just
block images from this site.</p>
EOF
            end_html($q);
    } elsif ($q->param('showme')) {
        print $q->header(
                -type => 'text/html; charset=utf-8'
            ),
            start_html($q, "Show Me My Data");
        if (defined($track_id)) {
            print
                $q->h2('Cookie data'),
                $q->p('This is the data we use to distinguish you from other users:'),
                $q->start_table(),
                $q->Tr($q->th('Cookie value'), $q->td(encode_entities($track_cookie))),
                $q->Tr($q->th('Tracking ID'), $q->td($track_id)),
                $q->end_table(),

                $q->h2('Track data'),
                $q->p('This is the data we record centrally about your use of our websites:'),
                $q->start_table({ -style => 'width: 100%;' }),
                $q->Tr($q->th(['Event ID', 'When logged', 'Your IP', 'Your user-agent', 'URL visited', 'Other data']));

            my $s = dbh()->prepare('
                        select event.id, extract(epoch from whenlogged),
                            ipaddr, useragent, url, extradata
                        from event left join useragent
                            on event.useragent_id = useragent.id
                        where tracking_id = ?
                        order by event.id');
            $s->execute($track_id);
            my $old_ua;
            while (my ($id, $when, $ip, $ua, $url, $extra) = $s->fetchrow_array()) {
                if (!$ua) {
                    $ua = '&mdash;';
                } elsif ($old_ua && $ua eq $old_ua) {
                    $ua = '&quot;';
                } else {
                    $old_ua = $ua;
                    encode_entities($ua);
                }

                $when = POSIX::strftime("%Y-%m-%d %H:%M:%S", localtime($when));

                utf8::encode($extra);
                $extra =~ s/([^\x20-\x7f])/sprintf('\%02x', ord($1))/ge;

                print $q->Tr(
                        $q->td([
                            $id,
                            $when,
                            $ip,
                            $ua,
                            encode_entities($url),
                            encode_entities($extra)
                        ]));
            }
            print $q->end_table(),
                    mySociety::Tracking::code($q, 'track: show recorded data'),
                    end_html($q);
        } else {
            print <<EOF;
<p>Your browser didn't send us a cookie with a tracking ID in it, so we can't
show you any data about you automatically. If you use more than one computer
or web browser, you may wish to try this from those other computers or
browsers &mdash; we record data separately for each one.</p>
EOF
        }
        print end_html($q);
    } else {
        my $actionsform =
            $q->start_form(-method => 'POST', -action => './')
                . $q->hidden(-name => 'ui')
                . $q->submit(
                    -name => 'optout',
                    -value => 'Opt out of user tracking'
                ) . ' ' . $q->submit(
                    -name => 'showme',
                    -value => 'Show me the data you hold about me'
                ) . $q->end_form();
    
        print $q->header(
                -type => 'text/html; charset=utf-8'
            ),
            start_html($q),
            <<EOF,
<h2>Quick Summary</h2>

<ul>
<li>We use cookies and a "web bug" to track users, so that we can find out
which parts of our sites users find confusing, and fix them.</li>
<li>We will never sell, rent, give away or otherwise disseminate this data.</li>
<li>We allow anyone to view the data we hold about them, or to opt out of this
tracking, whenever they want.</li>
EOF

            (
                $track_cookie eq 'do-not-track-me'
                ? <<EOF
<li>You have opted out of being tracked (on this computer and browser, at
least). We will not collect any tracking data about you, and have deleted what
we had stored.</li>
EOF
                : <<EOF
<li>You can view the tracking data that we hold about you, or opt out of being
tracked in this way, at any time, using these buttons:<br/>
$actionsform
</li>
EOF
            ), <<EOF,
</ul>

<h2>Context</h2>
 
<p>At mySociety we're obsessed with making our sites usable &mdash; our goal is
always that by the time people leave one of our sites they've successfully
squeezed every ounce of goodness out of it.</p>

<p>In order to do this better we're building and rolling out a system
which allows us to understand where people get confused or lost on our
sites, and so helps us improve things so more people leave having got
what they wanted to from our sites.</p>

<h2>How?</h2>

<p>In order to tell if a page is confusing for users, we monitor how many
users successfully navigate from one page to the next, or how many
people successfully click on a confirmation link in an email. By
comparing data from two versions of the same page, or two versions of
the same email, we can work out which page is better at getting more
people 'through', and which is more confusing and causes more people
to leave the site.</p>

<h2>What does this mean to me?</h2>

<p>Some people might be concerned by the fact that we record how they and other
users make use of our sites. In order to be completely transparent (unlike most
services) we allow our users to see what data has been kept on their use of our
sites. Nobody else can see this except the user concerned. Further, our users
can opt out of tracking at any time.</p>
EOF
            ($track_cookie eq 'do-not-track-me'
                # XXX should have opt back in button
                ? "<p>You have already opted out of our user tracking.</p>"
                : $actionsform),
            <<EOF,

<h2>What data do you use for your analysis?</h2>

<p>We do not use names or email addresses for our analysis &mdash; we are only
interested in how users en-masse succeed or fail to use parts of our sites. In
accordance with the rest of our site's privacy policies, we will never sell,
rent, give away or otherwise disseminate any name, email or address data you
submit to any of our sites, except with your permission and when required to
make the sites function (for instance, when you write to an elected
representative via WriteToThem.com, your name and address are passed to the
recipient).</p>

<h2>How does it work, technically?</h2>

<p>We use a "web bug" &mdash; a tiny, transparent image which can be added to
any page without altering its appearance, and a cookie containing a unique ID
number which is associated with that image. Each page that uses user-tracking
contains a reference to that image, and whenever we receive a request for the
image, we record some information from the referring page (unless the value of
the cookie is "do-not-track-me", indicating that the user has opted out).</p>

<h2>More information</h2>

<p>The Electronic Frontier Foundation has a
<a href="http://www.eff.org/Privacy/Marketing/web_bug.html">list of frequently
asked questions and answers</a> about "web bugs" and user tracking, and
Wikipedia has <a href="http://en.wikipedia.org/wiki/Web_bug">an article on
them</a> (though under the less-insidious-sounding name "web beacon").</p>

<p>If you have any questions specifically about mySociety's user tracking
system, please send them by email to
<a href="mailto:privacy\@mysociety.org">privacy\@mysociety.org</a>.</p>

EOF
            mySociety::Tracking::code($q, 'track: about'),
            end_html($q);
    }
}

while (my $q = new CGI::Fast()) {
    $q->charset('utf-8');

    # Do we already have a cookie, and if so, is it valid?
    my $track_cookie = $q->cookie($cookiename);

    my $track_id;
    if (!$track_cookie
        || ($track_cookie ne 'do-not-track-me'
            && !defined($track_id = id_from_cookie($track_cookie)))) {
        # Need new ID and cookie.
        $track_id = dbh()->selectrow_array("select nextval('tracking_id_seq')");
        $track_cookie = cookie_from_id($track_id);
        dbh()->do('insert into event (tracking_id, ipaddr, url, extradata) values (?, ?, ?, ?)', {},
                    $track_id,
                    $q->remote_addr(),
                    $q->url(),
                    'track: new tracking id'
                );
        do_commit();
    }

    if ($q->param('ui')) {
        do_ui_page($q, $track_id, $track_cookie);
    } else {
        do_web_bug($q, $track_id, $track_cookie);
    }
}

do_commit() if ($n_since_lastcommit > 0);

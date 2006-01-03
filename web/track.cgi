#!/usr/bin/perl -w -I../../perllib -I../perllib
#
# track.cgi:
# Return a web bug image, with a tracking cookie; log any associated
# information to the database.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#

my $rcsid = ''; $rcsid .= '$Id: track.cgi,v 1.9 2006-01-03 12:52:05 chris Exp $';

use strict;

require 5.8.0;

BEGIN {
    use mySociety::Config;
    mySociety::Config::set_file('../conf/general');
}

use CGI::Fast;
use Digest::SHA1;
use HTML::Entities;

use mySociety::DBHandle qw(dbh);

use Track;

binmode(STDOUT, ':bytes');
STDOUT->autoflush(1);

# Private secret for cookie verificatioon.
my $private_secret = Track::DB::secret();

# Secret shared with tracking sites.
my $secret = mySociety::Config::get('TRACK_SECRET');

# Details of cookie presented to user.
my $cookiename = mySociety::Config::get('TRACK_COOKIE_NAME', 'track_id');
my $cookiepath = mySociety::Config::get('TRACK_COOKIE_PATH', '/');
my $cookiedomain = mySociety::Config::get('TRACK_COOKIE_DOMAIN');
my $cookiesecure = mySociety::Config::get('TRACK_COOKIE_SECURE', 0);

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

    return if ($track_cookie eq 'do-not-track-me');

    # Got that over with; now do any data recording we want.
    my $ipaddr = $q->remote_addr();
    my $ua = $q->user_agent();

    # Required parameters.
    my $salt = $q->param('salt');
    my $url = $q->param('url');
    my $sign = $q->param('sign');
    return if (!$salt || !$url || !$sign);
    
    my $other = $q->param('other');
    
    my $D = new Digest::SHA1();
    $D->add("$secret\0$salt\0$url\0");
    $D->add("$other\0") if ($other);

    my $sign2 = $D->hexdigest();
    if (lc($sign2) ne lc($sign)) {
        # May as well warn here as it may help with debugging.
        warn "signature does not match (passed $sign vs computed $sign2)\n";
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
                dbh()->do('lock table useragent in share mode');
                $ua_id = dbh()->selectrow_array('select id from useragent where useragent = ?', {}, $ua);
                if (!$ua_id) {
                    $ua_id = dbh()->selectrow_array("select nextval('useragent_id_seq')");
                    dbh()->do('insert into useragent (id, useragent) values (?, ?)', {}, $ua_id, $ua);
                    warn "saw new user-agent '$ua' for first time\n";
                    $docommit = 1;
                }
                if ($n_useragents_cached < 1000) {
                    $useragent_cache{$ua} = $ua_id;
                }
            }
        }
    }
    
    dbh()->do('insert into event (tracking_id, ipaddr, useragent_id, url, extradata) values (?, ?, ?, ?, ?)', {}, $track_id, $ipaddr, $ua_id, $url, $other);
    ++$n_since_lastcommit;

    # XXX Commits are slow but their cost does not depend very much on the
    # number of rows committed. So ideally we want to batch them up; however,
    # might we hit some nasty concurrency issue?
    do_commit() if ($docommit || $n_since_lastcommit > 10 || $lastcommit < time() - 10);
}

sub do_ui_page ($$$) {
    my ($q, $track_id, $track_cookie) = @_;

    my $fn = $q->param('fn');
    $fn ||= '';
    if ($fn eq 'optout') {
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
            $q->start_html("mySociety User Tracking: Opt Out"),
            <<EOF,
<h1>mySociety User Tracking: Opt Out</h1>

<p>You have now opted out of our user tracking, <em>on this computer and web
browser</em> &mdash; if you use other computers you will need to opt out on
them as well to ensure that we don't record any more data about you.</p>

<h2>Details</h2>

<p>We have (tried to) set a cookie in your browser with the value
"do-not-track-me". The user tracking service does not record any data about
users whose browsers present a cookie with that value.</p>
EOF
            $q->end_html();
    } elsif ($fn eq 'showme') {
        print $q->header(
                -type => 'text/html; charset=utf-8'
            ),
            $q->start_html("mySociety User Tracking: Show My Data"),
            $q->h1('mySociety User Tracking: Show My Data');
        if (defined($track_id)) {
            print
                $q->h2('Cookie data'),
                $q->p('This is the data we use to distinguish you from other users:'),
                $q->start_table(),
                $q->Tr($q->th('Cookie value'), $q->td(encode_entities($track_cookie))),
                $q->Tr($q->th('Tracking ID'), $q->td($track_id)),

                $q->h2('Track data'),
                $q->p('This is the data we record centrally about your use of our websites:'),
                $q->start_table(),
                $q->Tr($q->th(['Event ID', 'When logged', 'Your IP', 'Your user-agent', 'URL visited', 'Other data']));

            my $s = dbh()->prepare('
                        select event.id, whenlogged, ipaddr, useragent, url,
                            extradata
                        from event, useragent
                        where tracking_id = ?
                            and event.useragent_id = useragent.id
                        order by event.id');
            $s->execute($tracking_id);
            my $old_ua;
            while (my ($id, $when, $ip, $ua, $url, $extra) = $s->fetchrow_array()) {
                if (!$ua) {
                    $ua = '&mdash;';
                } elsif ($old_ua && $ua eq $old_ua) {
                    $ua = '&quot;';
                } else {
                    encode_entities($ua);
                }

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
                    $q->end_html();
        } else {
            print <<EOF;
<p>Your browser didn't send us a cookie with a tracking ID in it, so we can't
show you any data about you automatically. If you use more than one computer
or web browser, you may wish to try this from those other computers or
browsers &mdash; we record data separately for each one.</p>
EOF
        }
        print $q->end_html();
    }
}

while (my $q = new CGI::Fast()) {
    # Do we already have a cookie, and if so, is it valid?
    my $track_cookie = $q->cookie($cookiename);

    my $track_id;
    if (!$track_cookie
        || ($track_cookie ne 'do-not-track-me'
            && !defined($track_id = id_from_cookie($track_cookie))) {
        # Need new ID and cookie.
        $track_id = dbh()->selectrow_array("select nextval('tracking_id_seq')");
        $track_cookie = cookie_from_id($track_id);
        do_commit();
    }

    my $fn = $q->param('ui');
    if ($ui) {
        do_ui_page($q, $track_id, $track_cookie);
    } else {
        do_web_bug($q, $track_id, $track_cookie);
    }
}

do_commit() if ($n_since_lastcommit > 0);

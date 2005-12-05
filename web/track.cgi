#!/usr/bin/perl -w -I../../perllib -I../perllib
#
# track.cgi:
# Return a web bug image, with a tracking cookie; log any associated
# information to the database.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#

my $rcsid = ''; $rcsid .= '$Id: track.cgi,v 1.6 2005-12-05 10:31:36 chris Exp $';

use strict;

require 5.8.0;

BEGIN {
    use mySociety::Config;
    mySociety::Config::set_file('../conf/general');
}

use CGI::Fast;
use Digest::SHA1;

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

while (my $q = new CGI::Fast()) {
    # Do we already have a cookie, and if so, is it valid?
    my $track_cookie = $q->cookie($cookiename);
    my $track_id;
    if (!$track_cookie || !defined($track_id = id_from_cookie($track_cookie))) {
        # Need new ID and cookie.
        $track_id = dbh()->selectrow_array("select nextval('tracking_id_seq')");
        $track_cookie = cookie_from_id($track_id);
        dbh()->commit();
    }

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

    # User may opt out of tracking.
    next if ($track_cookie =~ /\.do-not-track$/);

    # Got that over with; now do any data recording we want.
    my $ipaddr = $q->remote_addr();
    my $ua = $q->user_agent();

    # Required parameters.
    my $salt = $q->param('salt');
    my $url = $q->param('url');
    my $sign = $q->param('sign');
    next if (!$salt || !$url || !$sign);
    
    my $other = $q->param('other');
    
    my $D = new Digest::SHA1();
    $D->add("$secret\0$salt\0$url\0");
    $D->add("$other\0") if ($other);

    my $sign2 = $D->hexdigest();
    if (lc($sign2) ne lc($sign)) {
        # May as well warn here as it may help with debugging.
        warn "signature does not match (passed $sign vs computed $sign2)\n";
        next;
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
    if ($docommit || $n_since_lastcommit > 10 || $lastcommit < time() - 10) {
        dbh()->commit();
        $lastcommit = time();
        $n_since_lastcommit = 0;
    }
}


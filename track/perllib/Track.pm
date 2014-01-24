#!/usr/bin/perl
#
# Track.pm:
# Utilities for tracking thing.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: Track.pm,v 1.5 2007-08-02 11:45:04 matthew Exp $
#

package Track::DB;

use strict;

use mySociety::Config;
use mySociety::DBHandle qw(dbh);
use mySociety::Random;
use DBI;

BEGIN {
    mySociety::DBHandle::configure(
            Name => mySociety::Config::get('TRACKING_DB_NAME'),
            User => mySociety::Config::get('TRACKING_DB_USER'),
            Password => mySociety::Config::get('TRACKING_DB_PASS'),
            Host => mySociety::Config::get('TRACKING_DB_HOST', undef),
            Port => mySociety::Config::get('TRACKING_DB_PORT', undef)
        );

    if (!dbh()->selectrow_array('select secret from secret')) {
        local dbh()->{HandleError};
        dbh()->do('insert into secret (secret) values (?)', {}, unpack('h*', mySociety::Random::random_bytes(32)));
        dbh()->commit();
    }
}

=item secret

Return the site shared secret.

=cut
sub secret () {
    return scalar(dbh()->selectrow_array('select secret from secret'));
}

package Track;

# Generate with gimp or whatever then process with,
# od -t x1 NAME.png  \
#    | sed -e 's/^[0-7]*//' -e 's/ /\\x/g' -e 's/^/. "/' -e 's/$/"/'
# or similar.
use constant transparent_png_image =>
    "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52"
    . "\x00\x00\x00\x01\x00\x00\x00\x01\x08\x06\x00\x00\x00\x1f\x15\xc4"
    . "\x89\x00\x00\x00\x04\x67\x41\x4d\x41\x00\x00\xb1\x8f\x0b\xfc\x61"
    . "\x05\x00\x00\x00\x06\x62\x4b\x47\x44\x00\xff\x00\xff\x00\xff\xa0"
    . "\xbd\xa7\x93\x00\x00\x00\x09\x70\x48\x59\x73\x00\x00\x0b\x12\x00"
    . "\x00\x0b\x12\x01\xd2\xdd\x7e\xfc\x00\x00\x00\x07\x74\x49\x4d\x45"
    . "\x07\xd5\x0c\x02\x11\x36\x03\xec\x2f\x0b\x3e\x00\x00\x00\x0d\x49"
    . "\x44\x41\x54\x78\xda\x63\x60\x60\x60\x60\x00\x00\x00\x05\x00\x01"
    . "\x7a\xa8\x57\x50\x00\x00\x00\x00\x49\x45\x4e\x44\xae\x42\x60\x82";

use constant transparent_png_image_length => length(transparent_png_image);

1;

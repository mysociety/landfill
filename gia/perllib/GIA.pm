#!/usr/bin/perl
#
# GIA.pm:
# Global stuff for GIA.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: GIA.pm,v 1.5 2007-08-02 11:45:02 matthew Exp $
#

package GIA::Error;

use strict;
use Error qw(:try);
@GIA::Error::ISA = qw(Error::Simple);

package GIA::DB;

use strict;

use mySociety::Config;
use mySociety::DBHandle qw(dbh);
use mySociety::Random;
use DBI;

BEGIN {
    mySociety::DBHandle::configure(
            Name => mySociety::Config::get('GIA_DB_NAME'),
            User => mySociety::Config::get('GIA_DB_USER'),
            Password => mySociety::Config::get('GIA_DB_PASS'),
            Host => mySociety::Config::get('GIA_DB_HOST', undef),
            Port => mySociety::Config::get('GIA_DB_PORT', undef)
        );

    if (!dbh()->selectrow_array('select secret from secret for update of secret')) {
        local dbh()->{HandleError};
        dbh()->do('insert into secret (secret) values (?)', {}, unpack('h*', mySociety::Random::random_bytes(32)));
    }
    dbh()->commit();
}

=item secret

Return the site shared secret.

=cut
sub secret () {
    return scalar(dbh()->selectrow_array('select secret from secret'));
}

package GIA;

use Digest::SHA1 qw(sha1_hex);

=item token ID

Return a token securely identifying ID.

=cut
sub token ($) {
    my ($id) = @_;
    my $random = sprintf('%04x', int(rand(0xffff)));
    return "$id,$random," . substr(sha1_hex(GIA::DB::secret() . ",$random,$id"), 0, 8);
}

=item check_token TOKEN

If TOKEN is valid, return the associated ID; otherwise return undef.

=cut
sub check_token ($) {
    my ($token) = @_;
    return undef if (!defined($token) || $token !~ /^[1-9]\d*(\.[1-9]\d*)*,[0-9a-f]+,[0-9a-f]+$/i);
    my ($id, $random, $hash) = split(/,/, $token);
    if (substr(sha1_hex(GIA::DB::secret() . ",$random,$id"), 0, 8) eq $hash) {
        return $id;
    } else {
        return undef;
    }
}

1;

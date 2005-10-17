#!/usr/bin/perl
#
# GIA.pm:
# Global stuff for GIA.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: GIA.pm,v 1.1 2005-10-17 12:18:02 chris Exp $
#

package GIA::Error;

use strict;
use Error qw(:try);
@GIA::Error::ISA = qw(Error::Simple);

package GIA::DB;

use strict;

use mySociety::Config;
use mySociety::DBHandle qw(dbh);
use mySociety::Util;
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
        dbh()->{RaiseError} = 0;
        dbh()->do('insert into secret (secret) values (?)', {}, unpack('h*', mySociety::Util::random_bytes(32)));
        dbh()->{RaiseError} = 1;
    }
    dbh()->commit();
}

=item secret

Return the site shared secret.

=cut
sub secret () {
    return scalar(dbh()->selectrow_array('select secret from secret'));
}

1;

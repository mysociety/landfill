#!/usr/bin/perl
#
# PoP.pm:
# Various Placeopedia bits.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: matthew@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: PoP.pm,v 1.1 2005-09-10 10:50:30 matthew Exp $
#

package PoP;

use strict;
use mySociety::Config;
use DBI;

our $dbh;

BEGIN {
    use Exporter ();
    our @ISA = qw(Exporter);
    our @EXPORT = qw($dbh);
    my $dsn = mySociety::Config::get('POP_DB_STR');
    my $db_username = mySociety::Config::get('POP_DB_USER');
    my $db_password = mySociety::Config::get('POP_DB_PASS', undef);
    $dbh = DBI->connect($dsn, $db_username, $db_password);
}

1;

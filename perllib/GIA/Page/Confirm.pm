#!/usr/bin/perl
#
# GIA/Page/Confirm.pm:
# Confirm email address on an item.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: Confirm.pm,v 1.1 2005-10-18 17:40:43 chris Exp $
#

package GIA::Page::Confirm;

use strict;

use Error qw(:try);
use Digest::SHA1 qw(sha1_hex);

use mySociety::DBHandle qw(dbh);

use GIA;
use GIA::Web;

sub render ($$$) {
    my ($q, $hdr, $content) = @_;

    my $t = $q->param('t');

    if (defined($t) && $t =~ /^([1-9]\d*),([0-9a-f]+),([0-9a-f]{8})$/) {
        my ($itemid, $random, $hash) = ($1, $2, $3);

        if ($hash eq substr(sha1_hex(GIA::DB::secret() . ",$random,$itemid"), 0, 8)) {
            dbh()->do('update item set confirmed = true where id = ?', {}, $itemid);
            dbh()->commit();
            $$hdr = $q->header();
            $$content =
                $q->start_html('Thank You!')
                . $q->p('Thanks! We will now pass details of your item on to local charities.')
                . $q->end_html();
            return 0;
        }
    }
    
    # Bad or no token. Just redirect to the home page.
    $$hdr = $q->redirect(mySociety::Config::get('BASE_URL') . '/');
    $$content = '';
}

1;

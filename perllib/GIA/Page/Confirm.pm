#!/usr/bin/perl
#
# GIA/Page/Confirm.pm:
# Confirm email address on an item.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: Confirm.pm,v 1.3 2005-10-21 16:36:44 chris Exp $
#

package GIA::Page::Confirm;

use strict;

use Error qw(:try);

use mySociety::DBHandle qw(dbh);

use GIA;
use GIA::Web;

sub render ($$$) {
    my ($q, $hdr, $content) = @_;
    if (my $itemid = GIA::check_token($q->param('t'))) {
        dbh()->do('update item set whenconfirmed = current_timestamp where id = ?', {}, $itemid);
        dbh()->commit();
        $$hdr = $q->header();
        $$content =
            $q->start_html('Thank You!')
            . $q->p('Thanks! We will now pass details of your item on to local charities.')
            . $q->end_html();
        return 0;
    }
    
    # Bad or no token. Just redirect to the home page.
    $$hdr = $q->redirect(mySociety::Config::get('BASE_URL') . '/');
    $$content = '';
}

1;

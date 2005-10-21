#!/usr/bin/perl
#
# GIA/Page/Decline.pm:
# Page for an acceptor to decline the offer of an item.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: Decline.pm,v 1.1 2005-10-21 17:32:52 chris Exp $
#

package GIA::Page::Decline;

use strict;

use Error qw(:try);

use mySociety::DBHandle qw(dbh);
use mySociety::EvEl;

use GIA;
use GIA::Web;

sub render ($$$) {
    my ($q, $hdr, $content) = @_;
    if (my $ii = GIA::check_token($q->param('t'))) {
        my ($itemid, $acceptorid) = split(/\./, $ii);

        dbh()->do('update acceptor_item_interest set whendeclined = current_timestamp where item_id = ? and acceptor_id = ? and whendeclined is null', {}, $itemid, $acceptorid);
        dbh()->commit();

        $$hdr = $q->header();
        $$content =
            $q->start_html('Thanks')
            . $q->p("We'll look for another charity who might be interested in the item.")
            . $q->end_html();

        return 0;
    }

    # Bad or no token. Just redirect to the home page.
    $$hdr = $q->redirect(mySociety::Config::get('BASE_URL') . '/');
    $$content = '';
}

1;

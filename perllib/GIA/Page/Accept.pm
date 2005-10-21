#!/usr/bin/perl
#
# GIA/Page/Accept.pm:
# Page for an acceptor to accept the offer of an item.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: Accept.pm,v 1.1 2005-10-21 17:32:52 chris Exp $
#

package GIA::Page::Accept;

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

        dbh()->do('lock table acceptor_item_interest in share mode');

        # Check that this is the current acceptor.
        $$hdr = $q->header();
        my $a = dbh()->selectrow_array('select item_current_acceptor(?)', {}, $itemid);
        if ($a == $acceptorid) {
            $$content =
                $q->start_html('Thanks')
                . $q->p("We'll put you in touch with the owner of that item.")
                . $q->end_html();

            my ($donor_name, $donor_email, $description) = dbh()->selectrow_array('select name, email, description from item where id = ?', {}, $itemid);
            my ($acceptor_name, $acceptor_org, $acceptor_email) = dbh()->selectrow_array('select name, organisation, email from acceptor where id = ?', {}, $acceptorid);

            mySociety::EvEl::send({
                    _body_ => <<EOF,

$acceptor_name of $acceptor_org has expressed an interest in this item which
you've donated through GiveItAway:

$description

Please reply to this email to get in touch with $acceptor_name so that they
can arrange to collect the item from you.

Thanks for using GiveItAway!

EOF
                    To => [[$donor_email, $donor_name]],
                    Cc => [[$acceptor_email, "$acceptor_name ($acceptor_org)"]],
                    From => [$acceptor_email, "$acceptor_name ($acceptor_org)"],
                    Subject => 'Interest in your GiveItAway donation'
                }, $donor_email);

            dbh()->do('update acceptor_item_interest set whenaccepted = current_timestamp where item_id = ? and acceptor_id = ?', {}, $itemid, $acceptorid);
            dbh()->commit();
        } else {
            # Actually this bit of the logic is broken, and we should fix it.
            $$content =
                $q->start_html('Sorry')
                . $q->p('Unfortunately, we cannot arrange to send you that item because it has been offered to another charity.')
                . $q->end_html();
        }
        
        return 0;
    }
    
    # Bad or no token. Just redirect to the home page.
    $$hdr = $q->redirect(mySociety::Config::get('BASE_URL') . '/');
    $$content = '';
}

1;

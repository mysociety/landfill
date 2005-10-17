#!/usr/bin/perl
#
# GIA/Page/NewItem.pm:
# Create a new item.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: NewItem.pm,v 1.1 2005-10-17 12:18:02 chris Exp $
#

package NewItem;

use strict;

use GIA;
use GIA::Web;

sub render ($$$) {
    my ($q, $hdr, $content) = @_;

    $$hdr = $q->header();

    $$content =
        $q->start_html('Give Something Away!')
        . $q->p('Please fill out the form below with details of the item you would like to give away:')
        . $q->start_form(-method => 'POST')
            . $q->label({ -for => 'name' }, 'Your name ')
                . $q->textfield(-name => 'name', -id => 'name')
            . $q->label({ -for => 'email' }, 'Your email address ')
                . $q->textfield(-name => 'email', -id => 'email')
            . $q->label({ -for => 'email' }, 'Your postcode ')
                . $q->textfield(-name => 'email', -id => 'email')
                . '(so that we can find nearby charities)'
            . $q->label({ -for => 'category'}, 'What sort of item? ')
                . $q->popup_menu( ... )
            . $q->label({ -for => 'description'}, 'Description of the item')
                . $q->br()
                . $q->textarea(-name => 'description', -id => 'description', -columns => 40, -rows => 5)
        . $q->end_form()
        . $q->end_html();
}

1;

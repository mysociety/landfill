#!/usr/bin/perl
#
# GIA/Page/NewItem.pm:
# Create a new item.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: NewItem.pm,v 1.8 2007-08-02 11:45:07 matthew Exp $
#

package GIA::Page::NewItem;

use strict;

use Error qw(:try);
use Digest::SHA1 qw(sha1_hex);

use mySociety::DBHandle qw(dbh);
use mySociety::EvEl;
use mySociety::MaPit;
use mySociety::EmailUtil qw(is_valid_email);
use mySociety::PostcodeUtil qw(is_valid_postcode);

use GIA;
use GIA::Web;
use GIA::Form;

sub render ($$$) {
    my ($q, $hdr, $content) = @_;

    my ($lat, $lon);

    # Pre-allocate item and location IDs to guard against multiple-insertion.
    $q->param('itemid', dbh()->selectrow_array("select nextval('location_id_seq')"))
        if (!defined($q->param('itemid')));
    $q->param('locid', dbh()->selectrow_array("select nextval('location_id_seq')"))
        if (!defined($q->param('locid')));

    my $form = new GIA::Form([
            ['', 'itemid', 'hidden'],
            ['', 'locid', 'hidden'],
            ['Your email address', 
                'email', 'text',
                sub ($) {
                    return is_valid_email($_[0]) ? undef :
                        'Please enter a full, valid email address'
                }
            ], ['Your name',
                'name', 'text',
                qr/./, 'Please enter your name'
            ], ['Your postcode',
                'postcode', 'text',
                sub ($) {
                    my $pc = shift;
                    my $err = 'Please give your full postcode';
                    return $err if (!is_valid_postcode($pc));
                    # Check with MaPit.
                    my $res = undef;
                    try {
                        my $x = mySociety::MaPit::get_location($pc);
                        ($lat, $lon) = ($x->{wgs84_lat}, $x->{wgs84_lon});
                    } catch RABX::Error with {
                        my $E = shift;
                        $res = $err;
                    };
                    return $res;
                }
            ], ['Type of item',
                'category', 'select',
                dbh()->selectall_arrayref('select id, name from category order by name')
            ], ['Description',
                'description', 'longtext',
                qr/./, 'Please describe the item you are giving away'
            ], ['Donate item',
                'donate', 'button'
            ]
        ]);

    $form->populate($q);
    if ($form->is_valid()) {
        # XXX test for multiple-insertion.
        # Stuff the results into the database.
        my $desc = $q->param('description');

        # Construct a suitable link.
        my $token = GIA::token($q->param('itemid'));
        my $link = mySociety::Config::get('BASE_URL') . "/Confirm?t=$token";
        
        mySociety::EvEl::send({
                _body_ => <<EOF,

Thank you for agreeing to donate an item to a local charity through GiveItAway.
Here's what you've agreed to give:

$desc

Before we can pass on the details of this item, we need you to confirm your
email address. Please click on this link:

$link

EOF
                To => [[$q->param('email'), $q->param('name')]],
                From => 'noreply@giveitaway.com',
                Subject => 'Confirm your GiveItAway donation'
            }, $q->param('email'));
        
        dbh()->do('insert into location (id, postcode, lat, lon) values (?, ?, ?, ?)', {}, $q->param('locid'), $q->param('postcode'), $lat, $lon);
        dbh()->do('insert into item (id, email, name, category_id, description, location_id) values (?, ?, ?, ?, ?, ?)', {}, map { $q->param($_) } qw(itemid email name category description locid));
        dbh()->commit();
        $$hdr = $q->header();
        $$content =
            $q->start_html('Thank You!')
                . $q->p("Now check your email!")
                . $q->p("We have sent you an email to confirm your email address. Before your item is passed on to local charities, we need you to read your email and click the link we've sent you.")
                . $q->end_html();
    } else {
        $$hdr = $q->header();
        $$content =
            $q->start_html('Donate Item')
                . $q->p("Got something you'd like to give away to a good cause? Enter your details and we'll copy you into an email to a nearby charity that might be interested.")
                . $q->start_form(-method => 'POST')
                . $form->render($q)
                . $q->end_form()
                . $q->end_html();
    }

    return 0;

}

1;

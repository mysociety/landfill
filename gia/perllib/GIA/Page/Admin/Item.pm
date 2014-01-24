#!/usr/bin/perl
#
# GIA/Page/Admin/Item.pm:
# Detail of an individual item.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: Item.pm,v 1.3 2007-08-02 11:45:07 matthew Exp $
#

package GIA::Page::Admin::Item;

use strict;

use Error qw(:try);
use HTML::Entities;
use POSIX qw(strftime);

use mySociety::DBHandle qw(dbh);
use mySociety::MaPit;
use mySociety::EmailUtil qw(is_valid_email);
use mySociety::PostcodeUtil qw(is_valid_postcode);

use GIA;
use GIA::Form;
use GIA::Web;

sub render ($$$) {
    my ($q, $hdr, $content) = @_;

    our ($qp_itemid);
    $q->Import('p',
            itemid => [qr/^[1-9]\d*$/, undef]
        );
    if (!defined($qp_itemid)
        || !defined(dbh()->selectrow_array('select id from item where id = ?', {}, $qp_itemid))) {
        $$hdr = $q->redirect('Items');
        $$content = '';
        return 0;
    }

    my $itemid = $qp_itemid;
    my $locid = dbh()->selectrow_array('select location_id from item where id = ?', {}, $itemid);
    
    $$hdr = $q->header();
    $$content =
        $q->start_html("Detail of item $itemid");

    # First show a form which allows the user to edit the item. Then show the
    # item's history and actions which can be done with it.
    my ($lat, $lon);

    my $f = new GIA::Form([
            ['', 'itemid', 'hidden'],
            ['Donor email address',
                'email', 'text',
                sub ($) {
                    return is_valid_email($_[0]) ? undef :
                        'Please enter a full, valid email address'
                }
            ], ['Donor name',
                'name', 'text',
                qr/./, "Please enter the donor's name"
            ], ['Postcode',
                'postcode', 'text',
                sub ($) {
                    my $pc = shift;
                    my $err = 'Please give a full postcode';
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
                qr/./, 'Please give a description'
            ], ['Update item',
                'update', 'button'
            ]
        ]);

    # Populate the form with information about this item.

    if (!$q->param('update')) {
        # Populate item form from database.
        my $h = dbh()->selectrow_hashref('
                    select item.id as itemid, location_id as locid, email,
                        name, postcode, category_id as category, description
                    from item, location
                    where item.location_id = location.id
                        and item.id = ?', {}, $itemid);
        foreach (keys %$h) {
            $q->param($_, $h->{$_});
        }
    }

    $f->populate($q);

    if ($f->is_valid()) {
        # Update database from form.
        dbh()->do('
                update item set email = ?, name = ?, category_id = ?,
                    description = ?
                where id = ?',
                {},
                (map { $q->param($_) } qw(email name category description)),
                    $itemid);
        dbh()->do('
                update location set postcode = ?, lat = ?, lon = ?
                where id = ?',
                {},
                $q->param('postcode'), $lat, $lon, $locid);
        dbh()->commit();

        # Redirect back.
        $$hdr = $q->redirect("Item?itemid=$itemid");
        $$content = '';
        return 0;
    }

    $$content .=
        $q->h2('Item details')
        . $q->start_form(-method => 'POST')
            . $f->render($q)
        . $q->end_form()
        . $q->h2('Item history')
        . $q->start_table()
            . $q->Tr($q->th(['When', 'What']))
            . $q->Tr($q->td([
                    strftime('%Y-%m-%d&nbsp;%H:%M:%S', localtime(dbh()->selectrow_array('select epoch(whenadded) from item where id = ?', {}, $itemid))),
                    "item added to database"
                ]));

    my $whenconfirmed = dbh()->selectrow_array('select epoch(whenconfirmed) from item where id = ?', {}, $itemid);
    if (defined($whenconfirmed)) {
        $$content .= $q->Tr($q->td([
                        strftime('%Y-%m-%d&nbsp;%H:%M:%S', localtime($whenconfirmed)),
                        "donor email address confirmed"
                    ]));
    }

    my $st = dbh()->prepare('
                    select acceptor.id, name, email, organisation,
                        epoch(whensent), epoch(whenaccepted)
                    from acceptor_item_interest, acceptor
                    where acceptor_item_interest.acceptor_id = acceptor.id
                        and item_id = ?
                    order by whensent, whenaccepted');
    $st->execute($itemid);

    while (my ($acceptorid, $name, $email, $organisation, $whensent,
                $whenaccepted) = $st->fetchrow_array()) {
        $$content .= $q->Tr($q->td([
                        strftime('%Y-%m-%d&nbsp;%H:%M:%S', localtime($whensent)),
                        sprintf('offered to <a href="Acceptor?acceptorid=%d">%s &lt;%s&gt; (%s)</a>',
                                $acceptorid, encode_entities($name), encode_entities($email),
                                encode_entities($organisation))
                    ]));
                    
        $$content .= $q->Tr($q->td([
                        strftime('%Y-%m-%d&nbsp;%H:%M:%S', localtime($whenaccepted)),
                        sprintf('accepted by <a href="Acceptor?acceptorid=%d">%s &lt;%s&gt; (%s)</a>',
                                $acceptorid, encode_entities($name), encode_entities($email),
                                encode_entities($organisation))
                    ]))
            if (defined($whenaccepted));
    }


    $$content .=
        $q->end_table()
        . $q->h2('Actions');

    $f = new GIA::Form([
            ['', 'itemid', 'hidden'],
            ['Delete item permanently', 'delete', 'button'],
            (!defined(dbh()->selectrow_array('select whenconfirmed from item where id = ?', {}, $itemid))
            ? (["Confirm donor's email address", 'confirm', 'button'])
            : ())
        ]);

    $f->populate($q);
    if ($f->is_valid()) {
        if ($q->param('delete')) {
            dbh()->do('delete from item where id = ?', {}, $itemid);
            dbh()->do('delete from location where id = ?', {}, $locid);
            dbh()->commit();
            $$hdr = $q->redirect('Items');
            $$content = '';
            return 0;
        } elsif ($q->param('confirm')) {
            dbh()->do('update item set whenconfirmed = current_timestamp where id = ? and whenconfirmed is null', {}, $itemid);
            dbh()->commit();
            $$hdr = $q->redirect("Item?itemid=$itemid");
            $$content = '';
            return 0;
        }
    }

    $$content .=
        $q->start_form(-method => 'POST')
            . $f->render($q)
        . $q->end_form()
        . $q->end_html();

    return 0;
}

1;

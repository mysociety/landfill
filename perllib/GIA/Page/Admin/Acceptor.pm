#!/usr/bin/perl
#
# GIA/Page/Admin/Acceptor.pm:
# Add/edit acceptor.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: Acceptor.pm,v 1.1 2005-10-19 16:15:43 chris Exp $
#

package GIA::Page::Admin::Acceptor;

use strict;

use Error qw(:try);
use HTML::Entities;
use POSIX qw(strftime);

use mySociety::DBHandle qw(dbh);
use mySociety::MaPit;
use mySociety::Util qw(is_valid_email is_valid_postcode);

use GIA;
use GIA::Form;
use GIA::Web;

sub render ($$$) {
    my ($q, $hdr, $content) = @_;

    our ($qp_acceptorid, $qp_locid, $qp_new);
    $q->Import('p',
            acceptorid => [qr/^[1-9]\d*$/, undef],
            locid => [qr/^[1-9]\d*$/, undef],
            new => [qr/./, 0]
        );
    if (!$qp_new && (!defined($qp_acceptorid)
            || !defined(dbh()->selectrow_array('select id from acceptor where id = ?', {}, $qp_acceptorid)))) {
        $$hdr = $q->redirect('Acceptors');
        $$content = '';
        return 0;
    }

    if ($qp_new) {
        $q->param('acceptorid', dbh()->selectrow_array("select nextval('acceptor_id_seq')"))
            if (!defined($qp_acceptorid));
        $q->param('locid', dbh()->selectrow_array("select nextval('location_id_seq')"))
            if (!defined($qp_locid));
    }

    my ($lat, $lon);
    my $f = new GIA::Form([
            ['', 'new', 'hidden'],
            ['', 'acceptorid', 'hidden'],
            ['', 'locid', 'hidden'],
            ['Contact email address',
                'email', 'text',
                sub ($) {
                    return is_valid_email($_[0]) ? undef :
                        'Please enter a full, valid email address'
                }
            ], ['Contact name',
                'name', 'text',
                qr/./, "Please enter the contact's name"
            ], ['Organisation',
                'organisation', 'text',
                qr/./, "Please enter the accepting organisation"
            ], ['Charity number',
                'charity', 'text',
                qr/^(|[1-9]\d*)$/, "Please give the organisation's registered charity number, or leave blank if not a charity"
            ], ['Postcode',
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
            ], [$qp_new ? 'Add new acceptor' : 'Update acceptor',
                'update', 'button'
            ]
        ]);

    if (!$q->param('update') && !$qp_new) {
        # Populate form from database.
        my $h = dbh()->selectrow_hashref('
                    select acceptor.id as acceptorid, location_id as locid,
                        email, name, organisation, charity_number as charity,
                        postcode
                    from acceptor, location
                    where acceptor.location_id = location.id
                        and acceptor.id = ?', {}, $qp_acceptorid);
        foreach (keys %$h) {
            $q->param($_, $h->{$_});
        }
    }

    $f->populate($q);

    if ($f->is_valid()) {
        if ($qp_new) {
            dbh()->do('
                    insert into location (id, postcode, lat, lon)
                    values (?, ?, ?, ?)',
                    {}, (map { $q->param($_) } qw(locid postcode)), $lat, $lon);
            dbh()->do('
                    insert into acceptor (id, email, name, organisation,
                        location_id, charity_number)
                    values (?, ?, ?, ?, ?, ?)',
                    {}, (map { $q->param($_) } qw(acceptorid email name
                        organisation locid)),
                    $q->param('charity') || undef);
        } else {
            dbh()->do('
                    update acceptor set email = ?, name = ?, organisation = ?,
                        charity_number = ?
                    where id = ?',
                    {},
                    (map { $q->param($_) } qw(email name organisation)),
                    $q->param('charity') || undef,
                    $q->param('acceptorid'));
            dbh()->do('
                    update location set postcode = ?, lat = ?, lon = ?
                    where id = ?',
                    {},
                    $q->param('postcode'), $lat, $lon, $q->param('locid'));
        }
        dbh()->commit();
        $$hdr = $q->redirect("Acceptor?acceptorid=$qp_acceptorid");
        $$content = '';
        return 0;
    }

    $$hdr = $q->header();
    if ($qp_new) {
        $$content =
            $q->start_html('Add new acceptor')
            . $q->start_form(-method => 'POST')
                . $f->render($q)
            . $q->end_form()
            . $q->end_html();
        return 0;
    }

    $$content =
        $q->start_html("Acceptor $qp_acceptorid")
        . $q->start_form(-method => 'POST')
            . $f->render($q)
        . $q->end_form()
        . $q->end_html();

    return 0;
}

1;

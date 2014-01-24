#!/usr/bin/perl
#
# GIA/Page/Admin/Acceptor.pm:
# Add/edit acceptor.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: Acceptor.pm,v 1.4 2007-08-02 11:45:07 matthew Exp $
#

package GIA::Page::Admin::Acceptor;

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
            ], ['Interested in categories',
                'category',  'multiselect',
                dbh()->selectall_arrayref('select id, name from category order by name')
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

        # Categories.
        $q->param(-name => 'category', -values => dbh()->selectcol_arrayref('select category_id from acceptor_category_interest where acceptor_id = ?', {}, $qp_acceptorid));
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

        dbh()->do('delete from acceptor_category_interest where acceptor_id = ?', {}, $q->param('acceptorid'));
        foreach ($q->param('category')) {
            dbh()->do('insert into acceptor_category_interest (acceptor_id, category_id) values (?, ?)', {},  $q->param('acceptorid'), $_);
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
        . $q->h2('Acceptor details')
        . $q->start_form(-method => 'POST')
            . $f->render($q)
        . $q->end_form()
        . $q->h2('Items offered')
        . $q->start_table()
            . $q->Tr($q->th([
                    '',
                    'Item',
                    'When offered',
                    'Fate'
                ]));

    # Show items offered, accepted and declined.
    my $st = dbh()->prepare('select item_id, epoch(whensent), epoch(whenaccepted), epoch(whendeclined), description from acceptor_item_interest, item where item_id = item.id and acceptor_id = ? order by whensent desc');
    $st->execute($qp_acceptorid);

    my $n = 0;
    while (my ($itemid, $whensent, $whenaccepted, $whendeclined, $desc) = $st->fetchrow_array()) {
        my $fate = 'awaiting response';
        if (defined($whenaccepted)) {
            $fate = strftime('accepted %Y-%m-%d&nbsp;%H:%M:%S', localtime($whenaccepted));
        } elsif (defined($whendeclined)) {
            $fate = strftime('declined %Y-%m-%d&nbsp;%H:%M:%S', localtime($whendeclined));
        } else {
            # See if it's been accepted by anyone else.
            $fate = 'accepted by someone else'
                if (defined(dbh()->selectrow_array('select acceptor_id from acceptor_item_interest where item_id = ? and whenaccepted is not null', {}, $itemid)));
            
        }
    
        $$content .= $q->Tr($q->td([
                            $q->a({ -href => "Item?itemid=$itemid" }, $itemid),

                            $q->FormatText($desc),

                            strftime('%Y-%m-%d&nbsp;%H:%M:%S', localtime($whensent)),

                            $fate
                        ]));
        ++$n;
    }

    $$content .= $q->Tr($q->td({ -colspan => 4 }, $q->em('No items offered yet')))
        if (!$n);

    $$content .=
        $q->end_table()
        . $q->h2('Actions');

    $f = new GIA::Form([
            ['', 'acceptorid', 'hidden'],
            ['Delete acceptor permanently', 'delete', 'button']
        ]);

    $q->param('new', '');
    $f->populate($q);
    if ($f->is_valid()) {
        if ($q->param('delete')) {
            my $locid = dbh()->selectrow_array('select location_id from acceptor where id = ?', {}, $qp_acceptorid);
            dbh()->do('delete from acceptor_category_interest where acceptor_id = ?', {}, $qp_acceptorid);
            dbh()->do('delete from acceptor_item_interest where acceptor_id = ?', {}, $qp_acceptorid);
            dbh()->do('delete from acceptor where id = ?', {}, $qp_acceptorid);
            dbh()->do('delete from location where id = ?', {}, $locid);
            dbh()->commit();
            $$hdr = $q->redirect('Acceptors');
            $$content = '';
            return 0;
        }
    }
    
    $$content .=
        $q->start_form(-method => 'POST')
            . $f->render($q, 1)
        . $q->end_form()
        . $q->end_html();

    return 0;
}

1;

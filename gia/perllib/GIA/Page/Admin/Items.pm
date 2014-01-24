#!/usr/bin/perl
#
# GIA/Page/Admin/Items.pm:
# Items submitted.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: Items.pm,v 1.1 2005-10-19 14:11:35 chris Exp $
#

package GIA::Page::Admin::Items;

use strict;

use HTML::Entities;
use POSIX qw(strftime);

use GIA;
use GIA::Web;

use mySociety::DBHandle qw(dbh);

use constant PAGELENGTH => 50;

sub render ($$$) {
    my ($q, $hdr, $content) = @_;

    $$hdr = $q->header();
    $$content = $q->start_html("Items available");
    
    our ($qp_query, $qp_pagenum);
    $q->Import('p',
            query => [qr/./, ''],
            pagenum => [qr/^(0|[1-9]\d*)$/, 0]
        );

    my $limit = PAGELENGTH;
    my $offset = $qp_pagenum * PAGELENGTH;
    my $st = dbh()->prepare("
                    select item.id, email, item.name, description,
                        epoch(whenadded), epoch(whenconfirmed), postcode,
                        lat, lon, category.name
                    from item, category, location
                    where item.location_id = location.id
                        and item.category_id = category.id
                    order by whenadded desc
                    limit $limit offset $offset");
    $st->execute();

    $$content .=
        $q->start_table()
        . $q->Tr($q->th([
                '',
                'Item',
                'Donor',
                'Location',
                'When'
            ]));
    while (my ($id, $email, $name, $desc, $added, $confirmed,
                $postcode, $lat, $lon, $cat) = $st->fetchrow_array()) {
        my $state = 'new';
        if (defined($confirmed)) {
            my $accepted = dbh()->selectrow_array('select epoch(whenaccepted) from acceptor_item_interest where item_id = ? and whenaccepted is not null', {}, $id);
            if ($accepted) {
                $state = "accepted";
            } else {
                my $offers = dbh()->selectrow_array('select count(acceptor_id) from acceptor_item_interest where item_id = ?', {}, $id);
                if ($offers > 0) {
                    $state = "offered ($offers)";
                } else {
                    $state = "confirmed";
                }
            }
        }
        $$content .=
            $q->Tr($q->td([
                    $q->a({ -href => "Item?itemid=$id" }, $id),

                    $q->p($q->strong("Category: "), encode_entities($cat),
                            $q->br(), $q->strong("State: "), $state)
                        . $q->FormatText($desc),

                    $q->a({ -href => "mailto:$email" }, encode_entities($name),
                            "&lt;" . encode_entities($email) . "&gt;"),
                            
                    encode_entities($postcode),
                    
                    strftime('%Y-%m-%d&nbsp;%H:%M:%S', localtime($added || $confirmed))
                ]));
    }

    $$content .=
            $q->end_table()
            . $q->end_html();
    return 0;
}

1;

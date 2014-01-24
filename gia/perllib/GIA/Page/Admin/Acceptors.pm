#!/usr/bin/perl
#
# GIA/Page/Admin/Acceptors.pm:
# List of people to whom goods may be sent.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: Acceptors.pm,v 1.1 2005-10-19 16:15:43 chris Exp $
#

package GIA::Page::Admin::Acceptors;

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
    $$content =
        $q->start_html("Registered acceptors")
        . $q->p($q->a({ -href => 'Acceptor?new=1' }, "Add new acceptor"));

    our ($qp_query, $qp_pagenum);
    $q->Import('p',
            query => [qr/./, ''],
            pagenum => [qr/^(0|[1-9]\d*)$/, 0]
        );

    my $limit = PAGELENGTH;
    my $offset = $qp_pagenum * PAGELENGTH;
    my $st = dbh()->prepare("
                    select acceptor.id, email, name, organisation,
                        charity_number, epoch(whenadded), postcode, lat, lon
                    from acceptor, location
                    where acceptor.location_id = location.id
                    order by organisation
                    limit $limit offset $offset");
    $st->execute();

    $$content .=
        $q->start_table()
        . $q->Tr($q->th([
                '',
                'Acceptor',
                'Location',
                'When added'
            ]));
    while (my ($id, $email, $name, $organisation, $charity, $when, $postcode,
                $lat, $lon) = $st->fetchrow_array()) {
        $$content .= $q->Tr($q->td([
                $q->a({ -href => "Acceptor?acceptorid=$id" }, $id),

                encode_entities(sprintf('%s <%s> (%s)', $name, $email, $organisation))
                    . (defined($charity)
                        ? " Charity #" .  $q->a({ -href => "http://www.charity-commission.gov.uk/registeredcharities/showcharity.asp?chyno=$charity" }, $charity)
                        : ''),

                $postcode,

                strftime('%Y-%m-%d&nbsp;%H:%M:%S', localtime($when))
            ]));
    }

    $$content .=
        $q->end_table()
        . $q->end_html();

    return 0;
}

1;

#!/usr/bin/perl
#
# GIA/Page/NewItem.pm:
# Create a new item.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: NewItem.pm,v 1.2 2005-10-18 17:40:43 chris Exp $
#

package GIA::Page::NewItem;

use strict;

use Error qw(:try);
use Digest::SHA1 qw(sha1_hex);

use mySociety::DBHandle qw(dbh);
use mySociety::EvEl;
use mySociety::MaPit;
use mySociety::Util qw(is_valid_email is_valid_postcode);

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
            ['', '_page', 'hidden'],
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
        # Stuff the results into the database.
        my $desc = $q->param('description');

        # Construct a suitable link.
        my $random = sprintf("%04x", int(rand(0xffff)));
        my $token = $q->param('itemid') . ",$random," . substr(sha1_hex(GIA::DB::secret() . ",$random," . $q->param('itemid')), 0, 8);
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
                To => [$q->param('email'), $q->param('name')],
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
                . $q->start_form(-method => 'POST')
                . $form->render($q)
                . $q->end_html();
    }

    return 0;

}

1;

__END__

sub render ($$$) {
    my ($q, $hdr, $content) = @_;

    our ($qp_name, $qp_email, $qp_postcode, $qp_category, $qp_description, $qp_edited);
    $q->Import('p',
                name        => [qr/[A-Z]+/i, undef],
                email       => [sub ($$) { my ($q, $email) = @_; return is_valid_email($email); }, undef],
                postcode    => [sub ($$) { my ($q, $pc) = @_; return is_valid_postcode($pc); }, undef],
                category    => [sub ($$) { my ($q, $cat) = @_; return defined(dbh()->selectrow_array('select id from category where id = ?', {}, $cat)); }, undef],
                description => [qr/[A-Z]+/i, undef],
                edited      => [qr/^1$/, 0]);

    if ($qp_edited) {
        $$hdr = $q->header();
        $$content = $q->start_html("edited!") . $q->end_html();
    }

    $$hdr = $q->header();

    $q->param('edited', 1);
    $$content =
        $q->start_html('Give Something Away!')
        . $q->p('Please fill out the form below with details of the item you would like to give away:')
        . $q->start_form(-method => 'POST')
. '<input type="hidden" name="_page" value="NewItem" />'
            . $q->hidden(-name => 'edited')
            . $q->label({ -for => 'name' }, 'Your name ')
                . $q->textfield(-name => 'name', -id => 'name')
                . $q->br();
    $$content .= 'Please enter your name' . $q->br()
                    if ($qp_edited && !$qp_name);
    $$content .=
            $q->label({ -for => 'email' }, 'Your email address ')
                . $q->textfield(-name => 'email', -id => 'email')
                . $q->br();
    $$content .= 'Please enter a full, valid email address' . $q->br()
                    if ($qp_edited && !$qp_email);
    $$content .= $q->label({ -for => 'postcode' }, 'Your postcode ')
                . $q->textfield(-name => 'postcode', -id => 'postcode')
                . '(so that we can find nearby charities)'
                . $q->br();
    $$content .= 'Please enter your full postcode' . $q->br()
                    if ($qp_edited && !$qp_postcode);
    $$content .=
                $q->label({ -for => 'category'}, 'What sort of item? ')
                . $q->popup_menu(
                        -name => 'category',
                        -values => ['-1', @{dbh()->selectcol_arrayref('select id from category order by name')}],
                        -labels => {
                            -1 => '(select one)',
                            map { $_->[0] => $_->[1] } @{dbh()->selectall_arrayref('select id, name from category')}
                        }
                    )
                . $q->br();
    $$content .= 'Please select a description' . $q->br()
                    if ($qp_edited && !$qp_category);
    $$content .=
            $q->label({ -for => 'description'}, 'Description of the item')
                . $q->br()
                . $q->textarea(-name => 'description', -id => 'description', -columns => 40, -rows => 5)
                . $q->br();
    $$content .= 'Please give a brief description of the item' . $q->br()
                    if ($qp_edited && !$qp_description);
    $$content .=
            $q->submit(-label => "Donate item")
        . $q->end_form()
        . $q->end_html();

    return 0;
}

1;

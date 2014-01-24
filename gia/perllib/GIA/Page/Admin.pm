#!/usr/bin/perl
#
# GIA/Page/Admin.pm:
# Admin index page.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: Admin.pm,v 1.1 2005-10-19 14:11:35 chris Exp $
#

package GIA::Page::Admin;

use strict;

use GIA::Web;

sub render ($$$) {
    my ($q, $hdr, $content) = @_;
    $$hdr = $q->header();
    $$content =
            $q->start_html("Admin pages")
            . $q->end_html();
    return 0;
}

1;

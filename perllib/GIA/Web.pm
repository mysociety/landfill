#!/usr/bin/perl
#
# GIA/Web.pm:
# CGI-like class which we can extend.
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: Web.pm,v 1.1 2005-10-14 18:18:38 chris Exp $
#

package GIA::Web;

use strict;

use GIA;

use CGI qw(-nosticky);
my $have_cgi_fast = 0;
eval {
    use CGI::Fast;
    $have_cgi_fast = 1;
};

use fields qw(q);

=item new [QUERY]

Construct a new GIA::Web object, optionally from an existing QUERY. Uses
CGI::Fast if available, or CGI otherwise.

=cut
sub new ($;$) {
    my ($class, $q) = @_;
    if (!defined($q)) {
        $q = $have_cgi_fast ? new CGI::Fast() ; new CGI();
    }
    my $self = fields::new('GIA::Web');
    $self->{q} = $q;
    return $self;
}

# q
# Access to the underlying CGI (or whatever) object.
sub q ($) {
    return $_[0]->{q};
}

# AUTOLOAD
# Is-a inheritance isn't safe for this kind of thing, so we use has-a.
sub AUTOLOAD {
    my $f = $GIA::Web::AUTOLOAD;
    $f =~ s/^.*:://;
    eval "sub $f { return \$_[0]->{q}->$f(\@_); }";
}

1;

#!/usr/bin/perl
#
# GIA/Form.pm:
# Web forms.
#
# Copyright (c) 2005 Chris Lightfoot. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: Form.pm,v 1.5 2007-08-02 11:45:07 matthew Exp $
#

package GIA::Form;

use strict;

use Digest::SHA1;

use mySociety::Random qw(random_bytes);

use GIA;

# scrub STRING [MULTILINE]
# "Scrub" an input STRING, replacing undef with '', removing whitespace at
# start and end of string, replacing repeated spaces with single spaces, and,
# unless multiline is true, replacing vertical whitespace with horizontal.
sub scrub ($;$) {
    my ($x, $multiline) = @_;
    return '' if (!defined($x));
    $x =~ s/\r?\n/ /gs unless ($multiline);
    $x =~ s/^\s+//;
    $x =~ s/\s+$//;
    $x =~ s/\s+/ /gs;
    return $x;
}

sub ent ($) {
    my $x = shift;
    my %s = qw(< &lt; & &amp; > &gt; " &quot;);
    $x =~ s#([<&">])#$s{$1}#ge;
    return $x;
}

=item new ELEMENTS [CHECK ...]

Construct a new form object. ELEMENTS is a reference to a list describing each
element of the form; each element in the list is a reference to a list of the
following items:

=over 4

=item description

=item field name

=item field type

One of: hidden, for a state-preserving hidden field; text, for a text field;
longtext, for a textarea; select, for a drop-down list; multiselect, for a
multiple-selection list; button, for a clickable button; or html for some
literal HTML to include in the output form.

=back

The final items give information about the field. None is required for a hidden
field; for a text field, supply a regular expression and an error to show if
the entry does not match it; or a code reference, which should return an error
message or undef if the field value is valid. For a select field, they should
be followed by a list of the values to show for the field and descriptions, as
a list of references-to-lists.  For a button, there is no extra data and the
description is used as the label to appear on the button.

Each additional CHECK is 

=cut
sub new ($@) {
    my ($class, $element, @extrachecks) = @_;
    
    my %havename;
    my $i = 1;
    foreach (@$element) {
        my ($description, $name, $type) = @$_;
        die "name may not contain spaces in element #$i" if ($name =~ / /);
        die "name may not begin '__GIA_Form' in element #$i" if ($name =~ m#^__GIA_Form#);
        die "description not defined in element #$i" if (!defined($description));
        die "description must be text, not reference in element #$i" if (ref($description));
        die "name not defined in element #$i" if (!defined($name));
        die "name must be text, not reference in element #$i" if (ref($name));
        die "repeated name '$name' in element #$i" if ($type ne 'html' && exists($havename{$name}));
        $havename{$name} = 1;
        die "type not defined in element #$i" if (!defined($type));
        die "bad element type '$type' in element #$i" unless ($type =~ m#^(hidden|(long|)text|(multi|)select|button|html)$#);
        
        die "hidden field followed by extra data in element #$i" if ($type eq 'hidden' and @$_ > 3);

        if ($type eq 'text' || $type eq 'longtext') {
            die "$type field followed by extra data in element #$i" if (@$_ > 5);
            my ($check, $error) = @{$_}[3 .. 4];
            die "no check for $type field in element #$i" if (!defined($check));
            if (ref($check) eq 'CODE') {
                die "code ref check for text field should not be followed by error message in element #$i"
                    if (defined($error));
            } elsif (ref($check) eq 'Regexp') {
                die "regexp check for text field should be followed by error message in element #$i"
                    if (!defined($error));
                die "error message for text field must be text, not reference in element #$i"
                    if (ref($error));
            } else {
                die "check for text field should be code ref or regular expression in element #$i";
            }
        } elsif ($type eq 'select' || $type eq 'multiselect') {
            die "$type field followed by extra data in element #$i" if (@$_ > 4);
            my $values = $_->[3];
            die "values supplied for $type field should be reference to list in element #$i"
                unless (ref($values) eq 'ARRAY');
            foreach (@$values) {
                die "each possible value for $type field should be reference to list of key and optional description in element #$i"
                    if (ref($_) ne 'ARRAY' || @$_ < 1 || @$_ > 2);
                die "value in $type item must not be a reference in element #$i"
                    if (ref($_->[0]));
                die "description in $type item must not be a reference in element #$i"
                    if (@$_ > 1 && ref($_->[2]) ne '');
            }
        } elsif ($type eq 'button') {
            die "button field followed by extra data in element #$i" if (@$_ > 3);
        } elsif ($type eq 'html') {
            die "html field followed by extra data in element #$i" if (@$_ > 3);
        }
        ++$i;
    }

    my $self = {
            element => $element   # copy?
        };

    die "any extra checks must be code refs"
        if (grep { ref($_) ne 'CODE' } @extrachecks);

    $self->{extrachecks} = \@extrachecks;

    return bless($self, $class);
}

=item populate QUERY

Populate the form with values from QUERY. Also updates the values in QUERY to
reflect their "canonical" values in the form.

=cut
sub populate ($$) {
    my ($self, $q) = @_;

    die "QUERY should be a GIA::Web object or reference-to-hash" unless (ref($q) eq 'HASH' || UNIVERSAL::isa($q, 'GIA::Web'));

    # Read and check each field.
    $self->{fields} ||= { };
    $self->{errors} = { };
    foreach (@{$self->{element}}) {
        my ($description, $name, $type) = @$_;
        next unless ($name);
        my $value = (ref($q) eq 'HASH' ? $q->{$name} : $q->param($name));
        if ($type eq 'text' || $type eq 'longtext') {
            $value = scrub($value);
            ref($q) eq 'HASH' or $q->param($name, $value);
            my $check = $_->[3];
            my $e;
            if (ref($check) eq 'CODE') {
                $e = &$check($value);
            } elsif ($value !~ $check) {
                $e = $_->[4];
            }
            $self->{errors}->{$name} = $e if ($e);
        } elsif ($type eq 'select') {
            $self->{errors}->{$name} = 'Please select a value'
                if (!defined($value) || !grep { $_->[0] eq $value } @{$_->[3]});
        } elsif ($type eq 'multiselect') {
            my %allowed = map { $_ => 1 } @{$_->[3]};
            my @values = grep { exists($allowed{$_}) } $q->param($name);
            $q->param(-name => $name, -values => \@values);
        } elsif ($type eq 'button') {
            $value = defined($value) ? 1 : 0;
            #$q->param($name) ? 1 : 0;
        }
        $self->{fields}->{$name} = $value;
    }

    # Perform any extra checks.
    $self->{extraerrors} = [ ];
    foreach (@{$self->{extrachecks}}) {
        push(@{$self->{extraerrors}}, &$_($self));
    }

    ref($q) eq 'HASH' and return;

    # Verify that any hidden form fields have their state preserved.
    my $F = $q->param('__GIA_Form_hidden_fields');
    my @hiddenfields;
    if ($F && $F =~ m#^([0-9a-f]+):([A-Za-z0-9+/=]+):(.+)#) {
        my ($salt, $oldhash) = ($1, $2);
        @hiddenfields = split(/ /, $3);
        my $g = new Digest::SHA1();
        $g->add(@hiddenfields, $salt, GIA::DB::secret());
        if ($g->b64digest() eq $oldhash) {
            my $H = $q->param('__GIA_Form_hidden_fields_hash');
            if ($H && $H =~ m#^([0-9a-f]+):([A-Za-z0-9+/=]+)#)  {
                my ($salt2, $oldhash2) = ($1, $2);
                my $h = new Digest::SHA1();
                my $missing = 0;
                foreach (@hiddenfields) {
                    if (!defined($q->param($_))) {
                        ++$missing;
                        last;
                    }
                    $h->add($q->param($_));
                }
                if ($missing) {
                    push(@{$self->{extraerrors}}, 'A hidden field was missing; please start again or contact us for help');
                } else {
                    $h->add($salt2, GIA::DB::secret());
                    push(@{$self->{extraerrors}}, 'A hidden field has been changed; please start again or contact us for help')
                        if ($oldhash2 ne $h->b64digest());
                }
            } else {
                push(@{$self->{extraerrors}}, 'A required form parameter was missing; please start again or contact us for help');
            }

        } else {
            push(@{$self->{extraerrors}}, 'A hidden field has been changed; please start again or contact us for help')
        }
    } else {
        push(@{$self->{extraerrors}}, 'A necessary form parameter was missing or invalid; please start again or contact us for help');
    }

    # Set first-time flag.
    $self->{firsttime} = defined($q->param('__GIA_Form_submitted')) ? 0 : 1;
}

=item is_valid

Return true if the form has been completed without error and one of its buttons
pushed.

=cut
sub is_valid ($) {
    my ($self) = @_;
    die "is_valid called without populating form"
        if (!exists($self->{fields}));
    return 0 if (scalar(keys(%{$self->{errors}})) > 0);
    # The form is valid only if a button has been pushed.
    return 0 unless (1 == scalar(grep { $_->[2] eq 'button' && $self->{fields}->{$_->[1]} } @{$self->{element}}));
    # A form must be filled in before it is submitted!
    return !$self->{firsttime};
}

=item all_fields_valid

Return true if all of the form's fields are valid (even if no button has been
pushed).

=cut
sub all_fields_valid ($) {
    my ($self) = @_;
    return 0 if (scalar(keys(%{$self->{errors}})) > 0);
    return 1;
}

=item render QUERY [NOERRORS]

Return HTML expressing this form, and any errors in its currently-entered
values. If NOERRORS is true, don't print any errors.

=cut
sub render ($$;$) {
    my ($self, $q, $noerrors) = @_;
    if (!defined($noerrors)) {
        $noerrors = $self->{firsttime};
    }
    $noerrors ||= 0;
    $self->{errors} ||= { };
    my $ee = '';
    if (!$noerrors && @{$self->{extraerrors}}) {
        $ee = $q->ul(
                    map {
                        $q->li({ -class => 'giaform_error' }, ent($_))
                    } @{$self->{extraerrors}}
                );
    }
    my $html = $ee;

    # Do hidden elements first, since otherwise they would have to live in
    # table rows to be valid HTML. Hash all of these elements so that we can
    # verify that they are correct at the end. Also record the elements which
    # we've hashed, since those are the only ones which should be checked on
    # return (the caller might add new hidden fields from a trusted source,
    # and we shouldn't barf on those).
    my $g = new Digest::SHA1();
    my $h = new Digest::SHA1();
    my @hh = ( );
    foreach (grep { $_->[2] eq 'hidden' && $_->[1] !~ m#^__GIA_Form_# }
                @{$self->{element}}) {
        my ($description, $name, $type) = @$_;
        $html .= $q->hidden(-name => $name);
        push(@hh, $name);
        $g->add($name);
        $h->add($q->param($name));
    }
    my $salt = unpack('h*', random_bytes(4));
    $g->add($salt, GIA::DB::secret());
    $h->add($salt, GIA::DB::secret());
    my $f = join(" ", @hh);
    $q->param('__GIA_Form_hidden_fields', "$salt:" . $g->b64digest() . ":$f");
    $html .= $q->hidden(-name => '__GIA_Form_hidden_fields');
    $q->param('__GIA_Form_hidden_fields_hash', "$salt:" . $h->b64digest());
    $html .= $q->hidden(-name => '__GIA_Form_hidden_fields_hash');
    $q->param('__GIA_Form_submitted', 1);
    $html .= $q->hidden(-name => '__GIA_Form_submitted');
    
    $html .= $q->start_table({ -class => 'giaform' });
    foreach (grep { $_->[2] ne 'hidden' } @{$self->{element}}) {
         my ($description, $name, $type) = @$_;
        if ($type eq 'html') {
            $html .= $q->div($description);
        } elsif ($type eq 'text') {
            $html .= "<label for='$name'>" . ent($description) . '</label>' .
                            $q->textfield(
                                -name => $name,
                                -size => 25
                            );
        } elsif ($type eq 'longtext') {
            $html .= "<label for='$name'>" . ent($description) . '</label>' .
                            $q->textarea(
                                -name => $name,
                                -columns => 25,
                                -rows => 5
                            );
        } elsif ($type eq 'select') {
            $html .= "<label for='$name'>" . ent($description) . '</label>' .
                        $q->popup_menu(
                            -name => $name,
                            -values => [ '', map { $_->[0] } @{$_->[3]} ],
                            -labels => {
                                '' => '(select one)',
                                map { $_->[0] => $_->[1] }
                                grep { @$_ == 2 } @{$_->[3]}
                            }
                        );
        } elsif ($type eq 'multiselect') {
            $html .= "<label for='$name'>" . ent($description) . '</label>' .
                        $q->scrolling_list(
                            -name => $name,
                            -values => [map { $_->[0] } @{$_->[3]} ],
                            -labels => {
                                map { $_->[0] => $_->[1] }
                                grep { @$_ == 2 } @{$_->[3]}
                            },
                            -multiple => 1
                        );
        } elsif ($type eq 'button') {
            $html .= $q->submit(-name => $name, -label => $description);
        }

        if (!$noerrors && exists($self->{errors}->{$name})) {
            $html .= $q->ul($q->li({ -class => 'giaform_error' }, ent($self->{errors}->{$name})));
        }
    }

    $html .= $q->end_table();
    $html .= $ee;

    return $html;
}

=item has_value FIELD

Does the form have the given FIELD?

=cut
sub has_value ($$) {
    my ($self, $name) = @_;
    return exists($self->{fields}->{$name});
}

=item value FIELD

Get or set the VALUE of the named FIELD. Dies if the form is not valid or the
field does not exist.

=cut
sub value ($$) {
    my ($self, $name) = @_;
    die "value called for field '$name' on invalid form" unless ($self->is_valid());
    die "value called for unknown field '$name'" unless (exists($self->{fields}->{$name}));
    return $self->{fields}->{$name};
}

=item fieldnames

Return in list context the field names in this form.

=cut
sub fieldnames ($) {
    my ($self) = @_;
    return map { $_->[1] } grep { $_->[2] ne 'html' } @{$self->{element}};
}

1;

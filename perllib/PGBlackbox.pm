#!/usr/bin/perl
#
# PGBlackbox.pm:
# Utilities for pgblackbox.
#
# Copyright (c) 2006 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: PGBlackbox.pm,v 1.2 2006-09-06 18:01:41 chris Exp $
#

package PGBlackbox::Spoolfile;

use Carp;
use Fcntl q;
use File::Basename;
use File::stat;
use IO::Handle;

use fields qw(name fh slots);

#
# Format of file: the file begins with a brief note explaining its format and
# purpose, followed by a table mapping times of recording to offsets within the
# file. That is followed by the data themselves, in machine-independent
# Storable format.
#

use constant header =>
    "pgblackbox spool file\n" .
    "This is a binary file which records information about the activity of\n" .
    "PostgreSQL database installation. You should edit/modify it using the\n" .
    "pgblackbox tools, not by hand.\n\n";

# create FILENAME SLOTS
# Create a new spool file with space for SLOTS entries, named FILENAME, and
# return a file handle open on the new file or an error message on failure. The
# new file is created atomically and destroys any previous file of the same
# name.
sub create ($$) {
    my $filename = shift;
    croak "FILENAME must be defined" unless ($filename);
    my $slots = shift;
    croak "SLOTS must be a positive integer"
        unless ($slots && $slots =~ /^[1-9]\d*$/);

    my $err = undef;
    
    # Want to do this atomically so that we don't risk creating a bogus file.
    my $dir = dirname($filename);
    my $base = basename($filename);
    
    my $tempname;
    my $fh;
    do {
        $tempname = sprintf('%s/.%s.%08x', $dir, $base, int(rand(0xffffffff)));
        $fh = new IO::File($tempname, O_RDWR | O_CREAT | O_EXCL);
    } while (!$fh);

    # 10 to give two 0 bytes padding before the time (so we can expand in
    # 2038...).
    my $index = pack('N', $slots) . ("\0" x 10 x $slots;
    if (!$fh->syswrite(header . $index)) {
        $err = "write: $!";
        goto fail;
    } elsif (!$fh->sysseek(0, SEEK_SET)) {
        $err = "lseek: $!";
        goto fail;
    } elsif (!rename($tempname, $filename)) {
        $err = "rename: $!";
        goto fail;
    }

    return $fh;

fail:
    $fh->close();
    unlink($tempname);
    return $err;
}

# open FILENAME [RW]
#
sub open ($;$) {
    my $filename = shift;
    croak "FILENAME must be defined" unless ($filename);
    my $rw = shift;

    # XXX consider whether file is compressed and, if it is, decompress it into
    # a temporary file. Note that obviously we can't open a compressed file RW.
    
    my $fh = new IO::File($filename, $rw ? O_RDONLY : O_RDWR);
    if (!$fh) {
        return "open: $!";
    }

    my $err = undef;
    my $st = stat($fh);
    if ($st->size() < length(header) + 14) {
        $err = "File is too short to be valid";
        goto fail;
    }

    my $buf = '';
    my $n = $fh->sysread($buf, length(header));
    if (!defined($n)) {
        $err = "read: $!";
        goto fail;
    } elsif ($n < length(header)) {
        $err = "File truncated while reading header";
        goto fail;
    } elsif ($buf ne header)) {
        # Header doesn't match.
        $err = "Header doesn't match";
        goto fail;
    }

    $buf = '';
    $n = $fh->sysread($buf, 4);
    if (!defined($n)) {
        $err = "read: $!";
        goto fail;
    } elsif ($n < 4) {
        $err = "File truncated while reading index";
        goto fail;
    }

    my $N = unpack('N', $buf);
    if ($st->size() < length(header) + 4 + $N * 10) {
        $err = "File is too short to contain full index";
        goto fail;
    }

    # XXX could scan index at this point; maybe no point though.
    
    return $fh;
    
fail:
    $fh->close() if ($fh);
    return $err;
}

# new FILENAME [SLOTS]
# 
sub new ($$;$) {
    
}

# append DATA
sub append ($$) {
    my PGBlackbox::Spoolfile $self = shift;
    my $data = shift;
}

package PGBlackbox;

use strict;

1;

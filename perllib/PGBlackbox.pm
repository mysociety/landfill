#!/usr/bin/perl
#
# PGBlackbox.pm:
# Utilities for pgblackbox.
#
# Copyright (c) 2006 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#
# $Id: PGBlackbox.pm,v 1.12 2006-09-13 10:37:10 chris Exp $
#

package PGBlackbox::Spoolfile;

use Carp;
use Errno;
use Fcntl;
use File::Basename;
use File::stat;
use IO::File;
use Storable;

use fields qw(name fh slots cursor rw);

#
# Format of file: the file begins with a brief note explaining its format and
# purpose, followed by a table mapping times of recording to offsets within the
# file. That is followed by the data themselves, in machine-independent
# Storable format.
#

use constant HEADER =>
    "pgblackbox spool file\n" .
    "This is a binary file which records information about the activity of\n" .
    "a PostgreSQL database installation. You should read it using the\n" .
    "pgblackbox tools, not by hand.\n\n";

use constant HEADERLEN => length(HEADER);

# 4 bytes for the slot count.
use constant INDEXOFFSET => (HEADERLEN + 4);

# 6 bytes for time and 4 bytes for offset.
use constant SLOTLEN => 10;

# create FILE SLOTS
# Create a new named spool FILE with space for SLOTS entries, and return a file
# handle open on the new file or an error message on failure. The new file is
# created atomically and destroys any previous file of the same name.
sub create ($$$) {
    my PGBlackbox::Spoolfile $self = shift;

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
        return "open (temporary file): $!" if (!$fh && !$!{EEXIST});
    } while (!$fh);

    # 10 to give two 0 bytes padding before the time (so we can expand in
    # 2038...).
    my $index = pack('N', $slots) . ("\0" x SLOTLEN x $slots);
    my $n;
    if (!($n = $fh->syswrite(HEADER . $index))) {
        $err = "write: $!";
        goto fail;
    } elsif ($n < HEADERLEN + length($index)) {
        $err = "write: Wrote $n, expected " . (HEADERLEN + length($index));
        goto fail;
    } elsif (!$fh->sysseek(0, SEEK_SET)) {
        $err = "lseek: $!";
        goto fail;
    } elsif (!rename($tempname, $filename)) {
        $err = "rename: $!";
        goto fail;
    }

    $self = fields::new($self) unless (ref($self));
    $self->{name} = $filename;
    $self->{fh} = $fh;
    $self->{slots} = $slots;
    $self->{cursor} = 0;
    $self->{rw} = 1;
    
    return $self;

fail:
    $fh->close();
    unlink($tempname);
    return $err;
}

# open FILE [RW]
# Open an existing named spool FILE. Returns a new spoolfile object on success
# or an error message on failure.
sub open ($$;$) {
    my PGBlackbox::Spoolfile $self = shift;

    my $filename = shift;
    croak "FILENAME must be defined" unless ($filename);
    my $rw = shift;

    my ($fh, $fh2);
    
    if ($filename =~ /\.(gz|bz2)$/) {
        return "Cannot open a compressed spool file read/write" if ($rw);
        my %decompressor = ( gz => 'gunzip', bz2 => 'bunzip2' );
        my $prog = $decompressor{$1};
        
        $fh = IO::File->new_tmpfile() or return "open (temp file): $!";
        if (!($fh2 = new IO::File($filename, $rw ? O_RDWR : O_RDONLY))) {
            $err = "open: $!";
            goto fail;
        }

        my $pid = fork();
        if (!defined($pid)) {
            $err = "fork: $!";
            goto fail;
        } elsif (0 == $pid) {
            POSIX::close(0);
            POSIX::dup($fh2->fileno());
            POSIX::close(1);
            POSIX::dup($fh->fileno());
            { exec($prog); }
            print STDERR "exec: $!\n";
            POSIX::_exit(1);
        }

        wait();
        if ($? != 0) {
            $err = "$prog failed with status $?";
            goto fail;
        }

        $fh->sysseek(0, SEEK_SET);
    } else {
        $fh = new IO::File($filename, ($rw ? O_RDWR : O_RDONLY))
            or return "open: $!";
    }

    my $err = undef;
    my $st = stat($fh);
    if ($st->size() < HEADERLEN + 14) {
        $err = "File is too short to be valid";
        goto fail;
    }

    my $buf = '';
    my $n = $fh->sysread($buf, HEADERLEN);
    if (!defined($n)) {
        $err = "read: $!";
        goto fail;
    } elsif ($n < HEADERLEN) {
        $err = "File truncated while reading header";
        goto fail;
    } elsif ($buf ne HEADER) {
        # Header doesn't match.
        $err = "Header doesn't match proper format";
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
    if ($st->size() < HEADERLEN + 4 + $N * 10) {
        $err = "File is too short to contain full index";
        goto fail;
    }

    # Need to identify the current cursor position. Unused slots have time 0,
    # so want the index of the first 0 slot.
    my $i;
    my ($il, $ih) = (0, $N - 1);
    my $th = _slot($fh, $ih);
    if ($th == 0) {
        while ($ih > $il + 1) {
            my $i = int(($ih + $il) / 2);
            my $t = _slot($fh, $i);
            if ($t == 0) {
                $ih = $i;
            } else {
                $il = $i;
            }
        }
    }

    $self = fields::new($self) unless (ref($self));
    $self->{name} = $filename;
    $self->{fh} = $fh;
    $self->{slots} = $N;
    $self->{cursor} = $ih;
    $self->{rw} = $rw;

    return $self;
    
fail:
    $fh->close() if ($fh);
    $fh2->close() if ($fh2);
    return $err;
}

# _slot H I
#
sub _slot ($$) {
    my IO::Seekable $fh = shift;
    my $i = shift;

    my $buf = '';
    goto fail if (!$fh->sysseek(INDEXOFFSET + $i * SLOTLEN, SEEK_SET));
    my $n = $fh->sysread($buf, SLOTLEN);
    goto fail if (!defined($n) || $n != SLOTLEN);

    my ($time, $offset) = unpack('xxNN', $buf);

    return wantarray() ? ($time, $offset) : $time;

fail:
    return wantarray() ? () : undef;
}

# slot I
# Return in list context the time and offset of the values in slot I; in scalar
# context, just the time. Returns the empty list or undef on error.
sub slot ($$) {
    my PGBlackbox::Spoolfile $self = shift;
    my $i = shift;
    croak "I must be an integer between 0 and " . ($self->{slots} - 1)
        unless (defined($i) && $i =~ /^(0|[1-9]\d*)$/ && $i < $self->{slots});

    my @x = _slot($self->fh(), $i);
    goto fail unless (@x);
    
    return wantarray() ? @x : $x[0];

fail:
    return wantarray() ? () : undef;
}

# findslot TIME [SENSE]
# Find the last slot before, or, if SENSE is +1, the first slot after,
# TIME. Returns a slot number or undef if TIME is out of range.
sub findslot ($$;$) {
    my PGBlackbox::Spoolfile $self = shift;
    my $time = shift;
    my $sense = shift;
    $sense ||= -1;

    croak "SENSE should be a positive or a negative integer"
        unless (defined($sense) && $sense =~ /^[+-]?([1-9]\d*)$/);
 
    return undef if ($self->{cursor} == 0);

    my ($il, $ih) = (0, $self->{cursor} - 1);
    my ($tl, $th) = map { scalar($self->slot($_)) } ($il, $ih);

    if ($th < $time) {
        if ($sense < 0) {
            return $ih;
        } else {
            return undef;
        }
    }
    if ($tl > $time) {
        if ($sense < 0) {
            return undef;
        } else {
            return $il;
        }
    }

    while ($ih > $il + 1) {
        my $i = int(($ih + $il) / 2);
        my $t = $self->slot($i);

        if ($t < $time) {
            $il = $i;
        } else {
            $ih = $i;
        }
    }

    return $sense < 0 ? $il : $ih;
}

sub fh ($) {
    my PGBlackbox::Spoolfile $self = shift;
    return $self->{fh};
}

sub rw ($) {
    my PGBlackbox::Spoolfile $self = shift;
    return $self->{rw};
}

sub cursor ($) {
    my PGBlackbox::Spoolfile $self = shift;
    return $self->{cursor};
}

sub name ($) {
    my PGBlackbox::Spoolfile $self = shift;
    return $self->{name};
}

# eof
# Return true if there is space for a further append.
sub eof ($) {
    my PGBlackbox::Spoolfile $self = shift;
    return ($self->{cursor} == $self->{slots});
}

# append ACTIVITY LOCKS CLIENTS
# Write the given ACTIVITY, LOCKS and CLIENTS state to the end of the spool
# file. Returns undef on success or an error message on failure.
sub append ($$$$) {
    my PGBlackbox::Spoolfile $self = shift;
    my ($activity, $locks, $clients) = @_;

    return "Spoolfile is read-only" unless ($self->rw());
    return "No space left" if ($self->eof());

    my $time = time();
    my $buf = Storable::nfreeze([$activity, $locks, $clients]);
    $buf = pack('N', length($buf)) . $buf;
    my $off;
    return "lseek (to append): $!"
        if (!($off = $self->fh()->sysseek(0, SEEK_END)));

    my $n = $self->fh()->syswrite($buf, length($buf));
    return "write (data): $!" if (!defined($n));
    return "write (data): Wrote $n, expected " . length($buf)
        unless ($n == length($buf));

    return "lseek (to write offset): $!"
        if (!$self->fh()->sysseek(INDEXOFFSET + $self->{cursor} * SLOTLEN,
                                    SEEK_SET));
    my $slot = pack('xxNN', $time, $off);

    $n = $self->fh()->syswrite($slot, SLOTLEN);
    if (!defined($n)) {
        return "write (time and offset): $!";
    } elsif ($n < SLOTLEN) {
        return "write (time and offset): Wrote $n, expected " . SLOTLEN;
    }

    ++$self->{cursor};
    return undef;
}

# get INDEX
# Return the data at the given slot INDEX, or an error message on failure.
sub get ($$) {
    my PGBlackbox::Spoolfile $self = shift;
    my $i = shift;
    my ($time, $offset) = $self->slot($i)
        or return "Can't obtain offset for slot $i";
    
    return "lseek (to data): $!"
        if (!$self->fh()->sysseek($offset, SEEK_SET));

    my $buf = '';
    my $n = $self->fh()->sysread($buf, 4);
    return "read (length): $!" if (!defined($n));
    return "read (length): Read $n, expected 4" unless ($n == 4);

    my $len = unpack('N', $buf);

    $buf = '';
    $n = $self->fh()->sysread($buf, $len);
    return "read (data): $!" if (!defined($n));
    return "read (data): Read $n, expected $len" unless ($n == $len);
    
    my $data = Storable::thaw($buf);
    return "Malformed data" if (!defined($data));

    return $data;
}

DESTROY ($) {
    my PGBlackbox::Spoolfile $self = shift;
    $self->fh()->close();
}

package PGBlackbox;

use strict;

$PGBlackbox::default_config_file = "/etc/pgblackbox.conf";

# read_config HANDLE FILE
# Read configuration directives from the given HANDLE, which is assumed to
# refer to the named FILE (this is used in error messages). Returns a reference
# to a hash of config directives to their values on success, or an error
# message on failure.
sub read_config ($$) {
    my IO::Handle $h = shift;
    my $name = shift;
    my %config_vars = map { $_ => 1 } qw(
            user group pid_file
            dbuser dbgroup dbname
            compression_prog compression_suffix compression_age
            spool_dir spool_slots max_size
            interval
            cleanup_interval
        );
    my %config = ( );
    my $retval = \%config;
    my $n = 0;
    while (defined(my $line = $h->getline())) {
        ++$n;
        chomp($line);
        $line =~ s/#.*//;
        $line =~ s/^\s+//;
        $line =~ s/\s+$//;
        next if ($line eq '');
        my ($key, $value) = split(/\s+/, $line);
        if (!exists($config_vars{$key})) {
            $retval = sprintf('%s:%d: bad config directive "%s"',
                                $name, $n, $key);
            last;
        } elsif (exists($config{$key})) {
            $retval = sprintf('%s:%d: repeated config directive "%s"',
                                $name, $n, $key);
            last;
        }
        
        # Simple format checks.
        if ($key =~ /^(compression_age|max_size|interval)$/ && $value !~ /^[1-9]\d*$/) {
            $retval = sprintf('%s:%d: value for "%s" must be a positive integer, not "%s"',
                                $name, $n, $key, $value);
            last;
        }

        if ($key =~ /^(pid_file|compression_prog|spool_dir)$/ && $value !~ m#^/#) {
            $retval = sprintf('%s:%d: value for "%s" must be an absolute path, not "%s"',
                                $name, $n, $key, $value);
            last;
        }
     
        $config{$key} = $value;
    }
    
    $retval = sprintf('%s: %s', $name, $!);
        if ($h->error());

    return $retval;
}

1;

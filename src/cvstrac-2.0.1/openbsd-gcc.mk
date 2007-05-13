#!/usr/bin/make
#
#### The toplevel directory of the source tree.
#
SRCDIR = .

#### C Compiler and options for use in building executables that
#    will run on the platform that is doing the build.
#
BCC = gcc -g -O2 -I/usr/local/include

#### The suffix to add to executable files.  ".exe" for windows.
#    Nothing for unix.
#
E =

#### C Compile and options for use in building executables that
#    will run on the target platform.  This is usually the same
#    as BCC, unless you are cross-compiling.
#
#TCC = gcc -O6
#TCC = gcc -g -O0 -Wall
TCC = gcc -g -O -I/usr/local/include
#TCC = gcc -g -O0 -Wall -fprofile-arcs -ftest-coverage

#### Extra arguments for linking against SQLite
# For OpenBSD, libcrypt is part of libc
LIBSQLITE = -L/usr/local/lib -lsqlite3 -lm

#### Installation directory
#
INSTALLDIR = /var/www/cgi-bin


# You should not need to change anything below this line
###############################################################################
include $(SRCDIR)/main.mk

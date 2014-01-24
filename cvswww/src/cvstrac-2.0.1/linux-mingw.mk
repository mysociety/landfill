#!/usr/bin/make
#
# Makefile for MLAB.
#
#### The toplevel directory of the source tree.
#
SRCDIR = /home/drh/ncr/mlab

#### C Compiler and options for use in building executables that
#    will run on the platform that is doing the build.
#
BCC = gcc -g -O2

#### The suffix to add to executable files.  ".exe" for windows.
#    Nothing for unix.
#
E = .exe

#### C Compile and options for use in building executables that 
#    will run on the target platform.  This is usually the same
#    as BCC, unless you are cross-compiling.
#
TCC = /opt/mingw/bin/i386-mingw32-gcc -O6

#### Extra compiler options needed for programs that use the TCL library.
#
TCL_FLAGS = -I/home/drh/tcltk/8.4win -DSTATIC_BUILD=1

#### Linker options needed to link against the TCL library.
#
LIBTCL = /home/drh/tcltk/8.4win/libtcl84s.a -lmsvcrt

#### Directory containing TCL initialization scripts
#
TCLSCRIPTDIR = /home/drh/tcltk/8.4win/tcl8.4

#### Extra compiler options needed for programs that use the TK library.
#
TK_FLAGS =
#TK_FLAGS = -I/home/drh/tcltk/8.4win

#### Linker options needed to link against the TK library.
#
LIBTK = /home/drh/tcltk/8.4win/libtk84s.a -mwindows -limm32 -lcomctl32

#### Directory containing TK initialization scripts
#
TKSCRIPTDIR = /home/drh/tcltk/8.4win/tk8.4

#### Linker options used to link against zlib
#
ZLIB = -lz

# You should not need to change anything below this line
###############################################################################
include $(SRCDIR)/main.mk

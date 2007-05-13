#!/usr/bin/tclsh
#
# Run this TCL script to generate the "main.mk" makefile.
#

# Basenames of all source files:
#
set src {
  attach
  browse
  cgi
  common
  db
  format
  history
  cvs
  svn
  git
  index
  login
  main
  md5
  rss
  search
  setup
  test
  throttle
  ticket
  timeline
  tools
  user
  view
  wiki
  wikiinit
}

# Name of the final application
#
set name cvstrac

puts {# This file is included by linux-gcc.mk or linux-mingw.mk or possible
# some other makefiles.  This file contains the rules that are common
# to building regardless of the target.
#

XTCC = $(TCC) $(CFLAGS) -I. -I$(SRCDIR)

}
puts -nonewline "SRC ="
foreach s [lsort $src] {
  puts -nonewline " \\\n  \$(SRCDIR)/$s.c"
}
puts "\n"
puts -nonewline "TRANS_SRC ="
foreach s [lsort $src] {
  puts -nonewline " \\\n  ${s}_.c"
}
puts "\n"
puts -nonewline "OBJ ="
foreach s [lsort $src] {
  puts -nonewline " \\\n  $s.o"
}
puts "\n"
puts "APPNAME = $name\$(E)"
puts "\n"

puts {
all:	$(APPNAME) index.html

install:	$(APPNAME)
	mv $(APPNAME) $(INSTALLDIR)

translate:	$(SRCDIR)/translate.c
	$(BCC) -o translate $(SRCDIR)/translate.c

makeheaders:	$(SRCDIR)/makeheaders.c
	$(BCC) -o makeheaders $(SRCDIR)/makeheaders.c

mkindex:	$(SRCDIR)/mkindex.c
	$(BCC) -o mkindex $(SRCDIR)/mkindex.c

makewikiinit:	$(SRCDIR)/makewikiinit.c
	$(BCC) -o makewikiinit $(SRCDIR)/makewikiinit.c $(LIBSQLITE)

maketestdb:	$(SRCDIR)/maketestdb.c
	$(BCC) -o maketestdb $(SRCDIR)/maketestdb.c $(LIBSQLITE)

$(APPNAME):	headers $(OBJ)
	$(TCC) -o $(APPNAME) $(OBJ) $(LIBSQLITE)

index.html:	$(SRCDIR)/webpage.html $(SRCDIR)/VERSION
	sed -f $(SRCDIR)/VERSION $(SRCDIR)/webpage.html >index.html

clean:	
	rm -f *.o *_.c $(APPNAME)
	rm -f makewikiinit maketestdb
	rm -f translate makeheaders mkindex page_index.h index.html headers}

set hfiles {}
foreach s [lsort $src] {lappend hfiles $s.h}
puts "\trm -f $hfiles\n"

set mhargs {}
foreach s [lsort $src] {
  append mhargs " ${s}_.c:$s.h"
  set extra_h($s) {}
}
puts "headers:\tmakeheaders mkindex \$(TRANS_SRC)"
puts "\t./makeheaders $mhargs"
puts "\t./mkindex \$(TRANS_SRC) >page_index.h"
puts "\ttouch headers\n"
set extra_h(main) page_index.h

foreach s [lsort $src] {
  puts "${s}_.c:\t\$(SRCDIR)/$s.c \$(SRCDIR)/VERSION translate"
  puts "\t./translate \$(SRCDIR)/$s.c | sed -f \$(SRCDIR)/VERSION >${s}_.c\n"
  puts "$s.o:\t${s}_.c $s.h $extra_h($s) \$(SRCDIR)/config.h"
  puts "\t\$(XTCC) -o $s.o -c ${s}_.c\n"
  puts "$s.h:\tmakeheaders"
  puts "\t./makeheaders $mhargs\n\ttouch headers\n"
}

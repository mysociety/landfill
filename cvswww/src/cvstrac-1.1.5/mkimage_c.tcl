#!/usr/bin/tclsh
#
# This script reads the names of GIF files from the command line.
# It outputs C code that can be linked into CVSTrac so that the
# named GIFs are accessible by the web interface.
#
# Example usage:
#
#      tclsh mkimage_c.tcl *.gif >image.c
#

puts {/* This code is automatically generated. Do not edit. */}
puts {#include "image.h"}
foreach f $argv {
  set fd [open $f]
  fconfigure $fd -translation binary
  set data [read $fd]
  close $fd
  binary scan $data H[expr {[string length $data]*2}] hex
  puts "/*\n** WEBPAGE: /[file tail $f]\n*/"
  regsub -all {[^a-zA-Z0-9_]} [file tail $f] _ nm
  puts "void image_${nm}(void){"
  puts -nonewline "  static const char $nm\[\] = {"
  for {set i 0} {[string index $hex $i]!=""} {incr i 2} {
    if {$i%16==0} {puts -nonewline "\n   "}
    set byte [string range $hex $i [expr {$i+1}]]
    puts -nonewline " 0x$byte,"
  }
  puts "\n  };"
  puts "  cgi_set_content_type(\"image/gif\");"
  puts "  cgi_append_content($nm, sizeof($nm));"
  puts "  g.isConst = 1;"
  puts "}"
  puts ""
}

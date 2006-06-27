#!/usr/bin/perl -I../

use warnings;
use strict;
use mySociety::Boxes::Config;
use mySociety::Boxes::Routines;

print "Content-Type: text/html\n\n";
my $boxid=param("boxid") || 1;
print <<EOhtml;
<div class="msociety_yourpolitics">
<iframe src="/cgi-bin/showfeed/generateiframe.cgi?boxid=$boxid" width="250" height="450" border="0" frameborder="0" ></iframe>
</div>
EOhtml

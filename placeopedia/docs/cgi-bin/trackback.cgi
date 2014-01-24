#!/usr/bin/perl

use warnings;
use strict;
use FindBin;
use lib "$FindBin::Bin/../../perllib";
use lib "$FindBin::Bin/../../../perllib";
use mySociety::Config;
BEGIN {
    mySociety::Config::set_file("$FindBin::Bin/../../conf/general");
}
use PoP;
use CGI qw/:standard/;
use HTML::Scrubber;

my $site_name= mySociety::Config::get('SITE_NAME');
my %Passed_Values;
my %quoted;

{
        foreach my $param (param()) {
                $Passed_Values{$param}=param($param);
        }
	
        $Passed_Values{title} ||= '';
        $Passed_Values{blog_name} ||= $Passed_Values{title};
        $Passed_Values{excerpt} ||= '';
        $Passed_Values{url}||= '';

        if ($Passed_Values{excerpt} =~ m/^(.{210}.+?\b)/s) {
                $Passed_Values{excerpt}= $dbh->quote($1 . "...");
        } else  {
                $Passed_Values{excerpt}= $dbh->quote($Passed_Values{excerpt});
        }
	my %quoted;
        my $scrubber= HTML::Scrubber->new();
        $scrubber->allow(qw[a em strong p br]);
        $scrubber->comment(0);
        $Passed_Values{excerpt}= $scrubber->scrub($Passed_Values{excerpt});
        $Passed_Values{blog_name}= $scrubber->scrub($Passed_Values{blog_name});
        $Passed_Values{title}= $scrubber->scrub($Passed_Values{title});
        $Passed_Values{url}= $scrubber->scrub($Passed_Values{url});

	($Passed_Values{postid})= $ENV{REQUEST_URI} =~ m#/(\d+)/?$#; 
        if (not defined $Passed_Values{postid}) {
                &die_cleanly("form not passed on");
        }
	$Passed_Values{title} =~ s#<\s*a\s+#<a rel="nofollow" #g;
	$Passed_Values{excerpt} =~ s#<\s*a\s+#<a rel="nofollow" #g;

	$Passed_Values{"comment"}=<<EOcomment;
	<div class="trackback">
		<strong>$Passed_Values{title}</strong>
		<p>
			$Passed_Values{excerpt}
		</p>
		<p>
			<a rel="nofollow" href="$Passed_Values{url}">read more in $Passed_Values{blog_name}</a>
		</p>
	</div>
EOcomment

	foreach my $key (keys %Passed_Values) {
		$quoted{$key}=$dbh->quote($Passed_Values{$key});
	}

	my $query= $dbh->prepare("
		insert into comments set
	   	  comment=$quoted{comment},
	   	  posted=now(),
	   	  email='',
	   	  name='trackback',
		  postid=$quoted{postid},
	   	  istrackback=1,
                  site='$site_name'
	");
	$query->execute;

	$dbh->do("update posts set commentcount=commentcount+1
		  where postid=$Passed_Values{postid}");

	print "Content-Type: text/plain\r\n\r\n<error>0</error><message>it worked</message>\n";
}

sub die_cleanly {
	my $error= shift || 'unknown';
	print "Content-Type: text/plain\r\n\r\n<error>1</error><message>it went wrong</message>\n";
        exit(0);
}

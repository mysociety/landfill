#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;
use HTML::Scrubber;
use Email::Valid;
use CGI::Fast qw/param/;
use mysociety::NotApathetic::Config;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $url_prefix= $mysociety::NotApathetic::Config::url;

my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});
my %State; # State variables during display.
my %Passed_Values;

while (new CGI::Fast()) {
	foreach my $param (param()) {
		$Passed_Values{$param}=param($param);
	}

	&die_cleanly("internal error") unless ($Passed_Values{postid} =~ /^\d+$/);

	$Passed_Values{email} ||= '';
	$Passed_Values{text} ||= '';
	$Passed_Values{author} ||= 'Anonymous';
	$Passed_Values{url}= '';

	&handle_comment;
}


sub handle_comment {
	my %quoted;
	unless (Email::Valid->address( -address => $Passed_Values{email},
	                           -mxcheck => 1 )) {
		&die_cleanly("Email address invalid");
	}
	$Passed_Values{text} =~ s#<\s*a\s+#<a rel="nofollow" #g;
	my $scrubber= HTML::Scrubber->new();
	$scrubber->allow(qw[a em cite strong p br]);
	$scrubber->comment(0);
	$Passed_Values{text}= $scrubber->scrub($Passed_Values{text});
        $Passed_Values{text} =~ s/[\x80-\x9f]//g;
	$Passed_Values{author}= $scrubber->scrub($Passed_Values{author});

        unless ($Passed_Values{text} && $Passed_Values{text} ne 'Write your response...') {
            &die_cleanly("Please give a response.");
        }

	foreach my $pv (keys %Passed_Values) {
		$quoted{$pv}= $dbh->quote($Passed_Values{$pv});
	}


	$dbh->do("
		insert into comments
		   set postid=$quoted{postid},
		       comment=$quoted{text},
		       email=$quoted{email},
		       name=$quoted{author},
		       posted=now()
	");
	$dbh->do("
		update posts set commentcount=commentcount+1 where postid=$quoted{postid}
	");

	print "Location: $url_prefix/comments/$Passed_Values{postid}\r\n\r\n";
}


sub die_cleanly {
	my $reason=shift;
	print "Content-Type: text/plain\r\n\r\n

	Your submission failed:
		$reason
	Please go back and correct this before submitting again.
	";
	exit(0);
}

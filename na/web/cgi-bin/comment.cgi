#!/usr/bin/perl

use warnings;
use strict;
use HTML::Entities;
use HTML::Scrubber;
use Mail::Mailer qw/sendmail/;
use CGI qw/param/;
use Email::Valid;
use mysociety::NotApathetic::Config;
use mysociety::NotApathetic::Routines;

if ($mysociety::NotApathetic::Config::site_open_for_additions == 0) {
    print "Location: $mysociety::NotApathetic::Config::url\n\n";
    exit(0);
}

my $domain = $mysociety::NotApathetic::Config::domain; # DSN connection string
my $site_name= $mysociety::NotApathetic::Config::site_name; # DSN connection string
my $email_noreply= $mysociety::NotApathetic::Config::email_noreply; # DSN connection string
my $url_prefix= $mysociety::NotApathetic::Config::url;
my %State; # State variables during display.
my %Passed_Values;

{
	&setup_db;

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
		       posted=now(),
		       site='$site_name'
	");

	$dbh->do(" update posts set commentcount=commentcount+1 where postid=$quoted{postid} ");

        &email_comment_to_person();
	print "Location: $url_prefix/comments/$Passed_Values{postid}\r\n\r\n";
}



sub email_comment_to_person {
        my $mailer= new Mail::Mailer 'sendmail';
        my $to_person;
        my %headers;

        my $query= $dbh->prepare("select email from posts where postid=? and emailalert=1 "); 
        $query->execute($Passed_Values{postid});
        ($to_person) = $query->fetchrow_array;

        return unless ($to_person); # false id or no notification wanted

        $headers{'To'}= "$to_person" ;
        $headers{"From"}= "\"$site_name\" <$email_noreply\@$domain>";
        $headers{"Subject"}= "Reply to your $site_name post";
        $mailer->open(\%headers);

        print $mailer <<EOmail;

Someone has replied to your $site_name post. 
You can view it here:

        $url_prefix/comments/$Passed_Values{postid}

EOmail

        $mailer->close;

        return();
}

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
use HTML::Entities;
use HTML::Scrubber;
use Email::Valid;
use Text::Wrap;
use Mail::Mailer qw(sendmail);
use CGI qw/param/;

my $url_prefix= mySociety::Config::get('URL');
my $email_domain= mySociety::Config::get('EMAIL_DOMAIN');
my $site_name= mySociety::Config::get('SITE_NAME');

{
	my %Passed_Values;
	my $mailer= new Mail::Mailer 'sendmail';
        foreach my $param (param()) {
                $Passed_Values{$param}=param($param);
        }

	&die_cleanly unless defined $Passed_Values{from};
	&die_cleanly unless defined $Passed_Values{to};
	&die_cleanly unless defined $Passed_Values{name};
        &die_cleanly if (defined $Passed_Values{entryid} && $Passed_Values{entryid} !~ /^\d+$/);
    
	my $result;
	if (defined $Passed_Values{entryid}) {
            my $query=$dbh->prepare("
                        select  postid, title, shortwhy
			  from  posts
		  	 where  postid='$Passed_Values{entryid}'
                           and hidden=0
                           and site='$site_name' 
                           and validated=1
                       "); # XXX order by first_seen needs to change
            $query->execute;
            $result= $query->fetchrow_hashref;
	}

	unless (	(Email::Valid->address(-address => $Passed_Values{"from"},-mxcheck => 1 ))
			and (Email::Valid->address(-address => $Passed_Values{"to"},-mxcheck => 1 ))
			)
	{
		if (defined $Passed_Values{error_redirect})
		{
			print "Location: $Passed_Values{error_redirect}\r\n\r\n";
		}
		else
		{
			print "Location: $url_prefix/\r\n\r\n";
		}
                next;
	}


	my $from_address      = $Passed_Values{"from"} || "team$email_domain";
	my $from_name         = $Passed_Values{"name"} || 'Someone';
	my $to_person         = $Passed_Values{"to"} ;
	delete $Passed_Values{"subject"};
	delete $Passed_Values{"submitter_name"};
	delete $Passed_Values{"recipient"};
	my %headers;
	$headers{'To'}= "$to_person" ;
	if(defined $Passed_Values{entryid}){
		$headers{"Subject"} = "$site_name email: $result->{title}";
	} else{
		$headers{"Subject"} = "Have you heard about $site_name ?";
	}
	$headers{"From"}= "$from_name <$from_address>";
	$headers{"X-Originating-IP"}= $ENV{'HTTP_X_FORWARDED_FOR'}  || $ENV{'REMOTE_ADDR'} || return;
	$mailer->open(\%headers);
	
	my $shortwhy = "";
	if (defined $Passed_Values{entryid}){
		$shortwhy= wrap('     ','     ', $result->{shortwhy});
	}
	my $message= wrap('     ','     ', $Passed_Values{message});
	
	if (defined $Passed_Values{entryid}){
		print $mailer <<EOmail;

$headers{From} saw this item on $site_name and
thought you should see it:
$message

     $result->{title}

     $shortwhy

     $url_prefix/?$Passed_Values{entryid}
EOmail
		}
		else
		{
			print $mailer <<EOmail;

$headers{From} wanted to tell you about $site_name

$message

EOmail
		}
    	$mailer->close;

	print "Location: $url_prefix/emailfriend/emailsent.shtml?$Passed_Values{entryid}\r\n\r\n";

}

sub die_cleanly {
        my $error= shift || 'no error given';
        print "Location: $url/error/?$error\n\n";
	exit(0);
}


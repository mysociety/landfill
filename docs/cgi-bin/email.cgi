#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;
use HTML::Scrubber;
use Email::Valid;
use Text::Wrap;
use Mail::Mailer qw(sendmail);
use CGI qw/param/;
use mysociety::NotApathetic::Config;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;         # database password
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $url_prefix= $mysociety::NotApathetic::Config::url;
my $email_domain= $mysociety::NotApathetic::Config::email_domain;
my $site_name= $mysociety::NotApathetic::Config::site_name;
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 1});


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
	$from_name=~ s#[^a-zA-Z0-9 ']##g;
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
	#$headers{"X-Originating-IP"}= $ENV{'HTTP_X_FORWARDED_FOR'}  || $ENV{'REMOTE_ADDR'} || return;
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

     $url_prefix/comments?$Passed_Values{entryid}
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
        &mysociety::NotApathetic::Config::die_cleanly(@_);
}


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
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $url_prefix= $mysociety::NotApathetic::Config::url;
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});
my %Passed_Values;
my $mailer= new Mail::Mailer 'sendmail';

{
        foreach my $param (param()) {
                $Passed_Values{$param}=param($param);
        }

	&die_cleanly unless defined $Passed_Values{entryid};
	&die_cleanly unless defined $Passed_Values{from};
	&die_cleanly unless defined $Passed_Values{to};
	&die_cleanly unless defined $Passed_Values{name};
	&die_cleanly unless $Passed_Values{entryid} =~ /^\d+$/; 


        my $query=$dbh->prepare("
                      select postid,
                             title,
			     shortwhy
                        from posts
	 	       where postid='$Passed_Values{entryid}'
                         and hidden=0
			 and validated=1
                       "); # XXX order by first_seen needs to change

        $query->execute;
        my $result;
	$result= $query->fetchrow_hashref;

    	my %headers;

    	unless (  (Email::Valid->address
                        ( -address => $Passed_Values{"from"},
                          -mxcheck => 1 ) )
    	     and  (Email::Valid->address
                        ( -address => $Passed_Values{"to"},
                          -mxcheck => 1 ) )
		)
           {
                   if (defined $Passed_Values{error_redirect}) {
                        print "Location: $Passed_Values{error_redirect}\n\n";
                   }
                   else  {
                        print "Location: http://www.mugss.org/contact/error.shtml\n\n";
                   }
                   exit(0);
    	}


    	my $from_address      = $Passed_Values{"from"} || 'email-a-story@msmith.net';
    	my $from_name         = $Passed_Values{"name"} || 'Someone';
    	my $to_person         = $Passed_Values{"to"} ;
    	delete $Passed_Values{"subject"};
    	delete $Passed_Values{"submitter_name"};
    	delete $Passed_Values{"recipient"};

    	$headers{'To'}= "$to_person" ;
    	$headers{"Subject"}= "NotApathetic.com email: $result->{title}" ;
    	$headers{"From"}= "$from_name <$from_address>" ;
    	$headers{"X-Originating-IP"}= $ENV{'HTTP_X_FORWARDED_FOR'}  || $ENV{'REMOTE_ADDR'} || return;
    	$mailer->open(\%headers);

	my $shortwhy= wrap('     ','     ', $result->{shortwhy});
	my $message= wrap('     ','     ', $Passed_Values{message});

print $mailer <<EOmail;

$headers{From} saw this item on NotApathetic.com and
thought you should see it:
$message

     $result->{title}

     $shortwhy

     $url_prefix/comments/$Passed_Values{entryid}
EOmail

    	$mailer->close;

	print "Location: $url_prefix/emailsent.shtml?$Passed_Values{entryid}\n\n";

}
sub die_cleanly {
        my $reason=shift || '';
        print "Content-Type: text/plain\n\n

        Your submission failed:
                $reason
        Please go back and correct this before submitting again.
        ";
        exit(0);
}


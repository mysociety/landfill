#!/usr/bin/perl

push @INC, "../";
use warnings;
use strict;
use DBI;
use Email::Valid;
use CGI::Fast qw/param/;
use Mail::Mailer qw(sendmail);
use mysociety::NotApathetic::Config;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $url_prefix= $mysociety::NotApathetic::Config::url;
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});
my %State; # State variables during display.
my %Passed_Values;
my $search_term;

while (new CGI::Fast()) {
	print "Content-Type: text/html\n\n";
        my $notifyid_q = $dbh->quote(param('u'));
        my $auth_code_q = $dbh->quote(param('c'));
        my $query=$dbh->prepare ("select email,search from emailnotify
                                    where notifyid= $notifyid_q
                                      and authcode = $auth_code_q");

        $query->execute;

        if ($query->rows != 1 ) {
                &die_cleanly("Code either used or incorrect");
        }
        else {
		my ($address, $search_term)= $query->fetchrow_array;

                $query= $dbh->prepare ("update emailnotify
                                     set validated      = 1 ,
                                         authcode       = 'used',
					 lastupdated    = now()
                                   where notifyid       = $notifyid_q
                                     and authcode       = $auth_code_q
                                ");
                $query->execute || &die_cleanly("sql error");
                if ($query->rows == 1 ) { 
			&send_notify_email($address);
                        print "<h2>Successful</h2>\n<p>You registration is now cancelled.</p>";
                } else {
                        print "<h2>Request Failed</h2>\n<p>code and id did not match</p>";
                }
        }
}

sub send_notify_email {
        my $to_person= shift;
	my %headers;
	my $mailer= new Mail::Mailer 'sendmail';

        $headers{'To'}= "$to_person" ;
        $headers{"From"}= "NotApathetic.com <donotreply\@notapathetic.com>" ;
        $headers{"Subject"}= "Cancelled Updates from NotApathetic.com";
        $headers{"X-Originating-IP"}= $ENV{'HTTP_X_FORWARDED_FOR'}  || $ENV{'REMOTE_ADDR'} || return;
        $mailer->open(\%headers);


print $mailer <<EOmail;

We've had a request for this address to cancel a notification
of new posts on NotApathetic.com .

If you didn't request this (or have changed your mind), you can 
reregister by following the below link
	$url_prefix/emailnotify/?$search_term

EOmail

        $mailer->close;

}

sub die_cleanly {
        my $reason=shift;
        print "

        Your submission failed:
                $reason
        Please go back and correct this before submitting again.
        ";
        exit(0);
}


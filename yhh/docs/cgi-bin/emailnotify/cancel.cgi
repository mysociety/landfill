#!/usr/bin/perl -I ../

push @INC, "../";
use warnings;
use strict;
use DBI;
use Email::Valid;
use CGI qw/param/;
use Mail::Mailer qw(sendmail);
use mysociety::NotApathetic::Config;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $url_prefix= $mysociety::NotApathetic::Config::url;
my $site_name= $mysociety::NotApathetic::Config::site_name;
my $email_noreply= $mysociety::NotApathetic::Config::email_noreply;
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 1});
my %Passed_Values;

{
	print "Content-Type: text/html; charset=iso-8859-1\r\n\r\n";
        my $notifyid_q = $dbh->quote(param('u'));
        my $auth_code_q = $dbh->quote(param('c'));
        my $query=$dbh->prepare ("select email,search from emailnotify
                                    where notifyid= $notifyid_q
                                      and authcode = $auth_code_q");

        $query->execute;
        my $ref= $query->fetchrow_hashref;

        if (not defined $ref) {
                &die_cleanly("Code either used or incorrect");
        } else {

                $query= $dbh->prepare ("
                    update emailnotify
                                     set cancelled      = 1 ,
                                         authcode       = 'used',
					 lastupdated    = now()
                                   where notifyid       = $notifyid_q
                                     and authcode       = $auth_code_q
                                ");
                $query->execute || &die_cleanly("sql error");
                if ($query->rows == 1 ) { 
			&send_notify_email($ref->{email}, $ref->{search});
                        print "<h2>Successful</h2>\n<p>You registration is now cancelled.</p>";
                } else {
                        print "<h2>Request Failed</h2>\n<p>code and id did not match</p>";
                }
        }
}

sub send_notify_email {
        my $to_person= shift;
        my $search_term= shift || '';
	my %headers;
	my $mailer= new Mail::Mailer 'sendmail';

        $headers{'To'}= "$to_person" ;
        $headers{"From"}= "\"$site_name\" <$email_noreply>" ;
        $headers{"Subject"}= "Cancelled Updates from $site_name";
        $headers{"X-Originating-IP"}= $ENV{'HTTP_X_FORWARDED_FOR'}  || $ENV{'REMOTE_ADDR'} || return;
        $mailer->open(\%headers);


print $mailer <<EOmail;

We've had a request for this address to cancel a notification
of new posts on $site_name.

If you didn't request this (or have changed your mind), you can 
reregister by following the below link
	$url_prefix/emailnotify/?$search_term

EOmail

        $mailer->close;

}

sub die_cleanly {
        &mysociety::NotApathetic::Config::die_cleanly(@_);
}


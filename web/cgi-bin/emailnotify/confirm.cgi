#!/usr/bin/perl -I ../

push @INC, "../";
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
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});
my %State; # State variables during display.
my %Passed_Values;

while (new CGI::Fast()) {
	print "Content-Type: text/html; charset=iso-8859-1\r\n\r\n";
        my $notifyid_q = $dbh->quote(param('u'));
        my $auth_code_q = $dbh->quote(param('c'));
        my $query=$dbh->prepare ("select * from emailnotify
                                    where notifyid= $notifyid_q
                                      and validated = 0
                                      and authcode = $auth_code_q");

        $query->execute;

        if ($query->rows != 1 ) {
                &die_cleanly("Code either used or incorrect");
        }
        else {
        	my $randomness = rand(); $randomness=~ s/^0\.(\d+)/$1/;
        	my $new_authcode_q= $dbh->quote($randomness);
                $query= $dbh->prepare ("update emailnotify
                                     set validated      = 1 ,
                                         authcode       = $new_authcode_q,
					 lastupdated    = now()
                                   where notifyid       = $notifyid_q
                                     and authcode       = $auth_code_q
                                ");
                $query->execute || &die_cleanly("sql error");
                if ($query->rows == 1 ) { 
                        print "<h2>Successful</h2>\n<p>You are now registered for updates.</p>";
                } else {
                        print "<h2>Request Failed</h2>\n<p>code and id did not match</p>";
                }
        }
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


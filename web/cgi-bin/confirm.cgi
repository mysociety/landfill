#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;
use HTML::Scrubber;
use Email::Valid;
use CGI::Fast qw/param/;

use mysociety::IVotedForYouBecause::Config;

my $dsn = $mysociety::IVotedForYouBecause::Config::dsn; # DSN connection string
my $db_username= $mysociety::IVotedForYouBecause::Config::db_username;              # database username
my $db_password= $mysociety::IVotedForYouBecause::Config::db_password;         # database password
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});
my $url_prefix= $mysociety::IVotedForYouBecause::Config::url;

begin:

while (new CGI::Fast()) {

        my $postid_q = $dbh->quote(param('u'));
        my $auth_code_q = $dbh->quote(param('c'));
        my $query=$dbh->prepare ("select * from posts
                                    where postid= $postid_q
                                      and validated = 0
                                      and authcode = $auth_code_q");

        $query->execute;

        if ($query->rows != 1 ) {
                &die_cleanly("Code either used or incorrect");
        }
        else {
                $query= $dbh->prepare ("update posts
                                     set validated      = 1 ,
                                         authcode       = 'used',
					 posted		= now()
                                   where postid         = $postid_q
                                     and authcode       = $auth_code_q
                                ");
                $query->execute || &die_cleanly("sql error");
                if ($query->rows == 1 ) { 
                        print "Location: $url_prefix\r\n\r\n";      
                } else {
                        print "Location: $url_prefix/new/checkemail/failed.shtml\r\n\r\n";       
                }
        }
}


sub die_cleanly {
        my $reason=shift;
        print "Content-Type: text/plain\r\n\r\n

        Your submission failed:
                $reason
        Please go back and correct this before submitting again.
        ";
        goto begin;
}


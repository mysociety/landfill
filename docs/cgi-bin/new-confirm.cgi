#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;
use HTML::Scrubber;
use Email::Valid;
use CGI qw/param/;

use mysociety::NotApathetic::Config;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $site_name= $mysociety::NotApathetic::Config::site_name;
my $dbh;
my $url_prefix= $mysociety::NotApathetic::Config::url;

{
    eval {
        if (!defined($dbh) || !eval { $dbh->ping() }) {
            $dbh = DBI->connect($dsn, $db_username, $db_password, {RaiseError => 1});
        }
        my $postid = param('u');
        my $postid_q = $dbh->quote(param('u'));
        my $auth_code_q = $dbh->quote(param('c'));
        my $query=$dbh->prepare ("select * from posts
                                    where postid= $postid_q
                                      and validated = 0 
                                      and authcode = $auth_code_q
				      and site='$site_name'
				");

        $query->execute;

        if ($query->rows != 1 ) {
            $query=$dbh->prepare ("select * from posts
                                    where postid= $postid_q
                                      and validated = 1 
				      and site='$site_name'
				 ");
            $query->execute;

            if ($query->rows == 1 ) {
                        print "Location: $url_prefix/\r\n\r\n";      
            } else {
                        print "Location: $url_prefix/new/checkemail/failed.shtml\r\n\r\n";       
            }
        }
        else {
		my $new_authcode= rand(); $new_authcode=~ s/^0\.(\d+)/$1/;

                $query= $dbh->prepare ("update posts
                                     set validated      = 1 ,
                                         authcode       = '$new_authcode',
					 posted		= now()
                                   where postid         = $postid_q
                                     and authcode       = $auth_code_q
				     and site='$site_name'
                                ");
                $query->execute || &die_cleanly("sql error");
                if ($query->rows == 1 ) { 
			my $postid= $postid_q;
			$postid=~ s#'##g;
                        print "Location: $url_prefix/comments/$postid\r\n\r\n";      
                } else {
                        print "Location: $url_prefix/new/checkemail/failed.shtml\r\n\r\n";       
                }
        }
    };
    if ($@) {
        &die_cleanly($@);
    }
}

sub die_cleanly {
        &mysociety::NotApathetic::Config::die_cleanly(@_);
}


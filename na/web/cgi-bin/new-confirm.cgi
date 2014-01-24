#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;
use HTML::Scrubber;
use Email::Valid;
use CGI qw/param/;
use CGI::Carp qw/warningsToBrowser fatalsToBrowser/;
use mysociety::NotApathetic::Config;
use mysociety::NotApathetic::Routines;
my $site_name= $mysociety::NotApathetic::Config::site_name;


{
	my ($postid, $auth_code) = $ENV{QUERY_STRING}=~ m#(\d+)/(\d+)#;

	if (not defined $auth_code) {
		print "Location: $mysociety::NotApathetic::Config::url\n\n";
		exit(0);
	}

	&setup_db;

        my $query=$dbh->prepare ("select * from posts
                                    where postid= $postid
				      and site='$site_name'
                                      and authcode = $auth_code");

        $query->execute;

        if ( $query->fetchrow_hashref ) {
		my $new_authcode= rand(); $new_authcode=~ s/^0\.(\d+)/$1/;

                $query= $dbh->prepare ("update posts
                                     set validated      = 1 ,
                                         authcode       = '$new_authcode',
					 posted		= now()
                                   where postid         = $postid
                                     and authcode       = $auth_code
				     and site		= '$site_name'
                                ");
                $query->execute || &die_cleanly("sql error");
                if ( $query->rows ) { 
			my $postid= $postid;
			$postid=~ s#'##g;
                        print "Location: $url_prefix/new/stage2/?u=$postid;c=$new_authcode\r\n\r\n";      
                } else {
                        print "Location: $url_prefix/new/checkemail/failed.shtml\r\n\r\n";       
                }
        }
        else {
	    # check whether it's already confirmed
            $query=$dbh->prepare ("select * from posts
                                    where postid= $postid
				      and site='$site_name'
                                      and validated = 1 ");
            $query->execute;

            if ( $query->fetchrow_hashref ) {
                        print "Location: $url_prefix/\r\n\r\n";      
            } else {
                        print "Location: $url_prefix/new/checkemail/failed.shtml\r\n\r\n";       
            }
        }
}


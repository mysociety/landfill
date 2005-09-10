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
use CGI qw/param/;

my $site_name= mySociety::Config::get('SITE_NAME');
my $url_prefix= mySociety::Config::get('URL');

{
    eval {
        my $postid = param('u') || 0;
        my $postid_q = $dbh->quote($postid);
        my $auth_code = param('c') || '';
        my $auth_code_q = $dbh->quote($auth_code);
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
                $query->execute;
                if ($query->rows == 1 ) { 
			my $postid= $postid_q;
			$postid=~ s#'##g;
                        print "Location: $url_prefix/comments.shtml?$postid\r\n\r\n";      
                } else {
                        print "Location: $url_prefix/new/checkemail/failed.shtml\r\n\r\n";       
                }
        }
    };
}


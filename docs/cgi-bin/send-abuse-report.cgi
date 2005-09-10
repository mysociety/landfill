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
use Text::Wrap;
my %Passed_Values;

my $site_name= mySociety::Config::get('SITE_NAME');
my $url_prefix= mySociety::Config::get('URL');
my $admin_url_prefix= mySociety::Config::get('ADMIN_URL');
my $abuse_address= mySociety::Config::get('EMAIL_ABUSE');

{
        foreach my $param (param()) {
		$Passed_Values{$param}=param($param);
		$Passed_Values{$param}=~ s#\n# #gsi;
	}

	$Passed_Values{body}= wrap("","    ", $Passed_Values{'body'});
	&send_email;
	print "Location: $url_prefix/abuse/sent/\r\n\r\n";
}


sub send_email {
    my $postid= param('postid') || return;
    my $commentid= param('commentid') || '';

    use Mail::Mailer;
    my $mailer= new Mail::Mailer 'sendmail';#, Server => 'mailrouter.mcc.ac.uk';
    my %headers;
    my $address      = $abuse_address;
    my $name         = "$site_name abuse report";

    $headers{"Subject"}= "$site_name of abuse $admin_url_prefix/?$postid#$commentid" ;
    $headers{"To"}= "$name <$address>" ;
    $headers{'From'}= "$site_name <$abuse_address>" ;
    $headers{"X-Originating-IP"}= $ENV{'HTTP_X_FORWARDED_FOR'}  || $ENV{'REMOTE_ADDR'} || return;
    $mailer->open(\%headers);
    my $about;
        if ($commentid) {
                $about=&output_comment($postid,$commentid);
        } else {
                $about=&output_post($postid,$commentid);
	}

    $about=wrap('',"    ", $about);
    print $mailer <<EOmail;

Someone thinks $admin_url_prefix/?$postid#$commentid is abusive.

IP    :  $ENV{REMOTE_ADDR}
Name  :  $Passed_Values{name}
Email :  $Passed_Values{email}
Reason: $Passed_Values{body}

talking about:
	$about	
	
Delete comment: $admin_url_prefix/cgi-bin/hide.cgi?postid=$postid;commentid=$commentid

EOmail

    $mailer->close;

    return;
}

sub output_comment {
        my $postid= shift;
        my $commentid=shift;

        my $query=$dbh->prepare("
                      select * from comments
                       where postid=? and commentid=?
                         and site='$site_name'
                       ");
        $query->execute ($postid, $commentid);
        my $result=$query->fetchrow_hashref;

        return $result->{comment};
}

sub output_post {
        my $postid= shift;

        my $query=$dbh->prepare("
                      select * from posts
                       where postid=?
                         and site='$site_name'
                       ");
        $query->execute ($postid);
        my $result=$query->fetchrow_hashref;
        return "$result->{title}\n$result->{why}";
}


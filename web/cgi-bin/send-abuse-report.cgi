#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;
use HTML::Scrubber;
use Email::Valid;
use CGI qw/param/;
use Text::Wrap;
use mysociety::NotApathetic::Config;
my %Passed_Values;

my $dsn = $mysociety::NotApathetic::Config::dsn; # DSN connection string
my $db_username= $mysociety::NotApathetic::Config::db_username;              # database username
my $db_password= $mysociety::NotApathetic::Config::db_password;         # database password
my $site_name= $mysociety::NotApathetic::Config::site_name;         # database password
my $url_prefix= $mysociety::NotApathetic::Config::url;
my $admin_url_prefix= $mysociety::NotApathetic::Config::admin_url;
my $abuse_address= $mysociety::NotApathetic::Config::abuse_address;
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 1});
	

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

    $headers{"Subject"}= "$site_name of abuse $admin_url_prefix/comments.shtml?$postid#$commentid" ;
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

Someone thinks $admin_url_prefix/comments.shtml?$postid#$commentid is abusive.

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


sub die_cleanly {
        &mysociety::NotApathetic::Config::die_cleanly(@_);
}



sub output_comment {
        my $postid= shift;
        my $commentid=shift;

        my $query=$dbh->prepare("
                      select * from comments
                       where postid=? and commentid=? ");
        $query->execute ($postid, $commentid);
        my $result=$query->fetchrow_hashref;

        return $result->{comment};
}

sub output_post {
        my $postid= shift;

        my $query=$dbh->prepare("
                      select * from posts
                       where postid=?");
        $query->execute ($postid);
        my $result=$query->fetchrow_hashref;
        return "$result->{title}\n$result->{why}";
}


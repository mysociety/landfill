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
use mysociety::NotApathetic::Routines;
my %Passed_Values;

my $site_name= $mysociety::NotApathetic::Config::site_name;         # database password
my $url_prefix= $mysociety::NotApathetic::Config::url;
my $admin_url_prefix= $mysociety::NotApathetic::Config::admin_url;
my $abuse_address= $mysociety::NotApathetic::Config::abuse_address;

{
	&setup_db;
        foreach my $param (param()) {
		$Passed_Values{$param}=param($param);
		$Passed_Values{$param}=~ s#\n# #gsi;
		if ($param ne 'email') {
			$Passed_Values{$param}=~ s#@# at #g;
		}
	}

	unless (Email::Valid->address( -address => $Passed_Values{email})) {
		&die_cleanly("Email address invalid");
	}

	$Passed_Values{body}= wrap("","    ", $Passed_Values{'body'});
	&store_report;
	&send_email;
	print "Location: $url_prefix/abuse/sent/\r\n\r\n";
}


sub store_report {
    my $postid= param('postid') || return;
    my $commentid= param('commentid') || '';


    $dbh->do("insert into abusereports set
		postid=?,
    		commentid=?,
		reporter_name=?, 
		reporter_ip=?, 
		email=?,
		message=?,
		modified=now(),
		added=now(),
		open=1,
		site='$site_name'
	", undef, $postid, $commentid, $Passed_Values{name}, $ENV{REMOTE_ADDR},
		  $Passed_Values{email}, $Passed_Values{body});

}

# now send email abuse summary

sub send_email {
    use Mail::Mailer;
    my $mailer= new Mail::Mailer 'sendmail';#, Server => 'mailrouter.mcc.ac.uk';
    my %headers;
    my $address      = $abuse_address;
    my $name         = "$site_name abuse report";
    my $query= $dbh->prepare("select * from abusereports where open=1 and site='$site_name' group by postid, commentid desc");
    $query->execute();


    $headers{"Subject"}= "$admin_url_prefix abuse reports" ;
    $headers{"To"}= "$name <$address>" ;
    $headers{'From'}= "$site_name <$abuse_address>" ;

    $mailer->open(\%headers);
    while (my $row= $query->fetchrow_hashref) {

    	my $about;
        if ($row->{'commentid'}) {
                $about=&output_comment($row->{'postid'},$row->{'commentid'});
        } else {
                $about=&output_post($row->{'postid'},$row->{'commentid'});
	}

    	$about=wrap('',"    ", $about);
    	print $mailer <<EOmail;
$admin_url_prefix/comments.shtml?$row->{postid}#$row->{commentid}

	$row->{reporter_name}: $row->{reporter_email}

Posting:
	$about

Not abusive   : $admin_url_prefix/cgi-bin/closeabusereport.cgi?reportid=$row->{'reportid'}
Delete comment: $admin_url_prefix/cgi-bin/hide.cgi?postid=$row->{'postid'};commentid=$row->{'commentid'}

==============

EOmail
    }

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


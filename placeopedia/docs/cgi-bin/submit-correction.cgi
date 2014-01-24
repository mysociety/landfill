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
my $email_domain= mySociety::Config::get('EMAIL_DOMAIN');

{
    foreach my $param (param()) {
        $Passed_Values{$param}=param($param);
        $Passed_Values{$param}=~ s#\n# #gsi;
    }
    $Passed_Values{name} ||= '';
    $Passed_Values{email} ||= '';
    $Passed_Values{explain} ||= '';
    &handle_comment;
}

sub error {
    my ($field,$error) = @_;
    print "<error field=\"$field\">$error</error>\n";
}

sub handle_comment {
    my %quoted;
    my $error = 0;

    return if (!$Passed_Values{id});

    print CGI->header('application/xml');
    print "<submission>\n";
    unless (Email::Valid->address( -address => $Passed_Values{email})) {
        $error = 1;
        error('email', 'Email address invalid');
    }

    my $scrubber= HTML::Scrubber->new();
    $scrubber->allow(qw[a em strong p br]);
    $scrubber->comment(0);

    if (!$error) {
        foreach my $pv (keys %Passed_Values) {
            $Passed_Values{$pv}= $scrubber->scrub($Passed_Values{$pv});
            $quoted{$pv}= $dbh->quote($Passed_Values{$pv});
        }
        my $query;
        if (defined($quoted{lat}) && defined($quoted{lng}) && defined($quoted{zoom})) {
            $query = $dbh->prepare("INSERT INTO incorrect
            (post_id, name, email, reason, lat, lon, zoom) VALUES
            ($quoted{id}, $quoted{name}, $quoted{email}, $quoted{explain}, $quoted{lat}, $quoted{lng}, $quoted{zoom})");
            $query->execute;
        } else {
            $query = $dbh->prepare("INSERT INTO incorrect
            (post_id, name, email, reason) VALUES
            ($quoted{id}, $quoted{name}, $quoted{email}, $quoted{explain})");
            $query->execute;
        }
        $query = $dbh->prepare("update posts set hidden=1 where postid=$quoted{id}");
        $query->execute;
#       &send_email;
    }
    print "</submission>\n";
}

sub send_email {
    use Mail::Mailer;
    my $mailer= new Mail::Mailer 'sendmail';
    my %headers;
    my $address      = $Passed_Values{"email"};
    my $name         = $Passed_Values{"name"} || '';

    $headers{"Subject"}= "Incorrect place $site_name";
    $headers{"To"}= "\"$site_name\" <matthew\@mysociety.org>";
    $headers{'From'}= $headers{'To'};
    $headers{'Content-Type'} = 'text/plain; charset=utf-8';
    $headers{"X-Originating-IP"}= $ENV{'REMOTE_ADDR'} || return;


    # XXX lng, lat, zoom and name aren't validated. 
    # this isn't currently called, but just in case
    return(0); 


    $mailer->open(\%headers);


    print $mailer <<EOmail;

Someone thinks post $Passed_Values{id} is in the incorrect location.

Name  :  $name
Email :  $address
New location: $Passed_Values{lng}, $Passed_Values{lat}, $Passed_Values{zoom}

EOmail
    $mailer->close;
    return;
}


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
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 1});
my $url_prefix= $mysociety::NotApathetic::Config::url;
my $site_name= $mysociety::NotApathetic::Config::site_name;
my $email_domain= $mysociety::NotApathetic::Config::email_domain; 
my %Passed_Values;

{
	foreach my $param (param()) {
		$Passed_Values{$param}=param($param);
	}
	$Passed_Values{title} ||= '';
	$Passed_Values{name} ||= 'Unknown';
	$Passed_Values{region} ||= '';
	$Passed_Values{location} ||= '';
	$Passed_Values{category} ||= 'general';
	($Passed_Values{google_lat})= $Passed_Values{location} =~ m#lat=([-\.\d]+)#;
	($Passed_Values{google_long})= $Passed_Values{location} =~ m#long=([-\.\d]+)#;
print STDERR "lat: $Passed_Values{google_lat}\n";
print STDERR "long: $Passed_Values{google_long}\n";
	&handle_comment;
	&send_email();

	print "Location: $url_prefix/new/checkemail/\r\n\r\n";
}


sub handle_comment {
	my %quoted;

	my $scrubber= HTML::Scrubber->new();
	$scrubber->allow(qw[em strong p br]);
	$scrubber->allow('a' => 'href');
	$scrubber->comment(0);

	foreach my $pv (keys %Passed_Values) {
		$Passed_Values{$pv}= $scrubber->scrub($Passed_Values{$pv});
		$quoted{$pv}= $dbh->quote($Passed_Values{$pv});
	}

	my $randomness = rand(); $randomness=~ s/^0\.(\d+)/$1/;
	my $auth_code_q= $dbh->quote($randomness);
        my $email_alert=0;
        $email_alert=1 if (defined $Passed_Values{'emailalert'});


	my $query=$dbh->prepare("
		update posts
		   set title=$quoted{title} ,
		       name=$quoted{name},
		       emailalert=$email_alert,
		       posted=now(),
		       category=$quoted{category},
		       original_geography=$quoted{location},
		       google_lat=$quoted{google_lat},
		       google_long=$quoted{google_long}
		 where postid=$quoted{postid}
  		   and authcode=$quoted{authcode}
		   and site='$site_name'
	");

	$query->execute;


        $query= $dbh->prepare("select email from posts where postid=$quoted{postid} and authcode=$quoted{authcode}");
        $query->execute();
        ($Passed_Values{"email"})= $query->fetchrow_array();

}


sub send_email {
    use Mail::Mailer;
    my $mailer= new Mail::Mailer 'sendmail';#, Server => 'mailrouter.mcc.ac.uk';
    my %headers;
    my $address      = $Passed_Values{"email"} || 'nobody' . $email_domain;
    my $name         = $Passed_Values{"name"} || '';

    $headers{"Subject"}= "Request to post to $site_name";
    $headers{"To"}= "$name <$address>" ;
    $headers{'From'}= "$site_name <team$email_domain>";
    $headers{"X-Originating-IP"}= $ENV{'HTTP_X_FORWARDED_FOR'}  || $ENV{'REMOTE_ADDR'} || return;
    $mailer->open(\%headers);


    print $mailer <<EOmail;

Hi,

Someone has tried to post to $site_name using this address
on the topic of $Passed_Values{title}

If it wasn't you, just ignore it.

If this was you, and you wish to confirm that post, please click on
the following link
        $url_prefix/cgi-bin/new-confirm.cgi?u=$Passed_Values{postid}&c=$Passed_Values{authcode}

Thank you
$site_name

EOmail

    $mailer->close;

    return;
}


sub die_cleanly {
        &mysociety::NotApathetic::Config::die_cleanly(@_);

}

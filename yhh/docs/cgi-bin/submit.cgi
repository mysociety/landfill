#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use HTML::Entities;
use HTML::Scrubber;
use Email::Valid;
use CGI qw/param/;
use mysociety::NotApathetic::Config;

if ($mysociety::NotApathetic::Config::site_open_for_additions == 0) {
    print "Location: $mysociety::NotApathetic::Config::url\n\n";
    exit(0);
}

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
	$Passed_Values{email} ||= '';
	$Passed_Values{title} ||= '';
	$Passed_Values{body} ||= '';
	$Passed_Values{summary} ||= '';
	$Passed_Values{name} ||= 'Unknown';
	$Passed_Values{category} ||= 'general';

	&handle_comment;
}

sub error {
    my ($field, $error) = @_;
    print "<error field=\"$field\">$error</error>\n";
}

sub handle_comment {
    my %quoted;
    my $error = 0;
    print CGI->header('application/xml');
    print "<submission>\n";
    unless (Email::Valid->address( -address => $Passed_Values{email})) {
        $error = 1;
        error('email', 'Email address invalid');
    }

    my $scrubber= HTML::Scrubber->new();
    $scrubber->allow(qw[em strong p br]);
    $scrubber->rules('a' => { href => 1 });
    $scrubber->comment(0);

    if (!$error) {
	foreach my $pv (keys %Passed_Values) {
	    $Passed_Values{$pv}= $scrubber->scrub($Passed_Values{$pv});
            $Passed_Values{$pv} =~ s/[\x80-\x9f]//g;
	    $quoted{$pv}= $dbh->quote($Passed_Values{$pv});
	}

        my $randomness = rand(); $randomness=~ s/^0\.(\d+)/$1/;
	$Passed_Values{authcode}= $randomness;
	my $auth_code_q= $dbh->quote($randomness);

        my $email_alert=0;
        $email_alert=1 if (defined $Passed_Values{'emailalert'});

	my $query=$dbh->prepare("
		insert into posts
		   set email=$quoted{email} ,
		       why=$quoted{body} ,
		       shortwhy=$quoted{summary} ,
		       title=$quoted{title} ,
		       posted=now(),
		       authcode=$auth_code_q,
		       name=$quoted{name},
		       emailalert=$email_alert,
		       category=$quoted{category},
		       google_lat=$quoted{lat},
		       google_long=$quoted{lng},
                       google_zoom=$quoted{zoom},
		       site='$site_name'
	");
	$query->execute;
	$Passed_Values{rowid}= $dbh->{insertid};
	&send_email;
    }
    print "</submission>\n";
}

sub send_email {
    use Mail::Mailer;
    my $mailer= new Mail::Mailer 'sendmail';
    my %headers;
    my $address      = $Passed_Values{"email"};
    #my $name         = $Passed_Values{"name"} || '';

    $headers{"Subject"}= "Request to post to $site_name";
    $headers{"To"}= "$address" ;
    $headers{'From'}= "$site_name <team$email_domain>";
    #$headers{"X-Originating-IP"}= $ENV{'HTTP_X_FORWARDED_FOR'}  || $ENV{'REMOTE_ADDR'} || return;
    $mailer->open(\%headers);
    print $mailer <<EOmail;

Hi,

Someone has tried to post to $site_name using this address
on the topic of $Passed_Values{title}

If it wasn't you, just ignore it.

If this was you, and you wish to confirm that post, please click on
the following link
        $url_prefix/cgi-bin/new-confirm.cgi?u=$Passed_Values{rowid};c=$Passed_Values{authcode}

Thank you
$site_name

EOmail
    $mailer->close;
    return;
}

sub die_cleanly {
    &mysociety::NotApathetic::Config::die_cleanly(@_);
}

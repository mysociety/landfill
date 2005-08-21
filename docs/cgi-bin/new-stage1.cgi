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
	$Passed_Values{title} = $Passed_Values{"q"};
	$Passed_Values{why} ||= ''; # need to pull this from the DB
	($Passed_Values{google_lat})= $Passed_Values{location} =~ m#lat=(-?\d+\.\d+);#;
	($Passed_Values{google_long})= $Passed_Values{location} =~ m#long=(-?\d+\.\d+)#;

	&handle_comment;

	&send_email;
	print "Location: $url_prefix/new/checkemail/\r\n\r\n";
}


sub handle_comment {
	my %quoted;
	unless (Email::Valid->address( -address => $Passed_Values{email})) {
		&die_cleanly("Email address invalid");
	}


	my $scrubber= HTML::Scrubber->new();
	$scrubber->allow(qw[a em strong p br]);
	$scrubber->comment(0);

	my $query= $dbh->prepare ("select cur_text from cur where cur_title like ? limit 1");
	$query->execute("$Passed_Values{'title'}%");
	my $cur_text;
	($cur_text)= $query->fetchrow_array;

 	$cur_text =~ /^(.{75}.*?\.)/;
	$Passed_Values{shortwhy}=  $cur_text;

	foreach my $pv (keys %Passed_Values) {
		$Passed_Values{$pv}= $scrubber->scrub($Passed_Values{$pv});
                $Passed_Values{$pv} =~ s/[\x80-\x9f]//g;
		$quoted{$pv}= $dbh->quote($Passed_Values{$pv});
	}


        #unless ($Passed_Values{why} && $Passed_Values{why} ne 'Write your reasons here...') {
        #&die_cleanly("Please give a reason.");
        #}

        my $randomness = rand(); $randomness=~ s/^0\.(\d+)/$1/;
	$Passed_Values{authcode}= $randomness;
	my $auth_code_q= $dbh->quote($randomness);
	$quoted{"title"} =~ s#_# #g;


	$query=$dbh->prepare("
		insert into posts
		   set email=$quoted{email} ,
		       title=$quoted{title} ,
		       posted=now(),
		       name=$quoted{name},
		       google_lat=$quoted{google_lat},
		       google_long=$quoted{google_long},
		       authcode=$auth_code_q,
		       site='$site_name'
	");

	$query->execute;
	$Passed_Values{rowid}= $dbh->{insertid};

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

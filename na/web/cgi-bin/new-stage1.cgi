#!/usr/bin/perl


use warnings;
use strict;
use DBI;
use HTML::Entities;
use HTML::Scrubber;
use Email::Valid;
use CGI qw/param/;
use mysociety::NotApathetic::Config;
use mysociety::NotApathetic::Routines;

if ($mysociety::NotApathetic::Config::site_open_for_additions == 0) {
    print "Location: $mysociety::NotApathetic::Config::url\n\n";
    exit(0);
}



my $url_prefix= $mysociety::NotApathetic::Config::url;
my $site_name= $mysociety::NotApathetic::Config::site_name;
my $email_domain= $mysociety::NotApathetic::Config::email_domain; 
my %Passed_Values;

{
	foreach my $param (param()) {
		$Passed_Values{$param}=param($param);
	}
	&setup_db();

	$Passed_Values{email} ||= '';
	$Passed_Values{title} ||= '';
	$Passed_Values{why} ||= '';

	&handle_comment;

	&send_email;
	print "Location: $url_prefix/new/checkemail/\r\n\r\n";
}


sub handle_comment {
	my %quoted;

	my $scrubber= HTML::Scrubber->new();
	$scrubber->allow(qw[a em strong p br]);
	$scrubber->comment(0);

	foreach my $pv (keys %Passed_Values) {
		$Passed_Values{$pv}= $scrubber->scrub($Passed_Values{$pv});
                $Passed_Values{$pv} =~ s/[\x80-\x9f]//g;
		if ($pv ne 'email') {
			$Passed_Values{$pv}=~s#@# at #g;
		}
		$quoted{$pv}= $dbh->quote($Passed_Values{$pv});
	}

	unless ( Email::Valid->address($Passed_Values{'email'})) {
            &die_cleanly("invalid email address." . $Passed_Values{'email'});
	}


        unless ($Passed_Values{why} && $Passed_Values{why} ne 'Write your reasons here...') {
            &die_cleanly("Please give a reason.");
        }

        my $randomness = rand(); $randomness=~ s/^0\.(\d+)/$1/;
	$Passed_Values{authcode}= $randomness;
	my $auth_code_q= $dbh->quote($randomness);

        if ($Passed_Values{why} =~ m/^(.{210}.+?\b)/s) {
                $quoted{shortwhy}= $dbh->quote($1 . "...");
        } else  {
                $quoted{shortwhy}= $quoted{why};
        }

        if ($Passed_Values{why} =~ m/^(.{35}.+?\b)/s) {
                $quoted{title}= $dbh->quote($1 . "...");
        } else  {
                $quoted{title}= $quoted{why};
        }

        $Passed_Values{title} = $quoted{title};


	my $query=$dbh->prepare("
		insert into posts
		   set email=$quoted{email} ,
		       why=$quoted{why} ,
		       shortwhy=$quoted{shortwhy} ,
		       title=$quoted{title} ,
		       posted=now(),
		       site='$site_name',
		       authcode=$auth_code_q
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
        $url_prefix/mc/$Passed_Values{rowid}/$Passed_Values{authcode}

Thank you
$site_name

EOmail

    $mailer->close;

    return;
}

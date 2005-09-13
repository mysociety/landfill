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
use LWP::Simple qw($ua);
use PoP;
use HTML::Entities;
use HTML::Scrubber;
use Email::Valid;
use CGI qw/param/;

$ua->agent("Placeopedia, checking for article");

my $url_prefix= mySociety::Config::get('URL');
my $site_name= mySociety::Config::get('SITE_NAME');
my $email_domain= mySociety::Config::get('EMAIL_DOMAIN');
my %Passed_Values;

{
	foreach my $param (param()) {
		$Passed_Values{$param}=param($param);
	}
	$Passed_Values{email} ||= '';
	$Passed_Values{title} ||= '';

	&handle_comment;
#	print "Location: $url_prefix/new/checkemail/\r\n\r\n";
}

sub error {
    my ($field,$error) = @_;
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
    $scrubber->allow(qw[a em strong p br]);
    $scrubber->comment(0);

    my $wikititle = $Passed_Values{'title'};
    $wikititle =~ s/ /_/g;
    my $query= $dbh->prepare ("select cur_text from cur where cur_title = ? and cur_namespace = 0 and cur_is_redirect = 0 limit 1");
    $query->execute($wikititle);
    my($cur_text) = $query->fetchrow_array;
    if (!$cur_text) {
        # Try grabbing the page from Wikipedia instead.
        my $t = $wikititle;
        $t =~ s/ /_/g;
        $t =~ s/([^A-Za-z0-9_])/sprintf('%%%02x', ord($1))/ge;
        my $url = "http://en.wikipedia.org/wiki/$t";
        eval {
            local $SIG{ALRM} = sub { die "alarm\n"; };
            alarm(30);
            my @x = LWP::Simple::head($url);
            alarm(0);
            if (!@x) {
                $error = 1;
                error('q', "Wikipedia article doesn't exist");
            }
        };

        if ($@) {
            die "$@ in eval" unless ($@) eq "alarm\n";
            $error = 1;
            error('q', "Timed out looking up article title on Wikipedia");
        }
    }
#    $Passed_Values{why} = $cur_text;
 
    if (!$error) {
        foreach my $pv (keys %Passed_Values) {
            $Passed_Values{$pv}= $scrubber->scrub($Passed_Values{$pv});
            $Passed_Values{$pv} =~ s/[\x80-\x9f]//g;
            $quoted{$pv}= $dbh->quote($Passed_Values{$pv});
        }

        my $randomness = rand(); $randomness=~ s/^0\.(\d+)/$1/;
        $Passed_Values{authcode}= $randomness;
        my $auth_code_q= $dbh->quote($randomness);
        $quoted{"title"} =~ s#_# #g;
        my $query=$dbh->prepare("
		insert into posts
		   set email=$quoted{email},
		       title=$quoted{title},
		       why='',
		       posted=now(),
		       name=$quoted{name},
		       google_lat=$quoted{lat},
		       google_long=$quoted{lng},
                       google_zoom=$quoted{zoom},
		       authcode=$auth_code_q,
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

sub fetch_wikipedia {
use HTTP::Request;
use LWP::UserAgent;
use HTML::Scrubber;
         my $topic= shift;
         my $scrubber= HTML::Scrubber->new(allow => []);
         my $ua = LWP::UserAgent->new;
         $ua->agent("Placeopedia.com");
         my $req = HTTP::Request->new(GET => 'http://en.wikipedia.org/wiki/Special:Export/'.$topic);
         $req->header('Accept' => 'application/xml');

         # send request
         my $res = $ua->request($req);
         my $snippet;
         # check the outcome
         if ($res->is_success) {
            my $content= $res->decoded_content;
            $content=~ s#[^M\n]# #mgi;
            ($snippet) = $content =~ m#<text xml:space="preserve">(.*?)</text>#s;
            $snippet= $scrubber->scrub($snippet);
            my ($last)= $snippet=~ /^(.{1000}.*?\.)/;
		print STDERR $snippet;
            return ($last);
         }
         else {
            return '';
         }
}

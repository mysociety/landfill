#!/usr/bin/perl

use warnings;
use strict;
use DBI;
use Text::Wrap;
use Mail::Mailer qw(sendmail);
$Text::Wrap::columns = 65;

my $dsn = 'DBI:mysql:notapathetic:localhost'; # DSN connection string
my $db_username= 'notapathetic';              # database username
my $db_password= '';         # database password
my $dbh=DBI->connect($dsn, $db_username, $db_password, {RaiseError => 0});
my %State; # State variables during display.
my %Passed_Values;
my $search_term;
{
        my $query=$dbh->prepare ("select * from emailnotify where validated=1 and cancelled=0 ");

        $query->execute;


	while (my $result=$query->fetchrow_hashref) {
		&send_email($result);
	}
}

sub send_email {
	my $details=shift;
	my $limiter=&handle_search_term($details->{search});
	my $query=$dbh->prepare("
		 select * from posts
		  where posted >= date_sub(now(), interval 1 day)
		    and hidden=0
		    and validated=1
		        $limiter
		   ");
	$query->execute;

	if ($query->rows == 0 ) {
		return(); # nothing to email, so don't.
	}
	

	my $limiter_text='.';
	my $mailer= new Mail::Mailer 'sendmail';
        my $to_person         = $details->{"email"} ;
	my %headers; 

        $headers{'To'}= "$to_person" ;
        $headers{"From"}= "Notapathetic.com <donotreply\@notapathetic.com>" ;
        $headers{"Subject"}= "Yesterday on NotApathetic.com";
        $mailer->open(\%headers);

	if ($details->{search} ne '') {
		$limiter_text=" matching http://www.NotApathetic.com/?$details->{search}.\n";
	}


print $mailer <<EOfragment;

Yesterday on NotApathetic.com$limiter_text

EOfragment

	my $result;
	while ($result= $query->fetchrow_hashref) {
		my $desc= wrap('','       ', $result->{shortwhy});
		print $mailer <<EOfragment;
     $result->{title}
       $desc
       http://www.notapathetic.com/comments/$result->{postid}

EOfragment
	}


	print $mailer <<EOfooter;


To cancel your notifications, visit this URL:
  http://www.notapathetic.com/emailnotify/cancel?u=$details->{notifyid};c=$details->{authcode}

EOfooter
        $mailer->close;

	$dbh->do("update emailnotify set lastrun=now() where notifyid=$details->{notifyid}");
}


sub handle_search_term {
        my $search_path= shift || '';
        my @search_fields= ('posts.why', 'posts.title');
        return ('') if ($search_path eq '');

        my (@or)= split /\//, $search_path;

        my $limiter= ' and ( 1 = 0 '; # skipped by the optimiser but makes
                                # syntax a lot easier to get right

        foreach my $or (@or) {
                # a/b/c+d/e
                # a or b or (c and d) or e
                next if $or =~ /^\s*$/;
                my $or_q = $dbh->quote("\%$or\%");

                if ($or =~ /\+/) {
                        my @and= split /\+/, $or;
                        $limiter.= " or  ( 1 = 1 ";
                        foreach my $and (@and) {
                                my $and_q = $dbh->quote("\%$and\%");
                                $limiter.= " and ( 1=0 ";
                                foreach my $field (@search_fields) {
                                        $limiter.= " or $field like $and_q ";
                                }
                                $limiter.= " ) \n";
                        }
                        $limiter .= ' ) ';
                } else  {
                        $limiter .= " or ( 1=0 " ;
                                foreach my $field (@search_fields) {
                                        $limiter.= " or $field like $or_q ";
                                }
                        $limiter .= ' ) ';
                }

        }
        $limiter .= " )\n\n ";

        #print STDERR $limiter;
        return $limiter;

}



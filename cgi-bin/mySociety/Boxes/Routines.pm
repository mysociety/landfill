#!/usr/bin/perl

package mySociety::Boxes::Routines;
require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(add_feed_to_box generate_rss_feed generate_box_style_richard2 get_constituencyid_from_postcode);  # symbols to export on request
my $dbh= $main::dbh;

sub get_constituencyid_from_postcode {
	# need to call the mysoc API here.

	return (364); # manchester gorton
}


sub add_feed_to_box {
	my ($boxid, $feed, $tag)=@_; # feed could be an url or a numeric feedid
	my $feedid;
	$tag||='';
	if ($feed =~ m/^\d+$/) {
		$feedid=$feed;
	} else { # text - so it may or may not already exist
		my $tag=$feed;
		my $query= $dbh->prepare("select feedid from feeds where feedurl=?");
		$query->execute($feed);
		my $result;
		if ($result= $query->fetchrow_hashref) {
			$feedid=$result->{feedid};
		} else {
			$dbh->do("insert into feeds set feedurl=?, tag=?", undef, $feed, $tag);
			$feedid= $dbh->{mysql_insertid};
		}
	}
	$dbh->do("insert into boxcontents set boxid=?, feedid=?", undef, $boxid, $feedid);
}


sub generate_box_style_richard2 {
        my $boxid = shift;
	my $style='richard2';
	my $result;
	my $lasttag='';
	my $count=0;
	my $morelink='';

	print (&get_style_section('richard2', 'header'));

	foreach my $result (&get_items($boxid) ) {
		if ($result->{tag} ne $lasttag) { 
			if ($lasttag ne '') { # we don't want end stuff at the top
				if ($morelink ne '') { # link from the rss feed
					print "<li class=\"mysociety_yp_morelink\" target=\"_new\"><a href=\"$morelink\">more &raquo; <small>$lasttag</small></a></li>\n";
				}
				print "</ul>\n";
			}

			if ($last->{'tag'} eq '') {
				$count=0;

				if ($result->{tag} eq 'theyworkforyou') { $result->{feedtitle}= 'What my MP has been up to'; }
				if ($result->{tag} eq 'pledgebank') { $result->{feedtitle}= 'Pledges set up by my neighbours'; }
				print "<h4 class=\"mysociety_yp_$result->{tag}\">$result->{feedtitle}</h4>\n";
				print "<ul>\n";
			} else {
				print "<li class=\"mysociety_yp_morelink\" target=\"_new\"><a href=\"$morelink\">more &raquo; <small>$lasttag</small></a></li>\n";
				last;
			}
		}

		next if ($count ==5); $count++;
		last if $result->{'last'};
		print "<li><a target=\"_new\" href=\"$result->{link}\">$result->{title}</a></li>\n";

		$lasttag=$result->{tag};
		$morelink=$result->{morelink};
	}
	print "</ul>\n";

	print (&get_style_section('richard2', 'footer'));
} 

sub get_style_section {
	my ($style,$section)= @_;

	my $query=$dbh->prepare ("select content from styleinfo where style=? and place=?");
	$query->execute($style,$section);

	return ($query->fetchrow_array());
}



sub get_items {
	my $boxid= shift;
        my $entries_query= $dbh->prepare("select entries.title, entries.content, entries.shortcontent, entries.link, feeds.tag,
						 feedinfo.title as feedtitle, feedinfo.siteurl as morelink
	                                    from boxcontents, entries, feeds, feedinfo
					   where boxcontents.boxid=?
					     and entries.feedid=boxcontents.feedid
					     and entries.feedid=feeds.feedid
					     and entries.feedid=feedinfo.feedid
					order by feeds.tag, boxcontents.rowid,
					entries.entryid desc");
        $entries_query->execute($boxid) || die $dbh->errstr;


	while ($result =$entries_query->fetchrow_hashref) {
		my %entry=%{$result};
		if ($entry{'tag'} eq 'theyworkforyou') {
			$entry{'title'} =~ s#Written Answers - #Asked #;
			$entry{'title'} =~ s#Commons debates - Oral Answers to Questions - #Received an answer on #;
			$entry{'title'} =~ s#Commons Debates - #Spoke about #;
		}
		push @entries, \%entry;
	}

	my $last;
	$last->{'last'}='last';
	$last->{'tag'}='last';
	push @entries, $last;
	return (@entries)
}



sub generate_rss_feed {
	my $boxid=shift;
	use XML::RSS;
 	my $rss = new XML::RSS (version => '2');
 	$rss->channel(
   title        => "YourSociety mySociety.org box",
   link         => $about_url_prefix . $boxid,
   description  => "YourSociety mySociety.org box",
   dc => {
     subject    => "YourSociety mySociety.org box",
     creator    => 'boxes@mysociety.org',
     publisher  => 'boxes@mysociety.org',
     language   => 'en-gb',
     ttl        =>  600
   },
   syn => {
     updatePeriod     => "hourly",
     updateFrequency  => "1",
     updateBase       => "1901-01-01T00:00+00:00",
   },
);

	foreach my $result (&get_items($boxid) ) {
	    last if defined $result->{'last'};
            $rss->add_item(title => "$result->{title}",
                           link => "$result->{link}",
                           description=> "$result->{content}",
                           dc =>{ category =>"$result->{tag}"}
			  );

        }

        print $rss->as_string;
}


1;






#!/usr/bin/perl

BEGIN {push @INC, $ENV{PANOPTICON_PATH}.'/bin/'}

use warnings;
use strict;
use HTML::CalendarMonthSimple;
use PanopticonConfig;
my %seen;
my $query= $dbh->prepare('select date_format(first_seen, "%Y-%m-%d") as seen, count(first_seen) as count from entries group by seen');
$query->execute();
my $index=1;
my %months;
$months{"January"}='01';
$months{"February"}='02';
$months{"March"}='03';
$months{"April"}='04';
$months{"May"}='05';
$months{"June"}='06';
$months{"July"}='07';
$months{"August"}='08';
$months{"September"}='09';
$months{"October"}='10';
$months{"November"}='11';
$months{"December"}='12';

{
	my $row;
	while ($row= $query->fetchrow_hashref) {
		my ($yearmonth, $day)= $row->{seen} =~ (/^(\d{4}-\d{2})-(\d{2})/);
		$seen{$yearmonth}{$day}=$row->{count};
	}

print <<EOtable;
	<table border="0">
		<tr valign="top">
			<td>
EOtable

	foreach my $yearmonth (reverse sort keys %seen) {
		my ($year, $month)= $yearmonth =~ m#(\d{4})-(\d{2})#;
		my $cal = new HTML::CalendarMonthSimple(year=>$year, month=>$month);
		$cal->weekstartsonmonday(1);
		$cal->border(1);
		$cal->weekdays(qw[M T W T F]);
		$cal->cellalignment('center');
		$cal->sunday('S');
		$cal->saturday('S');
		my($y,$m) = ( $cal->year() , $cal->monthname() );
		$cal->header("<center><h4><a href=\"../history/$y-$months{$m}\">$m $y</a></h4></center>\n\n");


		foreach my $day (keys %{$seen{$yearmonth}}) {
			$cal->setdatehref($day, "../history/$yearmonth-$day");
		}

		print $cal->as_HTML;

		if ($index == 3) {
			$index=1;
			print "</td></tr>\n\n<tr valign=\"top\"><td>\n\n";
		} else  {
			print "</td>\n\n<td>\n\n";
			$index++;
		}
	}
	print "</td>\n\n</tr></table>\n\n";
}

#!/usr/bin/perl -w

package Track::Stats;

use strict;
use FindBin;

use mySociety::Config;
BEGIN {
    mySociety::Config::set_file("$FindBin::Bin/../../track/conf/general");
}
use mySociety::DBHandle qw(select_all);
use Track;

my %periods = (
    'week' => "and whenlogged >= current_timestamp-'7 days'::interval",
    'month' => "and whenlogged >= current_timestamp-'28 days'::interval",
);

my %sites = (
    'writetothem' => ['hfymp','fms','gny','twfy_alerts','cheltenhamhfyc','twfy','pb'],
    'fixmystreet' => ['gny','hfymp','twfy_alerts','cheltenhamhfyc','pb'],
    'theyworkforyou' => ['hfymp','fms','gny','twfy-alert-word', 'twfy-alert-person','cheltenhamhfyc','pb'],
    'hearfromyourmp' => ['twfy', 'fms', 'pb'],
);

my %adverts = (
    'cheltenhamhfyc' => ['subscribed=1%', 'hearfromyourcouncillor'],
    'twfy_alerts' => ['%advert=%', 'theyworkforyou'],
    'twfy' => ['%advert=%', 'theyworkforyou'],
    'hfymp' => ['subscribed=1%', 'hearfromyourmp'],
    'gny' => ['added-group', 'groupsnearyou'],
    'fms' => ['added-problem', 'fixmystreet'],
    'twfy-alert-person' => ['%advert=%', 'theyworkforyou'],
    'twfy-alert-word' => ['%advert=%', 'theyworkforyou'],
    'pb' => ['NOTUSED', 'pledgebank'],
);

sub generate {
    my %out;
    foreach my $period (keys %periods) {
        my $period_sql = $periods{$period};
        foreach my $site (keys %sites) {
            my $ads = $sites{$site};
            foreach my $ad (@$ads) {
                my $action = $adverts{$ad};
                my $d = select_all("select min(whenlogged) as first, max(whenlogged) as last,
                    extradata, count(distinct(tracking_id)),
                    (select extradata from event as e
                    where e.tracking_id=event.tracking_id
                        and e.extradata like '$action->[0]'
                        and e.url like '%$action->[1]%'
                        and e.whenlogged > event.whenlogged
                    order by e.whenlogged desc limit 1) as t
                from event
                where url like '%$site%'
                    and extradata like '%advert=$ad%'
                    $period_sql
                    and tracking_id != 294273 -- ME!
                group by extradata,t
                order by extradata,t");
                foreach my $row (@$d) {
                    my $converted = $row->{t} ? 1 : 0;
                    (my $extradata = $row->{extradata}) =~ s#^from_[a-z]+=1; ##;
                    $extradata =~ s#^subscribed=1; ##;
                    (my $first = $row->{first}) =~ s#\..*##;
                    if (!$out{$site}{$extradata}{$period}{first} || $first lt $out{$site}{$extradata}{$period}{first}) {
                        $out{$site}{$extradata}{$period}{first} = $first;
                    }
                    (my $last = $row->{last}) =~ s#\..*##;
                    if (!$out{$site}{$extradata}{$period}{last} || $last gt $out{$site}{$extradata}{$period}{last}) {
                        $out{$site}{$extradata}{$period}{last} = $last;
                    }
                    $out{$site}{$extradata}{$period}{$converted} += $row->{count};
                }
            }
        }
    }
    return wantarray ? %out : \%out;
}

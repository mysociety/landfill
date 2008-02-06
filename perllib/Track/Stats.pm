#!/usr/bin/perl -w

package Track::Stats;

use strict;

use mySociety::Config;
BEGIN {
    mySociety::Config::set_file('../../track/conf/general');
}
use mySociety::DBHandle qw(select_all);
use Track;

my %periods = (
    'week' => "and whenlogged >= current_timestamp-'7 days'::interval",
    'month' => "and whenlogged >= current_timestamp-'28 days'::interval",
);

my %sites = (
    'writetothem' => ['hfymp','fms','gny','twfy_alerts','cheltenhamhfyc'],
    'fixmystreet' => ['gny','hfymp','twfy_alerts','cheltenhamhfyc'],
    'theyworkforyou' => ['hfymp','fms','gny','twfy-alert-word', 'twfy-alert-person','cheltenhamhfyc'],
);

my %adverts = (
    'cheltenhamhfyc' => ['subscribed=1%', 'hearfromyourcouncillor'],
    'twfy_alerts' => ['%advert=%', 'theyworkforyou'],
    'hfymp' => ['subscribed=1%', 'hearfromyourmp'],
    'gny' => ['added-group', 'groupsnearyou'],
    'fms' => ['added-problem', 'fixmystreet'],
    'twfy-alert-person' => ['%advert=%', 'theyworkforyou'],
    'twfy-alert-word' => ['%advert=%', 'theyworkforyou'],
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
                    (my $first = $row->{first}) =~ s#\..*##;
                    if (!$out{$period}{$site}{$extradata}{first} || $first lt $out{$period}{$site}{$extradata}{first}) {
                        $out{$period}{$site}{$extradata}{first} = $first;
                    }
                    (my $last = $row->{last}) =~ s#\..*##;
                    if (!$out{$period}{$site}{$extradata}{last} || $last gt $out{$period}{$site}{$extradata}{last}) {
                        $out{$period}{$site}{$extradata}{last} = $last;
                    }
                    $out{$period}{$site}{$extradata}{$converted} += $row->{count};
                }
            }
        }
    }
    return wantarray ? %out : \%out;
}

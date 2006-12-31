#!/usr/bin/perl

use warnings;
use strict;
use CGI qw/param/;
use Date::Manip;
use HTML::Entities;
use mysociety::NotApathetic::Config;
use mysociety::NotApathetic::Routines;


my %State; # State variables during display.
our $url_prefix=$mysociety::NotApathetic::Config::url;

my @stopwords = qw(the to i of a and in is for that not are have it be my they as on you will all with no but we because don't this do if their who there any would or i'm them me an by what so get at when one from am like than can which about has our out up was then should even your can't how some way into those it's very well its where had were may he these isn't such go dont i'd just i'll i've did didn't too far rather most won't wont doesn't vote voting party people parties system government);
# my @stopwords = qw();
my %stopwords;
for (@stopwords) { $stopwords{$_} = 1 }

{
    print "Content-Type: text/html; charset=UTF8\r\n\r\n";
            my $query=$dbh->prepare("
                          select *
                            from posts
                           where validated=1
			     and site='$site_name'
                             and hidden=0
                           ");

            $query->execute;
            my $result;
	    
            my ($text, @text, %words, %posts, $total, $posts);
            while ($result=$query->fetchrow_hashref) {
                my %done = ();
                $text = lc($result->{title}) . "\n" . lc($result->{why});
                $text =~ tr/\x92/'/;
                $text =~ s/[^a-z0-9-']/ /ig;
                $text =~ s/((^|\s)'|'(\s|$))/ /g;
                $text =~ s/\s-\s/ /g;
                $text =~ s/lib dem/libdem/g;
                @text = split /\s+/, $text;
                foreach (@text) {
                    if (!$stopwords{$_}) {
                        $words{$_}++;
                        $total++;
                        if (!$done{$_}) {
                            $posts{$_}++;
                            $done{$_} = 1;
                        }
                    }
                }
                $posts++;
            }

            print "<b>Total: $total words, in $posts posts</b><br>\n";
            my $size; my $count = 0;
            foreach (sort { param('fixed') ? $a cmp $b : int(rand(3))-1 } keys %words) {
                next unless $_;
                if (param('old')) {
                    $size = 4 + $words{$_} / $total * 625 * 15;
                } else {
                    $size = sqrt(2000000 * ($words{$_}/$total) / length($_));
                }
                if ($posts{$_}/$posts < 0.2 && $words{$_} >= $posts{$_}+5) {
                    print '<span style="font-size: '.$size.'px;"><a title="Occurs '.$words{$_}.' times, in '.$posts{$_}.' posts" href="/search/?'.$_.'">'.$_.'</a></span>'."\n";
                }
#                print $words{$_} . "\n";
#                last if ($count++ == 100);
            }
}

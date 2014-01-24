#!/usr/bin/perl

# BSD licensed. 
# copyright Sam Smith <S@mSmith.net> 2007

use warnings;
use strict;
use Spreadsheet::ParseExcel;
use XML::Simple;
use CGI qw/param/;
use CGI::Carp qw/fatalsToBrowser warningsToBrowser/;
use Data::Dumper;
use File::Temp qw/ :mktemp  /;

my $code_to_place;
my $data;
my $meta;
my $place_info;


{
        my $file;

        if (defined $ENV{"HTTP_HOST"} ){
		use LWP::Simple;
                my $url= param('excel_url') || &error_dir ("http://services.disruptiveproactivity.com/meinedata/errors/not-excel/?$url");

                &error_dir ("http://services.disruptiveproactivity.com/meinedata/errors/not-excel/?$url") if $url !~ m#^https?://#;

                print "Content-type: text/plain\n\n";
                my $spreadsheet= get($url);
                my $fh;
                ($fh, $file) = mkstemp( "/tmp/meinedata-tmpfileXXXXXXXXXXXXXXXXX" );
                print $fh $spreadsheet;
                close($fh);
                rename ($file, $file .'.xls');
                $file .= '.xls';
        } else  {
                $file= shift || die "usage: $0 file.xls\n";
        }

        my $excel = Spreadsheet::ParseExcel::Workbook->Parse($file);
	if ($excel->{_ParseAbort}) {
		die $excel->{_ParseAbort};
	}
		; # || error_die("couldn't parse $file");
#print "got here - $file\n\n"; exit;

        my $sheet= shift(@{$excel->{Worksheet}}); 
        $sheet->{MaxRow} ||= $sheet->{MinRow};
        foreach my $row (3 .. $sheet->{MaxRow}) {
                #warn "$sheet->{Cells}[$row][0]->{'Val'} -> $sheet->{Cells}[$row][1]->{'Val'}\n";
                $code_to_place->{$sheet->{Cells}[$row][1]->{'Val'}}->{'name'}= $sheet->{Cells}[$row][1]->{'Val'};
                $place_info->{$sheet->{Cells}[$row][1]->{'Val'}}->{'group'}= $sheet->{Cells}[$row][2]->{'Val'};
                $place_info->{$sheet->{Cells}[$row][1]->{'Val'}}->{'colour'}= $sheet->{Cells}[$row][3]->{'Val'};
                $place_info->{$sheet->{Cells}[$row][1]->{'Val'}}->{'pointsize'}= $sheet->{Cells}[$row][4]->{'Val'};
                $place_info->{$sheet->{Cells}[$row][1]->{'Val'}}->{'link'}= $sheet->{Cells}[$row][5]->{'Val'};
        }

        foreach $sheet (@{$excel->{Worksheet}}) { # first one already shifted
                #printf("Sheet: %s\n", $sheet->{Name});
                my $title_orig= $sheet->{Name};
                next if $title_orig =~ m#^variables#i;
                my $title= $title_orig;
                $title =~ s#[^a-z0-9]#_#ig;
                $sheet->{MaxRow} ||= $sheet->{MinRow};
                $meta->{'data'}->{$title}->{'title'}= $sheet->{Cells}[0][0]->{'Val'} || $title_orig;
                foreach my $row (3 .. $sheet->{MaxRow}) {
                        $sheet->{MaxCol} ||= $sheet->{MinCol};
                        foreach my $col (1 ..  $sheet->{MaxCol}) {
                                my $cell = $sheet->{Cells}[$row][$col];
                                next if not defined $cell->{'Val'};
                                next if $cell->{'Val'} eq '';

                                my $location= $sheet->{Cells}[$row][0]->{'Val'};
                                my $year= $sheet->{Cells}[2][$col]->{'Val'};

                                if (defined $code_to_place->{$location}) {
                                        $location=$code_to_place->{$location}->{'name'};
                                }

        #warn "$title -> $location -> $year -> $cell->{'Val'}\n";
                                $data->{$location}->{'data'}->{$title}->{'s'.$year}+= int($cell->{'Val'} + 0.5);
                                $meta->{'data'}->{$title}->{'sequences'}->{'s'.$year}=$year;

                                if ( not defined  $meta->{'data'}->{$title}->{'max'}) {
                                        $meta->{'data'}->{$title}->{'max'}=0;
                                }

                                if ( $data->{$location}->{'data'}->{$title}->{'s'.$year} > $meta->{'data'}->{$title}->{'max'}) {
                                        $meta->{'data'}->{$title}->{'max'}= int($data->{$location}->{'data'}->{$title}->{'s'.$year}+1);
                                }

                        }
                }
        }

        foreach $sheet (@{$excel->{Worksheet}}) { # first one already shifted
                #printf("Sheet: %s\n", $sheet->{Name});
                my $title= $sheet->{Name};
                next unless $title =~ m#^variables#i;
                #$meta->{'sheets'}->{$title}=$title;
                $sheet->{MaxRow} ||= $sheet->{MinRow};
                foreach my $row (3 .. $sheet->{MaxRow}) {
                        $sheet->{MaxCol} ||= $sheet->{MinCol};
                        foreach my $col (1 ..  $sheet->{MaxCol}) {
                                my $cell = $sheet->{Cells}[$row][$col];
                                next if not defined $cell->{'Val'};
                                next if $cell->{'Val'} eq '';

                                my $location= $sheet->{Cells}[$row][0]->{'Val'};
                                my $label_orig= $sheet->{Cells}[2][$col]->{'Val'};
                                my $label= $label_orig;

                                $label =~ s#[^a-z0-9]#_#ig;

                                if (defined $code_to_place->{$location}) {
                                        $location=$code_to_place->{$location}->{'name'};
                                }

        #warn "$title -> $location -> $year -> $cell->{'Val'}\n";
                                $data->{$location}->{'variables'}->{$label}+= $cell->{'Val'};

                                if ( not defined  $meta->{'data'}->{$label}->{'max'}) {
                                        $meta->{'data'}->{$label}->{'max'}=0;
                                }

                                if ( $data->{$location}->{'variables'}->{$label} > $meta->{'data'}->{$label}->{'max'}) {
                                        $meta->{'data'}->{$label}->{'max'}= $cell->{'Val'} +1;
                                }
                                $meta->{'data'}->{$label}->{'max'}= $cell->{'Val'} +1;
                                $meta->{'data'}->{$label}->{'title'}= $label_orig;

                        }
                }
        }
        $excel=undef;
        my $xml;
        foreach my $location (sort { lc($a) cmp lc($b) } keys %{$data}) {
                my $point;

                foreach my $title (keys %{$data->{$location}->{'data'}} ){
                        my $sequence;
                        $sequence= $data->{$location}->{'data'}->{$title};
                        #$sequence->{'name'}=$title;
                        push @{$point->{'sequences'}->{$title}}, $sequence;
                }
                foreach my $var (keys %{$data->{$location}->{'variables'}} ){
                        $point->{'variables'}->{$var} =$data->{$location}->{'variables'}->{$var};
                }
                $point->{'name'}=$location;
                $point->{'title'}=$location;
                $point->{'group'}=$place_info->{$location}->{'group'};
                $point->{'pointsize'}= $place_info->{$location}->{'pointsize'};
                $point->{'group'}=$place_info->{$location}->{'group'};
                $point->{'colour'}=$place_info->{$location}->{'colour'} || '000000';
                push @{$xml->{'points'}->{'point'}}, $point;
        }

        foreach my $var (keys %{$meta->{'data'}}) {
                my $variable;
                $variable->{'name'}=$var;
                $variable->{'title'}= $meta->{'data'}->{$var}->{'title'};
                $variable->{'for_matching'}=$var;
                $variable->{'min'}=0;
                $variable->{'unit'}=$meta->{'data'}->{$var}->{'unit'} || '';
                $variable->{'max'}=$meta->{'data'}->{$var}->{'max'};
                $variable->{'sequences'}= $meta->{'data'}->{$var}->{'sequences'};

                push @{$xml->{'variables'}->{'variable'}}, $variable;
        }

        print XMLout($xml, AttrIndent =>1, NoAttr=>1, RootName=>'iquango', XMLDecl=>1);

        if (defined $ENV{"HTTP_HOST"} ){
                unlink($file);
        }
}



sub error_die {
        my $message= shift;
print <<EOhtml;

<?xml version='1.0' standalone='yes'?>
<iquango>
        <kill>$message</kill>
</iquango>

EOhtml
	exit;
}

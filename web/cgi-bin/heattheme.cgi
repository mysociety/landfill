#!/usr/bin/perl

# this is an enhanced implementation of the heat theme code.

# Copyright Sam Smith
# BSD licensed

use warnings;
use strict;
use LWP::Simple;
use HTML::TagFilter;
use CGI qw/param escapeHTML/;
use Data::Dumper;
use HTML::Entities;
use Lingua::EN::Fathom;

my $url1= param('url') || '';
my $text1= param('text') || '';
my $url2= param('url2') || '';
my $text2='';
my %stopwords;
my $fathom = new Lingua::EN::Fathom;
my $tf = new HTML::TagFilter;
$tf->allow_tags({});

{
	if ($url1 ne '') { $text1=get($url1); }
	if ($url2 ne '') { $text2= get($url2); }

	&process_stopwords('stop_words.txt');

	print "Content-Type: text/html\n\n";

	print &header;

	print escapeHTML($url1);
	&process_text($text1);

	if ($text2 ne '') {
		print escapeHTML($url2);
		&process_text($text2);
	}

	print &footer;

}

sub process_text {
	my $text= shift;
	my %words;

	$text= $tf->filter($text);
	$text= decode_entities($text);
	my $w;
	foreach my $word (split /[\(\)\[\]\s,\.\n]/, $text) {
		$w= lc($word);
		next if $w eq '';
		next if defined $stopwords{$w};
		next if $w !~ m#...#i;
		$words{$w}++;
	}

	my @order= reverse sort { $words{$a} <=> $words{$b} } keys %words ;
        $fathom->analyse_block($text);

        my $num_chars             = $fathom->num_chars;
        my $num_words             = $fathom->num_words;
        my $percent_complex_words = $fathom->percent_complex_words;
        my $num_sentences         = $fathom->num_sentences;
        my $num_paragraphs        = $fathom->num_paragraphs;
        my $syllables_per_word    = $fathom->syllables_per_word;
        my $words_per_sentence    = $fathom->words_per_sentence;
   	$percent_complex_words = sprintf("%4.2f\%",$percent_complex_words);
   	$syllables_per_word = sprintf("%5.1f",$syllables_per_word);
   	$words_per_sentence = sprintf("%5.1f",$words_per_sentence);
      	my $fog	    = sprintf "%3.3f", $fathom->fog;
      	my $flesch  = sprintf "%3.3f", $fathom->flesch;
        my $kincaid = sprintf "%3.3f", $fathom->kincaid;


	print <<EOsummary;
<div class="heattheme_block">
<div class="heattheme_info">
<ul>
	<li>Number of characters: $num_chars</li>
	<li>Number of words: $num_words</li>
	<li>Percentage of complex words: $percent_complex_words</li>
	<li>Number of sentences: $num_sentences</li>
	<li>Number of paragraphs: $num_paragraphs</li>
	<li>Syllables per word: $syllables_per_word</li>
	<li>Words per sentence: $words_per_sentence</li>
	<li><a href="http://en.wikipedia.org/wiki/Gunning_fog_index">Gunning-Fog index</a>: $fog</li>
	<li><a href="http://en.wikipedia.org/wiki/Flesch-Kincaid_Readability_Test#Flesch_Reading_Ease">Flesch</a> reading ease level: $flesch</li>
	<li><a
	href="http://en.wikipedia.org/wiki/Flesch-Kincaid_Readability_Test#Flesch.E2.80.93Kincaid_Grade_Level">Flesch-Kincaid</a> grade level: $kincaid</li>
</ul>
</div>
EOsummary

#print Dumper(\@order, \%words);
	print &display(\@order, \%words);
	print "</div>\n\n";
}


sub process_stopwords  {
	my $filename= shift;
	open (F, "<$filename") || return;
	foreach my $l (<F>) {
		chomp($l);
		$stopwords{$l}=1;
	}
	close(F);
}


sub display {
	my $item= shift;
	my $byusage= shift;

	unshift @{$item}, '';

	return <<EOhtml;
<table class="heattheme">
<tr>
	<td id="cell1" class="heattheme">$item->[1] ($byusage->{$item->[1]})</td>
	<td id="cell3" class="heattheme">$item->[3] ($byusage->{$item->[3]})</td>
	<td id="cell6" class="heattheme">$item->[6] ($byusage->{$item->[6]})</td>
	<td id="cell10" class="heattheme">$item->[10] ($byusage->{$item->[10]})</td>
</tr>

<tr class="heattheme">
	<td id="cell2" class="heattheme">$item->[2] ($byusage->{$item->[2]})</td>
	<td id="cell5" class="heattheme">$item->[5] ($byusage->{$item->[5]})</td>
	<td id="cell9" class="heattheme">$item->[9] ($byusage->{$item->[9]})</td>
	<td id="cell14" class="heattheme">$item->[14] ($byusage->{$item->[14]})</td>
</tr>
<tr class="heattheme">
	<td id="cell4" class="heattheme">$item->[4] ($byusage->{$item->[4]})</td>
	<td id="cell8" class="heattheme">$item->[8] ($byusage->{$item->[8]})</td>
	<td id="cell13" class="heattheme">$item->[13] ($byusage->{$item->[13]})</td>
	<td id="cell18" class="heattheme">$item->[18] ($byusage->{$item->[18]})</td>
</tr>
<tr class="heattheme">
	<td id="cell7" class="heattheme">$item->[7] ($byusage->{$item->[7]})</td>
	<td id="cell12" class="heattheme">$item->[12] ($byusage->{$item->[12]})</td>
	<td id="cell17" class="heattheme">$item->[17] ($byusage->{$item->[17]})</td>
	<td id="cell22" class="heattheme">$item->[22] ($byusage->{$item->[22]})</td>

</tr>
<tr class="heattheme">
	<td id="cell11" class="heattheme">$item->[11] ($byusage->{$item->[11]})</td>
	<td id="cell16" class="heattheme">$item->[16] ($byusage->{$item->[16]})</td>
	<td id="cell21" class="heattheme">$item->[21] ($byusage->{$item->[21]})</td>
	<td id="cell26" class="heattheme">$item->[26] ($byusage->{$item->[26]})</td>
</tr>
<tr class="heattheme">
	<td id="cell15" class="heattheme">$item->[15] ($byusage->{$item->[15]})</td>
	<td id="cell20" class="heattheme">$item->[20] ($byusage->{$item->[20]})</td>
	<td id="cell25" class="heattheme">$item->[25] ($byusage->{$item->[25]})</td>
	<td id="cell30" class="heattheme">$item->[30] ($byusage->{$item->[30]})</td>
</tr>
<tr class="heattheme">
	<td id="cell19" class="heattheme">$item->[19] ($byusage->{$item->[19]})</td>
	<td id="cell24" class="heattheme">$item->[24] ($byusage->{$item->[24]})</td>

	<td id="cell29" class="heattheme">$item->[29] ($byusage->{$item->[29]})</td>
	<td id="cell34" class="heattheme">$item->[34] ($byusage->{$item->[34]})</td>
</tr>
<tr class="heattheme">
	<td id="cell23" class="heattheme">$item->[23] ($byusage->{$item->[23]})</td>
	<td id="cell28" class="heattheme">$item->[28] ($byusage->{$item->[28]})</td>
	<td id="cell33" class="heattheme">$item->[33] ($byusage->{$item->[33]})</td>
	<td id="cell38" class="heattheme">$item->[38] ($byusage->{$item->[38]})</td>
</tr>
<tr class="heattheme">
	<td id="cell27" class="heattheme">$item->[27] ($byusage->{$item->[27]})</td>
	<td id="cell32" class="heattheme">$item->[32] ($byusage->{$item->[32]})</td>
	<td id="cell37" class="heattheme">$item->[37] ($byusage->{$item->[37]})</td>
	<td id="cell42" class="heattheme">$item->[42] ($byusage->{$item->[42]})</td>

</tr>
<tr class="heattheme">
	<td id="cell31" class="heattheme">$item->[31] ($byusage->{$item->[31]})</td>
	<td id="cell36" class="heattheme">$item->[36] ($byusage->{$item->[36]})</td>
	<td id="cell41" class="heattheme">$item->[41] ($byusage->{$item->[41]})</td>
	<td id="cell45" class="heattheme">$item->[45] ($byusage->{$item->[45]})</td>
</tr>
<tr class="heattheme">
	<td id="cell35" class="heattheme">$item->[35] ($byusage->{$item->[35]})</td>

	<td id="cell40" class="heattheme">$item->[40] ($byusage->{$item->[40]})</td>
	<td id="cell44" class="heattheme">$item->[44] ($byusage->{$item->[44]})</td>
	<td id="cell47" class="heattheme">$item->[47] ($byusage->{$item->[47]})</td>
</tr>
<tr class="heattheme">
	<td id="cell39" class="heattheme">$item->[39] ($byusage->{$item->[39]})</td>
	<td id="cell43" class="heattheme">$item->[43] ($byusage->{$item->[43]})</td>
	<td id="cell46" class="heattheme">$item->[46] ($byusage->{$item->[46]})</td>
	<td id="cell48" class="heattheme">$item->[48] ($byusage->{$item->[48]})</td>
</tr>
</table>

EOhtml


}


sub header {
	return <<EOhtml;
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html lang="en">
<head>
        <meta http-equiv="Content-Type" content="text/html; charset=utf-8">
        <link rel="stylesheet" href="http://services.disruptiveproactivity.com/heattheme/heattheme.css">
        <title>Your Heattheme map</title>
</head>
<body>
<a href="/heattheme/"><h1>Your Heat theme map</h1></a>

EOhtml
}

sub footer {

	return ' <div class="footer"> 
		<a href="http://www.disruptiveproactivity.com/"><img 
		border="0" style="float: right" src="http ://flirble.disruptiveproactivity.com/images/small2.png" 
		alt=" " /></a> </div></body></html> ';
}

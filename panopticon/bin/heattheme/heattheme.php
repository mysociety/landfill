<?php

/*
	Copyright (c) 2006 Chris Jenkinson <chris@starglade.org>.
	All rights reserved.
	
	This software is licensed under the GNU General Public License.
	<http://opensource.org/licenses/gpl-license.php>

	$Id: heattheme.php,v 1.1 2006-07-23 12:33:09 sams Exp $
*/

error_reporting(E_ALL);

require_once 'db.class.php';
require_once 'config.inc.php';
require_once 'functions.inc.php';

$check_stopwords = (isset($_GET['donotcheck']) ? false : true);
$timeframe = (!empty($_GET['timeframe']) ? $_GET['timeframe'] : '');
$time = 0;

# hostname, user, password, db
$db = new sql($_ENV['Hostname'], $_ENV['User'], $_ENV['Password'], $_ENV['Database']);

if ('lastweek' == $timeframe)
{
	echo 'timeframe is last week';
#	$time = mktime(0, 0, 0, date('m'), date('d') - 7);
	$time = " date_sub(now(), interval 7 day) ";
}
else if ('lastmonth' == $timeframe)
{
	echo 'timeframe is last month.';
	$time = " date_sub(now(), interval 30 day) ";
}
else if ($timeframe == 'last3months')
{
	$time = " date_sub(now(), interval 92 day) ";
}
else if ($timeframe == 'last6months')
{
	# $time = mktime(0, 0, 0, date('m') - 6);
	$time = " date_sub(now(), interval 6 month) ";
}
else if ($timeframe == 'lastyear')
{
	# $time = mktime(0, 0, 0, date('m'), date('d'), date('Y') - 1);
	$time = " date_sub(now(), interval 1 year ) ";
}
else
{
	$time = (int) $timeframe;
}

$sql = 'SELECT title
	FROM entries 
	WHERE pubdate > %d 
	ORDER BY pubdate DESC';

$sql = sprintf($sql, $time);

$db->sql_query($sql);

$data = $db->sql_data();

$allwords = array();

$stopwords = @file('stop_words.txt') or die('could not open stop words file.');

$stopwords = array_merge($stopwords, array('written', 'answers', 'commons', 'house', 'debates'));

foreach ($stopwords as $key => $value)
{
	$stopwords[$key] = strtolower(trim($value));
}

foreach ($data as $value)
{
	$words = explode(' ', $value['title']);

	foreach ($words as $word)
	{
		$word = strtolower($word);
		
		if (preg_match('/([a-z]+)/i', $word, $matches))
		{
			$matched = $matches[1];
			
			if ($check_stopwords)
			{
				if (!in_array($matched, $stopwords))
				{
					@$allwords[$matched]++;
				}
			}
			else
			{
				@$allwords[$matched]++;
			}
		}
	}
}

arsort($allwords);

$cell_order = array(1, 3, 6, 10, 2, 5, 9, 14, 4, 8, 13, 18, 7, 12, 17, 22, 11, 16, 21, 26, 15, 20, 25, 30, 19, 24, 29, 34, 23, 28, 33, 38, 27, 32, 37, 42, 31, 36, 41, 45, 35, 40, 44, 47, 39, 43, 46, 48);

$i = 1;
$j = 0;

foreach ($allwords as $word => $count)
{
	$w[$i] = array('word' => $word, 'count' => $count);
	$i++;
}

printf('<link rel="stylesheet" href="cells.css" />' . "\n");

printf("<table class=\"heattheme\">\n<tr>\n");

foreach ($cell_order as $number)
{
	if ($j > 3)
	{
		printf("</tr>\n<tr class=\"heattheme\">\n");
		$j = 0;
	}
	
	$j++;

	if (isset($w[$number]))
	{
		printf("\t" . '<td id="cell%1$d" class="heattheme"><a href="http://panopticon.mysociety.org/?/%2$s">%2$s</a> (%3$d)</td>' . "\n", $number, $w[$number]['word'], $w[$number]['count']);
	}
	else
	{
		printf("\t" . '<td class="heattheme">&nbsp;</td>' . "\n");
	}
}

printf("</tr>\n</table>");

?>

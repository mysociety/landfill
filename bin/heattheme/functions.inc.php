<?php

/*
        Copyright (c) 2006 Chris Jenkinson <chris@starglade.org>.
	All rights reserved.
	
	This software is licensed under the GNU General Public License.
	<http://opensource.org/licenses/gpl-license.php>
	
	$Id: functions.inc.php,v 1.1 2006-07-23 12:33:09 sams Exp $
*/

function add_magic_quotes($array)
{
	foreach ($array as $key => $value)
	{
		if (is_array($value))
		{
			$array[$key] = add_magic_quotes($value);
		}
		else
		{
			$array[$key] = addslashes($value);
		}
	}
	
	return $array;
}

function del_magic_quotes($array)
{
	foreach ($array as $key => $value)
	{
		if (is_array($value))
		{
			$array[$key] = del_magic_quotes($value);
		}
		else
		{
			$array[$key] = stripslashes($value);
		}
	}
	
	return $array;
}

?>

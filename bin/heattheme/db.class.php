<?php

/*
        Copyright (c) 2006 Chris Jenkinson <chris@starglade.org>.
	All rights reserved.
	
	This software is licensed under the GNU General Public License.
	<http://opensource.org/licenses/gpl-license.php>
	
	$Id: db.class.php,v 1.2 2006-08-05 11:29:30 sams Exp $
*/

class sql
{
	var $conn;
	var $res;
	var $queries_array;
	
	function sql ($host, $user, $pass, $db_name, $port = 3306)
	{
		$this->conn = mysql_pconnect("$host:$port", $user, $pass);
		mysql_select_db($db_name, $this->conn);
	}
	
	function sql_insert($table, $columns)
	{
		$sql = 'INSERT INTO %s (%s) VALUES (%s)';
		
		$names = array();
		$vals = array();	    	
		
		foreach ($columns as $name => $value)
		{
			$names[] = $name;
			
			if (is_array($value))
			{
				$vals[] = serialize($value);
			}
			else if (is_string($value))
			{
				$vals[] = sprintf('"%s"', $value);
			}
			else
			{
				$vals[] = $value;
			}
		}
		
		$sql = sprintf($sql, $table, implode(', ', $names), implode(', ', $vals));
		
		$this->sql_query($sql);	
	}
	
	function sql_update($table, $row_name, $row_id, $columns)
	{
		$sql = 'UPDATE %s SET %s WHERE %s = %d';
		
		$vals = array();
		
		foreach ($columns as $name => $value)
		{
			if (is_int($value))
			{
				$vals[] = sprintf('%s = %d', $name, $value);
			}
			else if (is_float($value))
			{
				$vals[] = sprintf('%s = %f', $name, $value);
			}
			else if (is_string($value))
			{
				$vals[] = sprintf('%s = "%s"', $name, $value);
			}
			else if (is_array($value))
			{
				$vals[] = sprintf('%s = "%s"', $name, serialize($value));
			}
		}
		
		$sql = sprintf($sql, $table, implode(', ', $vals), $row_name, $row_id);
		
		$this->sql_query($sql);
	}
	
	function sql_replace($table, $row_name, $row_id, $columns)
	{
		$sql = sprintf('SELECT 1 FROM %s WHERE %s = %d', $table, $row_name, $row_id);
		
		$this->sql_query($sql);
		$row = $this->sql_data(true);
		
		if (!empty($row))
		{
			$this->sql_update($table, $row_name, $row_id, $columns);
		}
		else
		{
			$columns[$row_name] = $row_id;
			
			$this->sql_insert($table, $columns);
		}
	}
	
	function sql_delete($table, $columns)
	{
		$sql = 'DELETE FROM %s WHERE %s';
		
		$vals = array();
		
		foreach ($columns as $name => $value)
		{
			if (is_int($value))
			{
				$vals[] = sprintf('%s = %d', $name, $value);
			}
			else if (is_float($value))
			{
				$vals[] = sprintf('%s = %f', $name, $value);
			}
			else if (is_string($value))
			{
				$vals[] = sprintf('%s = "%s"', $name, $value);
			}
			else if (is_array($value))
			{
				$vals[] = sprintf('%s = "%s"', $name, serialize($value));
			}
		}
		
		$sql = sprintf($sql, $table, implode(' AND ', $vals));
		
		$this->sql_query($sql);
	}
	
	function sql_query($sql)
	{
		$this->queries_array[] = $sql;
		
		$this->res = @mysql_query($sql, $this->conn);
		
		if (mysql_error())
		{
			global $lang;
			
			if (empty($lang))
			{
				$lang['db_error'] = 'A database error occured';
				$lang['db_error_long'] = 'There was an error while accessing the database. The error message is:';
				$lang['db_error_sql'] = 'The SQL query is:';
			}
			
			printf("<h2>%s</h2>\n<p>%s</p>\n<ul>\n\t<li><p>%s (%d)</p></li>\n</ul>\n<p>%s</p>\n<ul>\n\t<li><pre>%s</pre></li>\n</ul>", $lang['db_error'], $lang['db_error_long'], mysql_error(), mysql_errno(), $lang['db_error_sql'], $sql);
			
			echo '<pre>';
			var_dump($this->queries_array);
			debug_print_backtrace();
			
			echo '</pre>';
			
			die();
		}
	}
	
	function sql_data($single = false)
	{
		$row = array();
		$array = array();
		
		if ($single)
		{
			while ($row = $this->sql_fetchrow())
			{
				$array = del_magic_quotes($row);
			}
		}
		else
		{
			while ($row = $this->sql_fetchrow())
			{
				$array[] = del_magic_quotes($row);
			}
		}
		
		return $array;
	}
	
	function sql_fetchrow()
	{		
		$res = (int) $this->res;
		
		if ($res)
		{
			$this->row[$res] = @mysql_fetch_assoc($this->res);
			return $this->row[$res];
		}
		else
		{
			return false;
		}
	}
	
	function sql_close()
	{
		mysql_close($this->conn);
	}
}

?>

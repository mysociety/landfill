<?php
/*
 * admin-pop.php:
 * Placeopedia admin pages.
 * 
 * Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
 * Email: francis@mysociety.org. WWW: http://www.mysociety.org
 *
 * $Id: admin-pop.php,v 1.3 2005-10-18 21:34:22 matthew Exp $
 * 
 */

require_once "../../phplib/db.php";
require_once "../../phplib/utility.php";
require_once "../../phplib/importparams.php";

class ADMIN_PAGE_POP_LATEST {
    function ADMIN_PAGE_POP_LATEST() {
        $this->id = 'poplatest';
        $this->navname = 'Timeline';
        if (get_http_var('linelimit')) {
            $this->linelimit = get_http_var('linelimit');
        } else {
            $this->linelimit = 250;
        }
    }
}

class ADMIN_PAGE_POP_MAIN {
    function ADMIN_PAGE_POP_MAIN() {
        $this->id = 'pop';
        $this->navname = 'All the entries';
    }
}

class ADMIN_PAGE_POP_INCORRECTPLACES { 
    function ADMIN_PAGE_POP_INCORRECTPLACES() {
        $this->id = 'popreports';
        $this->navname = 'Incorrect post reports';
    }

    function replace($ids) {
        $success = false;
        foreach ($ids as $id) {
            $new = db_getRow('SELECT post_id,lat,lon,zoom,name,email FROM incorrect WHERE id=?', array($id));
            if ($new['post_id'] && $new['lat'] && $new['lon']) {
                db_query('UPDATE posts SET hidden=0, lat=?, lon=?, google_lat=?, google_long=?, google_zoom=?, name=?, email=? WHERE postid=?',
                    array($new['lat'], $new['lon'], $new['lat'], $new['lon'], $new['zoom'], $new['name'], $new['email'], $new['post_id']));
                db_query('UPDATE incorrect SET status="Used" WHERE id=?', $id);
                $success = true;
            }
        }
        if ($success)
            print '<p><em>Those entries have been successfully updated.</em></p>';
    }

    function display() {
        if (get_http_var('update')) {
            $ids = get_http_var('update_ids');
            if ($ids) $this->replace($ids);
        }
        foreach ($_POST as $k=>$v) {
            if (preg_match('#^revert_(\d+)#', $k, $m)) {
                $id = $m[1];
                db_query('UPDATE posts SET hidden=0,status="Not used" WHERE postid=(SELECT post_id FROM incorrect WHERE id=?', $id); 
                print '<p><em>That post has been reverted.</em></p>';
            }
        }
    ?>
<form action="" method="post"><input type="hidden" name="page" value="popreports">
<input type="submit" name="update" value="Use new data of checked entries">
<table cellpadding="3" cellspacing="0" border="0">
<tr><th>Article</th><th>Current location</th><th>Creator</th><th>Reporter</th><th>Reason</th><th>New location</th><th>Status</th><th>Revert</th><th>Replace</th></tr>
<?
       $q = db_getAll('SELECT p.postid, p.title, p.lat, p.lon, p.google_zoom, p.name, p.email,
                            i.id AS report_id, i.name AS reporter_name, i.email AS reporter_email, i.reason,
                            i.lat AS new_lat, i.lon AS new_lon, i.zoom AS new_zoom, i.status
                        FROM incorrect AS i,posts AS p
                        WHERE p.postid = i.post_id');
        foreach ($q as $r) {
            if (!$r['status']) $r['status'] = 'Unsorted';
            $new_loc = 'Not given'; $update = false;
            if ($r['new_lon'] && $r['new_lat']) {
                $update = true;
                $new_loc = "($r[new_lon], $r[new_lat], $r[new_zoom])";
            }
            print "<tr><td><a href='http://en.wikipedia.org/wiki/$r[title]'>$r[title]</a></td>
            <td><a href='http://www.placeopedia.com/?$r[postid]'>($r[lon], $r[lat], $r[google_zoom])</a></td>
            <td>$r[name]<br>$r[email]</td><td>$r[reporter_name]<br>$r[reporter_email]</td>
            <td>$r[reason]</td>
            <td>$new_loc</td><td>$r[status]</td><td><input type='submit' name='revert_$r[report_id]' value='Original is correct'></td>";
            if ($update) print "<td><input type='checkbox' name='update_ids[]' value='$r[report_id]'></td>";
            else print '<td>No new location</td>';
            print '</tr>';
        }
?></table>
</form><?
    }
}

class ADMIN_PAGE_POP_SUMMARY {
    function ADMIN_PAGE_PB_SUMMARY() {
        $this->id = 'summary';
    }
    function display() {
    }
}

?>

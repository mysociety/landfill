<?php
/*
 * admin-pop.php:
 * Placeopedia admin pages.
 * 
 * Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
 * Email: francis@mysociety.org. WWW: http://www.mysociety.org
 *
 * $Id: admin-pop.php,v 1.1 2005-10-07 19:07:56 matthew Exp $
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
            $new = db_getRow('SELECT post_id,lat,lon,zoom FROM incorrect WHERE id=?', array($id));
            if ($new['post_id'] && $new['lat'] && $new['lon']) {
                db_query('UPDATE posts SET hidden=0, lat=?, lon=?, google_lat=?, google_long=?, google_zoom=? WHERE postid=?',
                    array($new['lat'], $new['lon'], $new['lat'], $new['lon'], $new['zoom'], $new['post_id']));
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
    ?>
<form action="" method="post"><input type="hidden" name="page" value="popreports">
<input type="submit" name="update" value="Use new data of checked entries">
<table cellpadding="3" cellspacing="0" border="0">
<tr><th>Article</th><th>Current location</th><th>Creator</th><th>Reporter</th><th>New location</th></tr>
<?
       $q = db_getAll('SELECT p.postid, p.title, p.lat, p.lon, p.google_zoom, p.name, p.email,
                            i.id AS report_id, i.name AS reporter_name, i.email AS reporter_email,
                            i.lat AS new_lat, i.lon AS new_lon, i.zoom AS new_zoom
                        FROM incorrect AS i,posts AS p
                        WHERE p.postid = i.post_id');
        foreach ($q as $r) {
            $new_loc = 'Not given'; $update = false;
            if ($r['new_lon'] && $r['new_lat']) {
                $update = true;
                $new_loc = "($r[new_lon], $r[new_lat], $r[new_zoom])";
            }
            print "<tr><td>$r[title]</td><td><a href='http://www.placeopedia.com/?$r[postid]'>($r[lon], $r[lat], $r[google_zoom])</a></td>
            <td>$r[name]<br>$r[email]</td><td>$r[reporter_name]<br>$r[reporter_email]</td>
            <td>$new_loc</td>";
            if ($update) print "<td><input type='checkbox' name='update_ids[]' value='$r[report_id]'></td>";
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

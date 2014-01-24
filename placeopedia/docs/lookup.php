<?

$xml = false;
if ($_GET['output'] == 'xml') {
    $xml = true;
    header("Content-Type: application/xml");
    print "<page>\n";
}

$q = $_GET['q'];
if (!$q) exit;
if (validate_postcode($q)) {
	$q = preg_replace('#[^A-Za-z0-9]#', '', $q);
	$q = strtoupper($q);
	$q = preg_replace('#(\d[A-Z]{2})#', ' $1', $q);
}
$h_q = htmlspecialchars($q);
$u_q = urlencode($q);
$f = file_get_contents('http://maps.google.co.uk/maps?output=js&q=' . $u_q);
if (preg_match('#panel: \'(.*?)\'#', $f, $m) && preg_match('#We could not understand#', $f)) {
    $error = $m[1];
    $out = ($xml) ? '<error><![CDATA['.$error.']]></error>' : $error;
} elseif (preg_match('#panel: \'(.*?)\'#', $f, $m)) {
    $refine = $m[1];
    if ($xml) {
        preg_match_all('#<div class=\\\042ref\\\042><a href=\\\042/maps\?q=(.*?)&.*?>(.*?)</a></div>#', $refine, $m, PREG_SET_ORDER);
        foreach ($m as $match) {
            $out .= "<refinement><query>$match[1]</query><description><![CDATA[$match[2]]]></description></refinement>\n";
        }
    } else {
	preg_match_all('#<refinement><query>(.*?)</query><description>(.*?)</description></refinement>#', $refine, $m, PREG_SET_ORDER);
        if (sizeof($m)) {
            $out .= "<p>More than one match for <strong>$h_q</strong>, pick one:</p> <ul>";
            foreach ($m as $r) {
                $q = $r[1];
                $d = $r[2];
                $out .= '<li><a href="?'.urlencode($q).'">'.$d.'</a></li>';
            }
            $out .= '</ul>';
        } else {
            $out .= '<p>No matches!</p>';
        }
    }
} elseif (preg_match('#<overlay panelStyle="/maps\?file=lp.*?>(.*?)</overlay>#', $f, $m)) {
    if ($xml) {
    } else {
        $overlay = $m[1];
        preg_match_all('#<location[^>]*?><point lat="(.*?)" lng="(.*?)"/>.*?<title[^>]*?>(.*?)</title><address>(.*?)</address><phone>(.*?)</phone><distance>(.*?)</distance>.*?<url>([^<]*?)</url></info></location>#', $overlay, $m, PREG_SET_ORDER);
        if (sizeof($m)) {
            $out .= '<p>The following places matched <strong>'.$h_q.'</strong>:</p> <ul>';
            foreach ($m as $r) {
                $lat = $r[1]; $lng = $r[2]; $title = $r[3];
                $address = strip_tags(str_replace('</line><line>', ', ', $r[4]));
                $phone = $r[5]; $distance = $r[6]; $url = $r[7];
                $out .= '<li><a href="http://maps.google.co.uk'.$url.'">'.$title."</a>, $address, $phone, $distance ($lat, $lng)";
            }
            $out .= '</ul>';
        } else {
            $out .= '<p>No matches!</p>';
        }
    }
} else {
    preg_match('#center: {lat: (.*?),lng: (.*?)}#', $f, $m);
    if ($xml) $out = '<center lat="'.$m[1].'" lng="'.$m[2].'"/>';
    else {
        $lat = $m[1]; $lng = $m[2];
    }
    preg_match('#span: {lat: (.*?),lng: (.*?)}#', $f, $m);
    if ($xml) $out .= '<span lat="'.$m[1].'" lng="'.$m[2].'"/>';
    else {
        $sp_lat = $m[1]; $sp_lng = $m[2];
        $out .= "<div id='map'></div> <p><strong>$h_q</strong> : $lat $lng";
        $out .= '<script type="text/javascript">var scy = '.$lat.'; var scx = '.$lng.';</script>';
    }
}
if ($out) print $out;
if ($xml) print "</page>\n";

function validate_postcode ($postcode) {
    // See http://www.govtalk.gov.uk/gdsc/html/noframes/PostCode-2-1-Release.htm
    $in  = 'ABDEFGHJLNPQRSTUWXYZ';
    $fst = 'ABCDEFGHIJKLMNOPRSTUWYZ';
    $sec = 'ABCDEFGHJKLMNOPQRSTUVWXY';
    $thd = 'ABCDEFGHJKSTUW';
    $fth = 'ABEHMNPRVWXY';
    $num = '0123456789';
    $nom = '0123456789';
    $gap = '\s\.';

    if (preg_match("/^[$fst][$num][$gap]*[$nom][$in][$in]$/i", $postcode) ||
        preg_match("/^[$fst][$num][$num][$gap]*[$nom][$in][$in]$/i", $postcode) ||
        preg_match("/^[$fst][$sec][$num][$gap]*[$nom][$in][$in]$/i", $postcode) ||
        preg_match("/^[$fst][$sec][$num][$num][$gap]*[$nom][$in][$in]$/i", $postcode) ||
        preg_match("/^[$fst][$num][$thd][$gap]*[$nom][$in][$in]$/i", $postcode) ||
        preg_match("/^[$fst][$sec][$num][$fth][$gap]*[$nom][$in][$in]$/i", $postcode)) {
        return true;
    } else {
        return false;
    }
}


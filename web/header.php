<?
$path = '';
$home = ($_SERVER['PHP_SELF'] == $path . '/index.php');
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html lang="en-gb">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<title><? if (isset($title)) print $title . ' - '; ?>mySociety.co.uk</title>
<style type="text/css" media="all">@import url('<?=$path ?>/style.css');</style>
</head>
<body<? if ($home) print ' id="home"'; ?>>
<?=($home?'<h1 id="header">':'<div id="header"><a href="' . $path . '/">')
?><img border="0" src="/i/logo.png" alt="mySociety" width="311" height="68"><?=($home?'</h1>':'</a></div>') ?>

<? if (!$home) { ?>
<table align="center" border="0" cellpadding="10" cellspacing="0">
<tr>
<td colspan="3"><a href="<?=$path ?>/fixmystreet/">FixMyStreet</a>
<td align="right" colspan="3"><a href="<?=$path ?>/hearfromyourcouncillor/">HearFromYourCouncillor</a>
</tr>
<tr>
<td colspan="2"><a href="<?=$path ?>/pledgebank/">PledgeBank</a>
<td align="right" colspan="2"><a href="<?=$path ?>/writetothem/">WriteToThem</a>
</tr>
</table>

<?  if (!isset($nopdf)) { ?>
<a href="/pdfs<?=dirname($_SERVER['PHP_SELF']) ?>.pdf"><img align="right" src="/i/pdf.png" width="75" height="75" alt="PDF Document"></a>
<?      }
    } ?>


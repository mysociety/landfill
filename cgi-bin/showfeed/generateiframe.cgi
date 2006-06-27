#!/usr/bin/perl -I../

use warnings;
use strict;
use mySociety::Boxes::Config;
use mySociety::Boxes::Routines;

print "Content-Type: text/html\n\n";
my $boxid=param("boxid") || 1;
print <<EOhtml;
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
    "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">

<head>
  <title></title>
 <link href="$static_url_prefix/$boxid.rss" type="application/rss+xml" />
  <style type="text/css">
      	div.mysociety_yp {
      		width:200px;
      		position:relative;
		background:white;
		color:#808080;
		font-size:0.6em;
	}

       	div.mysociety_yp, div.mysociety_yp input {
       		font-family:Verdana, sans-serif;
	}

      	div.mysociety_yp_frame {
		border:solid 2px #9aa65b;
		padding:0;
	}

      div.mysociety_yp a:hover {color:#E59A51}

      div.mysociety_yp h3 {height:1%;
      		font-weight:bold;
		font-size:1.1em;
		text-align:center;
		color:white;
		padding:4px 0;
		margin:0;
		background:#9aa65b;
	}


	  div.mysociety_yp_search input.textbox {
	  	width:190px;
        	background: white url('http://memedev.com/democracybox/box/return.png') no-repeat right 50%;
		border:solid 1px #808080;
		padding:3px;
	}

      div.mysociety_yp_search input.button {height:2em;}

      h4.mysociety_yp_pledgebank,  h4.mysociety_yp_theyworkforyou, h4.mysociety_yp_hearfromyourmp {
      		padding:3px;
		margin:0;
		color:white;
		border-bottom: solid 1px #909f3b;
		border-top: solid 1px #909f3b;
		height:1%;
		font-size:1em;
	}

      	h4.mysociety_yp_pledgebank {color:#A056AF; background:#f6cfff;}

	h4.mysociety_yp_theyworkforyou {color:#358431; background:#d3efd1;}

	h4.mysociety_yp_hearfromyourmp {color:#421a8f; background:#dff0ff;}


      	div.mysociety_yp ul {list-style-type:none;margin:0; padding:0;}

      	div.mysociety_yp ul li {padding:3px; border-bottom:1px dotted #909f3b;height:1%}

      	div.mysociety_yp ul li a {text-decoration:none; color:#292929; }

      	li.mysociety_yp_morelink {font-size:1em; position:relative;border-bottom:none!important;}

	li.mysociety_yp_morelink a {color:#E59A51!important;}

	li.mysociety_yp_morelink small {position:absolute; right:3px; color:#bfbfbf; font-size:1em;}

      	div.mysociety_yp_ad {
		text-align:center;
		background:#ffffcc;
		border-top:solid 1px #909f3b;
        	border-bottom:solid 1px #909f3b;
		margin-bottom:0;
		padding:3px;
		height:1%
	}

      	div.mysociety_yp_mysocietylogo {
      		text-align:center;
		background:#9aa65b;
		height:1%
	}

      div.mysociety_yp_mysocietylogo a {
      		display:block;
		width:99px;
		height:21px;
        	background:url('http://memedev.com/democracybox/box/mysociety.png') no-repeat 0 2px;
		margin:auto;
		padding-top:4px;
        	position:relative;
	}

     div.mysociety_yp_mysocietylogo a span {display:none;}

  </style>
</head>
<body id="body">
EOhtml

&generate_box_style_richard2($boxid);

print <<EOhtml;


</body>
</html>


EOhtml

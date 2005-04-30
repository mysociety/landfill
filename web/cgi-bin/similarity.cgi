#!/usr/bin/perl -w
#
# similarity.cgi:
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#

my $rcsid = ''; $rcsid .= '$Id: similarity.cgi,v 1.6 2005-04-30 19:50:31 matthew Exp $';

use strict;

use lib qw(/home/chris/projects/mysociety/perllib /data/vhost/www.notapathetic.com/docs/cgi-bin);

use CGI::Fast;
use DBI;

use mysociety::NotApathetic::Config;

use mySociety::Util;

my $dbh;

while (my $q = new CGI::Fast()) {
    if (!defined($dbh) || !eval { $dbh->ping() }) {
        $dbh = DBI->connect($mysociety::NotApathetic::Config::dsn, $mysociety::NotApathetic::Config::db_username, $mysociety::NotApathetic::Config::db_password, {RaiseError => 1});
    }

    my $userid = $q->cookie(-name => 'notapathetic_userid');
    undef($userid) if (defined($userid) && $userid !~ m#^(0|[1-9]\d*)$#);
    $userid ||= int(rand(0x7fffffff));

    my $C = $q->cookie(
                    -name => 'notapathetic_userid',
                    -value => $userid,
                    -path => '/',
                    -domain => 'notapathetic.com',
                    -expires => '+365d'
                );

    # See if the user has given us an answer for the two posts.
    if (defined($q->param('id1')) && defined($q->param('id2'))) {
        foreach (qw(0 1 2)) {
            if (defined($q->param("s$_"))) {
                $dbh->do('
                        replace into similarity
                            (userid, postid1, postid2, similarity)
                        values (?, ?, ?, ?)', {},
                        $userid, $q->param('id1'), $q->param('id2'), $_);
            }
        }
    }

again:

    # Obtain two posts and their IDs.
    my ($id1, $why1) =
        $dbh->selectrow_array('
                select postid, why
                from posts
                where hidden = 0 and validated <> 0
                order by rand()
                limit 1');

    $why1 =~ s/(\r\n){2,}/<\/p> <p>/g;
    $why1 =~ s/\r\n/<br \/>/g;

    my ($id2, $why2) =
        $dbh->selectrow_array('
                select postid, why
                from posts
                where postid <> ? and hidden = 0 and validated <> 0
                order by rand()
                limit 1', {}, $id1);

    $why2 =~ s/(\r\n){2,}/<\/p> <p>/g;
    $why2 =~ s/\r\n/<br \/>/g;

    my $s = $dbh->selectrow_array('
                    select similarity
                    from similarity
                    where userid = ?
                        and ((postid1 = ? and postid2 = ?)
                            or (postid1 = ? and postid2 = ?))',
                    {}, $userid, $id1, $id2, $id2, $id1);

    goto again if (defined($s));

    # How many scores has this person produced?
    my $howmany = $dbh->selectrow_array('
                    select count(*)
                    from similarity
                    where userid = ?', {}, $userid);

    my $congrats = '';
    if ($howmany >= 100 && $howmany < 105) {
        $congrats = q(<p style="font-size: 200%"><b>Well done!</b> You've fulfilled the pledge!);
    } elsif ($howmany > 105 && $howmany < 200) {
        $congrats = q(<p>It is hard to express how grateful we are for your dedication to the gathering of statistics</p>);
    } elsif ($howmany >= 200 && $howmany < 205) {
        $congrats = q(<p>You are a hero of statistics gathering</p>);
    } elsif ($howmany >= 300 && $howmany < 305) {
        $congrats = q(<p>Your text-classification powers are amazing! We are considering bottling you and selling you to Google.</p>);
    }

    print $q->header(-cookie => $C), <<EOF;
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
<title>Not Apathetic - help us understand why people are not voting in the 2005 general election</title>
<link rel="stylesheet" type="text/css" title="mainscreen" href="/css/notnetscape.css" media="screen, projection" />
<!-- <link rel="stylesheet" type="text/css" title="portable" href="/css/handheld.css" media="handheld" /> -->
<link rel="shortcut icon" href="/favicon.ico" />
<link href="/cgi-bin/rss.cgi" rel="alternate" type="application/rss+xml" title="NotApathetic Reasons" />
<script type="text/javascript" src="/forms.js"></script>
</head>
<body>
<div id="wrap"><a name="top"></a>
<div id="top">
<h1><a href="/"><img src="/images/na2.png" alt="Not Apathetic" /></a></h1>
<h3 id="myh3"><a href="/">Tell the world why you're not voting - don't let your silence go unheard<span></span></a></h3>
</div>
    
    <ul class="nav main">
    <li><a href="/"><span>h</span>ome</a></li>
    <li><a href="/about/"><span>a</span>bout us</a></li>
    <li><a href="/news/"><span>n</span>ews</a></li>
    <li><a href="/emailnotify/"><span>e</span>mail alerts</a></li>
    <li class="last"><a href="/emailfriend/"><span>t</span>ell a friend</a></li>
    </ul>
    <ul class="nav posts">
    <li><a href="/recentcomments/"><span>r</span>ecent comments</a></li>
    <li><a href="/random"><span>r</span>andom</a></li>
    <li><a href="/busiest/"><span>b</span>usiest posts</a></li>
    <li class="last"><a title="RSS feeds, logos, XML" href="/data/"><span>rss</span>+</a></li>
    </ul>

<!--<div style="width: 100%" width="100%">-->
<style type="text/css">
div#leftColumn  {
    clear: left;
    float: left;
    width: 330px;
}
div#rightColumn {
    width: 330px;
    float: right;
}

div#voteButtons {
    width: 737px;       /* A small prize if you can explain this number -- chris\@mysociety.org */
    background-color: #ccc;
    padding: 10px;
    clear: both;
    float: left;
}

div#voteButtons form {
    display: inline;
}

</style>
    
<div id="voteButtons">
<p style="font-weight: bold;">So far you've classified $howmany pairs</p>
$congrats
<div style="font-weight: bold;">Compare these reasons:
    <form method="POST" action="/cgi-bin/similarity.cgi">
        <input type="hidden" name="id1" value="$id1">
        <input type="hidden" name="id2" value="$id2">
        <input type="submit" name="s0" value="Very similar">
    </form>
    <form method="POST" action="/cgi-bin/similarity.cgi">
        <input type="hidden" name="id1" value="$id1">
        <input type="hidden" name="id2" value="$id2">
        <input type="submit" name="s1" value="Similar">
    </form>
    <form method="POST" action="/cgi-bin/similarity.cgi">
        <input type="hidden" name="id1" value="$id1">
        <input type="hidden" name="id2" value="$id2">
        <input type="submit" name="s2" value="Different">
    </form>
</div>

</div>

<div id="leftColumn">
<h2>They're <span>not voting</span> because...</h2>
<div><p>$why1</p></div>
<p style="font-size: 80%;"><a href="/comments/$id1">Link</a></p>
</div>
<div id="rightColumn">
<h2>They're <span>not voting</span> because...</h2>
<div><p>$why2</p></div>
<p style="font-size: 80%;"><a href="/comments/$id2">Link</a></p>
</div>

</div>

<div id="footer">
<small>Built by </small><a href="http://www.mysociety.org/"><img src="/images/mysociety.gif" alt="mySociety" /></a>
<ul class="nav footer"><li><a href="mailto:team\@notapathetic.com"><span>c</span>ontact us</a></li>
<li><a href="/about/privacy/"><span>p</span>rivacy policy</a></li></ul>
<div class="hideme"><a href="#top">back to top of page</a></div>
</div>

</div>

</body>
</html>
EOF
}

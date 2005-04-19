#!/usr/bin/perl -w
#
# similarity.cgi:
#
# Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
# Email: chris@mysociety.org; WWW: http://www.mysociety.org/
#

my $rcsid = ''; $rcsid .= '$Id: similarity.cgi,v 1.1 2005-04-19 13:13:35 chris Exp $';

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
    $userid ||= int(rand(0xffffffff));

    # See if the user has given us an answer for the two posts.
    if (defined($q->param('id1')) && defined($q->param('id2'))) {
        foreach (qw(0 1 2)) {
            if (defined($q->param("s$_"))) {
                $dbh->do('
                        insert into similarity
                            (userid, postid1, postid2, similarity)
                        values (?, ?, ?, 0)', {},
                        $userid, $q->param('id1'), $q->param('id2'));
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

    print $q->header(
                -cookie => $q->cookie(
                                -name => 'notapathetic_userid', -value => $userid
                            )
            ), <<EOF;
<p>So far you've classified $howmany pairs</p>
<table width="100%"><tr>
<td width="33%">
    <form method="POST">
        <input type="hidden" name="id1" value="$id1">
        <input type="hidden" name="id2" value="$id2">
        <input type="submit" name="s0" value="Very similar">
    </form>
</td>
<td width="33%">
    <form method="POST">
        <input type="hidden" name="id1" value="$id1">
        <input type="hidden" name="id2" value="$id2">
        <input type="submit" name="s1" value="Similar">
    </form>
</td>
<td width="33%">
    <form method="POST">
        <input type="hidden" name="id1" value="$id1">
        <input type="hidden" name="id2" value="$id2">
        <input type="submit" name="s2" value="Different">
    </form>
</td>
</tr></table>
<table width="100%"><tr>
    <td width="50%">$why1</td>
    <td width="50%">$why2</td>
</tr></table>
EOF
}

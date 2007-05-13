#!/bin/sh
#
# Run this script to send CVSTrac to the website.
#
SRCDIR=../cvstrac

VERS=`echo '@VERSION@' | sed -f $SRCDIR/VERSION`
make
strip cvstrac
mv cvstrac cvstrac-$VERS.bin
gzip -f cvstrac-$VERS.bin
DIR=`pwd`
cd $SRCDIR/..
mv cvstrac cvstrac-$VERS
tar czf $DIR/cvstrac-$VERS.tar.gz `find cvstrac-$VERS -type f | grep -v CVS`
mv cvstrac-$VERS cvstrac
cd $DIR
echo 'rsync to www.hwaci.com'
export RSYNC_RSH=ssh
rsync cvstrac-$VERS.bin.gz index.html cvstrac-$VERS.tar.gz \
   hwaci@typhoon.he.net:public_html/sw/cvstrac
echo 'rsync to www.cvstrac.org'
rsync cvstrac-$VERS.bin.gz index.html cvstrac-$VERS.tar.gz \
   root@sqlite.org:/home/www/www_cvstrac_org.website

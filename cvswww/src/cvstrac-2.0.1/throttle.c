/*
** Copyright (c) 2002 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public
** License as published by the Free Software Foundation; either
** version 2 of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** General Public License for more details.
** 
** You should have received a copy of the GNU General Public
** License along with this library; if not, write to the
** Free Software Foundation, Inc., 59 Temple Place - Suite 330,
** Boston, MA  02111-1307, USA.
**
** Author contact information:
**   drh@hwaci.com
**   http://www.hwaci.com/drh/
**
*******************************************************************************
**
** This file contains code used to throttle output to misbehaving spiders.
*/
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "throttle.h"

/*
** The following is an approximation of -ln(0.5)/3600.0.  Euler's constant
** (e) raised to this power is 0.5.
*/
#define DECAY_RATE  0.00019254

/* Number of idle seconds before captcha cookie expires */
#define CAPTCHA_EXPIRE 86400

/* Maximum number of hits on the captcha page before a lockout */
#define CAPTCHA_LOCKOUT 5

/* Lock the user out if they fail the is_edit_allowed() check this many
** times.
*/
#define WIKI_EDIT_LOCKOUT 2

/*
** Return the name of the captcha cookie
*/
static char *captcha_cookie_name(void){
  return mprintf("%s_captcha", g.zName);
}

static void captcha_set_cookie(void){
  /* The captcha cookie is just an expiry time value. Nothing fancy,
  ** we don't need to keep much information. We do want to update it
  ** to a later expiry on successful connection...
  */
  char *zExpiry = mprintf("%d",time(NULL)+CAPTCHA_EXPIRE);
  cgi_set_cookie(captcha_cookie_name(), zExpiry, 0, 0);
}

static void captcha_clear_cookie(void){
  cgi_set_cookie(captcha_cookie_name(), "0", 0, 0);
}

static void lockout(){
  cgi_reset_content();
  common_standard_menu(0,0);
  common_header("Access denied");
  @ <p>Your access to this website has been temporarily suspended because
  @ you are using it excessively.  You can retry your request later.</p>
  common_footer();
  cgi_append_header("Pragma: no-cache\r\n");
  cgi_set_status(403,"Forbidden");
  cgi_reply();
  exit(0);
}

/*
** Check to see if there have been too many recent accesses from the
** same IP address. 
**
** If there is an overload, the resulting action depends on the exitOnOverload
** parameter.  If that parameter is true, an error reply is constructed
** and an exit performed without returning.  If the parameter is false,
** a non-zero value is returned.
**
** If the needCaptcha flag is non-zero, the client will be required to
** pass the captcha test regardless of the load. However, failure to
** pass the test won't result in eventual lockout.
**
** If there is no overload, a zero is returned.
*/
int throttle(int exitOnOverload,int needCaptcha){
  const char *zLimit;
  double rLimit;
  const char *zAddr;
  char **az;
  double rLoad;
  double rLastLoad;
  int lastAccess;
  time_t now;
  int overload;
  int captcha = CAPTCHA_LOCKOUT;
  int useCaptcha = atoi(db_config("enable_captcha","0"));
  const char *zCookie = P(captcha_cookie_name());
  const char *zUrl = getenv("REQUEST_URI");

  time(&now);

  if( !g.isAnon ) return 0; /* Throttling does not occur for identified users */
  zLimit = db_config("throttle", 0);
  if( zLimit==0 ) return 0; /* Throttling is turned off */
  rLimit = atof(zLimit);
  if( rLimit<=0.0 ) return 0;  /* Throttling is turned off */

  /* Users with valid captcha cookies are okay.
  */
  if( useCaptcha && zCookie && zCookie[0] && atoi(zCookie) > now ){
    /* update the cookie to a new expiry time
    */
    captcha_set_cookie();
    return 0;
  }

  zAddr = getenv("REMOTE_ADDR");
  if( zAddr==0 ) return 0;  /* No remote IP address provided */

  az = db_query("SELECT load, lastaccess, captcha FROM access_load "
                "WHERE ipaddr='%q'", zAddr);
  if( az[0] && az[1] ){
    rLastLoad = rLoad = atof(az[0]);
    lastAccess = atoi(az[1]);
    if( lastAccess>now ) lastAccess = now;
    rLoad = 1.0 + exp(DECAY_RATE*(lastAccess-now))*rLoad;
    if( rLoad>rLimit && rLoad<rLimit*2.0 ){
      /* If the throttler triggers, make sure it locks out the IP address
      ** for at least 1 hour */
      rLoad = rLimit*2.0;
    }
    if( rLoad>rLimit && rLastLoad>rLimit && az[2] && useCaptcha ){
      /* Once the client blows the limit, repeated hits on anything
      ** other than the captcha page will decrement the captcha
      ** counter, eventually resulting in lockout.
      */
      captcha = atoi(az[2])-1;
    }
  }else{
    rLastLoad = rLoad = 1.0;
    lastAccess = 0;
  }

  db_execute("REPLACE INTO access_load(ipaddr,load,lastaccess,captcha) "
             "VALUES('%q',%g,%d,%d)", zAddr, rLoad, now, captcha);
  overload = rLoad>=rLimit;
  if( useCaptcha && (overload || needCaptcha) ){
    if( captcha <= 0 && exitOnOverload ) {
      /* Captcha lockout count exceeded, client is blocked until the
      ** load drops again.
      */
      lockout();
    }

    if( zUrl==0 ) zUrl = "index";
    cgi_redirect(mprintf("captcha?cnxp=%T", zUrl));
  }else if( overload && exitOnOverload ){
    /* Just block the client */
    lockout();
  }
  return overload;
}

/*
** WEBPAGE: /honeypot
**
** This page gives fair warning to real users not to click on any of the
** hyperlinks.
*/
void honeypot(void){
  login_check_credentials();
  if( !g.isAnon ){
    cgi_redirect("index");
  }
  common_add_action_item("stopper","I am a spider");
  common_add_action_item("index","I am human");
  common_add_help_item("CvstracAdminAbuse");
  common_header("Honey Pot");
  @ <p>This page is intended to capture spiders that
  @ ignore the "robots.txt" file.
  @ If you are not a spider, click on the "I am human" link
  @ above.  If you click on the "I am a spider" link, your access to this
  @ server will be suspended for about an hour.</p>
  common_footer();
}

/*
** WEBPAGE: /stopper
**
** Only robots should come to this page, never legitimate users.  Disable
** any IP address that comes to this page.
*/
void stopper(void){
  const char *zAddr = getenv("REMOTE_ADDR");
  const char *zLimit = db_config("throttle", 0);
  double rLimit;

  login_check_credentials();
  if( zLimit!=0 && (rLimit = atof(zLimit))>0.0 && zAddr!=0 ){
    time_t now;
    time(&now);
    db_execute("REPLACE INTO access_load(ipaddr,load,lastaccess) "
               "VALUES('%q',%g,%d)", zAddr, rLimit*2.0, now);
    throttle(1,1);
  }
  cgi_redirect("index");
}


/*
** WEBPAGE: /info_throttle
**
** Provide information about the current throttling database.
*/
void throttle_info(void){
  const char *zReset = P("reset");
  const char *zOrderBy;
  char **az;
  int limit;
  int i;

  login_check_credentials();
  if( !g.okSetup ){
    login_needed();
    return;
  }
  zOrderBy = PD("ob","1");
  limit = atoi(PD("limit","50"));
  if( zOrderBy[0]=='1' ){
    zOrderBy = "ORDER BY load DESC";
  }else if( zOrderBy[0]=='2' ){
    zOrderBy = "ORDER BY ipaddr";
  }else{
    zOrderBy = "ORDER BY lastaccess DESC";
  }
  if( zReset ){
    time_t now;
    time(&now);
    db_execute("DELETE FROM access_load WHERE lastaccess<%d", now-86400);
  }
  common_add_nav_item("setup", "Main Setup Menu"); 
  common_add_help_item("CvstracAdminAbuse");
  common_add_action_item("info_throttle?reset=1","Remove Older Entries");
  if( limit>0 ){
    common_add_action_item("info_throttle?limit=-1","View All");
  }else{
    common_add_action_item("info_throttle?limit=50","View Top 50");
  }
  common_header("Throttle Results");
  @ Contents of the ACCESS_LOAD table:
  @ <table border="1" cellspacing="0" cellpadding="2">
  @ <tr>
  @ <th><a href="info_throttle?ob=2">IP Address</a></th>
  @ <th><a href="info_throttle?ob=3">Last Access</a></th>
  @ <th><a href="info_throttle?ob=1">Load</a></th></tr>
  az = db_query("SELECT ipaddr, lastaccess, load FROM access_load %s LIMIT %d",
               zOrderBy, limit);
  for(i=0; az[i]; i+=3){
    struct tm *pTm;
    time_t atime;
    char zTime[200];
    atime = atoi(az[i+1]);
    pTm = localtime(&atime);
    strftime(zTime, sizeof(zTime), "%Y-%m-%d %H:%M:%S", pTm);
    @ <tr><td>&nbsp;&nbsp;%h(az[i])&nbsp;&nbsp;</td>
    @ <td>&nbsp;&nbsp;%h(zTime)&nbsp;&nbsp;</td>
    @ <td>&nbsp;&nbsp;%s(az[i+2])&nbsp;&nbsp;</td></tr>
  }
  @ </table>
  @ <p>
  @ <a href="info_throttle?reset=1">Remove older entries</a>
  common_footer();
}

/*
** WEBPAGE: /captcha
**
** Generate the captcha page. This is basically a honeypot with a cookie
** for state. Once the client exceeds the throttle threshold, they risk
** getting locked out unless they (eventually) get this question correct
** or slow down on the hits.
*/
void captcha_page(void){
  time_t now = time(NULL);
  int q1, q2;

  if( atoi(PD("a","0")) == (atoi(PD("q1","-1")) + atoi(PD("q2","-1"))) ){
    /* User gave the right answer. Set cookie and continue on.
    */
    captcha_set_cookie();
    cgi_redirect(PD("nxp","index"));
    return;
  }

  /* Note that we don't do _any_ credential checks in this page... However,
  ** some flags are needed for sane header generation. For example, we
  ** want a "Login" in the menu rather than "Logout" so isAnon should be
  ** set.
  */
  g.isAnon = 1;

  common_standard_menu(0, 0);
  common_add_help_item("CvstracAdminAbuse");
  common_header("Abbreviated Turing Test");

  /* small numbers */
  srand(now);
  q1 = (rand()%5)+1;
  q2 = (rand()%5)+1;

  @ In order to continue, you must show you're a human. Please answer
  @ the following mathematical skill testing question (and ensure cookies
  @ are enabled):
  @ <p>
  @ <form action="captcha" method="POST">
  @ What is <tt>%d(q1) + %d(q2)</tt>?
  @ <input type="text" name="a" value="" size=4>
  @ <font size="-1">Hint: %d(q1+q2)</font>
  @ <input type="hidden" name="q1" value="%d(q1)">
  @ <input type="hidden" name="q2" value="%d(q2)">
  if( P("nxp") ){
    @ <input type="hidden" name="nxp" value="%h(P("nxp"))">
  }
  @ </p>
  @ <p>
  @ <input type="submit" name="in" value="Submit"></td>
  @ </p>
  @ </form>
  common_footer();
}

static int count_links(const char *zText){
  int nLink = 0;

  if( zText!=0 ){
    int i, j;

    for(i=0;zText[i];i++){
      char c = zText[i];
      if( (c=='h' || c=='f' || c=='m') && (j=is_url(&zText[i]))>0 ){
        nLink ++;
        i += j;
        continue;
      }
    }
  }
  return nLink;
}

/* If someone blows the limit, tweak the current throttler counter
** for their IP. Each failure increases it by a defined fraction
** of the throttle limit, which mean they'll get locked out after
** triggering the failure too many times. Currently, that's
** twice.
**
** Hopefully this strikes a balance between stopping spammers and
** not annoying legitimate users too much...
*/
static void increase_load(){
  const char *zAddr = getenv("REMOTE_ADDR");
  const char *zLimit = db_config("throttle", 0);
  double rLimit;

  if( zLimit!=0 && (rLimit = atof(zLimit))>0.0 && zAddr!=0 ){
    time_t now = time(0);
    char *zLastLoad = db_short_query("SELECT load FROM access_load "
                                     "WHERE ipaddr='%q'", zAddr);
    db_execute("REPLACE INTO access_load(ipaddr,load,lastaccess) "
               "VALUES('%q',%g,%d)", zAddr,
               (zLastLoad ? atof(zLastLoad) : 0)
                            + rLimit/WIKI_EDIT_LOCKOUT,
               now);
    if(zLastLoad) free(zLastLoad);
  }

  /* we also need to clear any captcha cookie since having it
  ** bypasses the throttler. This is also handy since it's not
  ** IP specific, so users who are behind variable IP's are going
  ** to still get caught.
  */
  captcha_clear_cookie();
}

/*
** Apply any appropriate anti-spam heuristics to the provided wiki edit.
**
** Obviously, we're only going to apply this restriction to anonymous
** users. Currently.
**
** Returns NULL if the change is acceptable. Otherwise, it returns a string
** containing an explanation for the rejection.
*/
char *is_edit_allowed(const char *zOld, const char *zNew){
  if( g.isAnon ){
    const char *zKeys = db_config("keywords","");
    int nMscore = atoi(db_config("keywords_max_score","0"));
    int nMax = atoi(db_config("max_links_per_edit","0"));
    
    /*
    ** Check for too many "bad words" in the new text. Checking the "diff"
    ** might be better?
    */
    if( nMscore && zKeys[0] ) {
      db_add_functions();
      if( db_exists("SELECT 1 WHERE search('%q','%q')>%d",zKeys,zNew,nMscore)){
        increase_load();
        return "Forbidden keywords!";
      }
    }

    /*
    ** Check to see if the threshold of external links was exceeded.
    */
    if( nMax ){
      int nOld = count_links(zOld);
      int nNew = count_links(zNew);

      /* Note that someone could bypass this by replacing a whole bunch of
      ** links in an existing page. If that starts to happen it might be
      ** necessary to compare the list of links or something.
      **
      ** Some keyword filtering would help a bit, too.
      */
      if( nNew - nOld >= nMax ){
        increase_load();
        return "Too many external links for one edit!";
      }
    }
  }
  return 0;
}

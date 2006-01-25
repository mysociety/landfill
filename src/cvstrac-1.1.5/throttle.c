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

/*
** Check to see if there have been too many recent accesses from the
** same IP address. 
**
** If there is an overload, the resulting action depends on the exitOnOverload
** parameter.  If that parameter is true, an error reply is constructed
** and an exit performed without returning.  If the parameter is false,
** a non-zero value is returned.
**
** If there is no overload, a zero is returned.
*/
int throttle(int exitOnOverload){
  const char *zLimit;
  double rLimit;
  const char *zAddr;
  char **az;
  double rLoad;
  int lastAccess;
  time_t now;
  int overload;

  if( !g.isAnon ) return 0; /* Throttling does not occur for identified users */
  zLimit = db_config("throttle", 0);
  if( zLimit==0 ) return 0; /* Throttling is turned off */
  rLimit = atof(zLimit);
  if( rLimit<=0.0 ) return 0;  /* Throttling is turned off */
  zAddr = getenv("REMOTE_ADDR");
  if( zAddr==0 ) return 0;  /* No remote IP address provided */
  az = db_query("SELECT load, lastaccess FROM access_load WHERE ipaddr='%q'",
                zAddr);
  time(&now);
  if( az[0] && az[1] ){
    rLoad = atof(az[0]);
    lastAccess = atoi(az[1]);
    if( lastAccess>now ) lastAccess = now;
    rLoad = 1.0 + exp(DECAY_RATE*(lastAccess-now))*rLoad;
    if( rLoad>rLimit && rLoad<rLimit*2.0 ){
      /* If the throttler triggers, make sure it locks out the IP address
      ** for at least 1 hour */
      rLoad = rLimit*2.0;
    }
  }else{
    rLoad = 1.0;
    lastAccess = 0;
  }
  db_execute("REPLACE INTO access_load(ipaddr,load,lastaccess) "
             "VALUES('%q',%g,%d)", zAddr, rLoad, now);
  overload = rLoad>=rLimit;
  if( overload && exitOnOverload ){
    cgi_reset_content();
    common_standard_menu(0,0);
    common_header("Access denied");
    @ <p>Your access to this website has been temporarily suspended because
    @ you are using it excessively.  You can retry your request later.</p>
    common_footer();
    cgi_append_header("Pragma: no-cache\r\n");
    cgi_set_status(200,"OK");
    cgi_reply();
    exit(0);
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
  common_add_menu_item("stopper","I am a spider");
  common_add_menu_item("index","I am human");
  common_header("Honey Pot");
  @ <p>This page is intended to capture spiders that
  @ ignore the "robots.txt" file.
  @ If you are not a spider, click on the "I am human" link
  @ above.  If you click on the "I am a spider" link, your access to this
  @ CVSTrac server will be suspended for about an hour.</p>
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
    throttle(1);
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
  const char **az;
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
  common_add_menu_item("setup", "Main Setup Menu"); 
  common_add_menu_item("info_throttle?reset=1","Remove Older Entries");
  if( limit>0 ){
    common_add_menu_item("info_throttle?limit=-1","View All");
  }else{
    common_add_menu_item("info_throttle?limit=50","View Top 50");
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

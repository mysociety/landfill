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
** This file contains code used to browse through the CVS repository.
*/
#include "config.h"
#include "browse.h"
#include <sys/times.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

static void other_methods_footer() {
@ <hr>
@ <p>Instead of browsing above, you can get all the mySociety source code in two ways:</p>
@ <p><strong>Anonymous CVS</strong>. This is updated as we commit new changes. 
@ Set CVSROOT to <tt>:pserver:anonymous@cvs.mysociety.org:/repos</tt> 
@ and grab the module <tt>mysociety</tt>. We recommend
@ <a href="http://www.tortoisecvs.org">TortoiseCVS</a> as a CVS client on
@ Windows. On Unix or on a Mac just type
@ "<tt>cvs -d :pserver:anonymous@cvs.mysociety.org:/repos co mysociety</tt>" at
@ a terminal.</p>
@ <p><strong>HTTP download</strong>. This compressed archive is updated once a
@ day.
@ <a href="http://www.mysociety.org/cvs/mysociety.tar.gz">http://www.mysociety.org/cvs/mysociety.tar.gz</a>
@ </p>
@ <p><strong>Licence.</strong> Most of mySociety's code is made available under
@ <a href="/cvstrac/getfile/mysociety/LICENSE.txt">the Affero GPL</a>; parts
@ are available under other free licences, as noted in the source. If you want
@ to reuse some of our code, but its existing licence would prohibit you from
@ doing so, ask us about relicensing.</p>
@ <hr>
}

/*
** This routine generates an HTML page that describes the complete
** revision history for a single file.
*/
static void revision_history(const char *zName, int showMilestones){
  char **az;
  int i;
  const char *zTail;

  if( zName[0]=='/' ) zName++;  /* Be nice to TortoiceCVS */
  zTail = strrchr(zName, '/');
  if( zTail ) zTail++;

  /* @ <h2>History of /%h(zName)</h2> */
  if( showMilestones ){
    common_add_action_item(mprintf("rlog?f=%t",zName), "Omit Milestones");
    az = db_query("SELECT filechng.cn, date, vers, nins, ndel, prevvers,"
                  "       message, user, branch "
                  "FROM filechng, chng "
                  "WHERE filename='%q' AND filechng.cn=chng.cn "
                  "UNION ALL "
                  "SELECT '',date,cn,NULL,NULL,NULL,message,user,branch "
                  "FROM chng "
                  "WHERE milestone=1 "
                  "ORDER BY 2 DESC", zName);
  } else {
    common_add_action_item(mprintf("rlog?f=%t&sms=1",zName), "Show Milestones");
    az = db_query("SELECT filechng.cn, date, vers, nins, ndel, prevvers,"
                  "       message, user, branch "
                  "FROM filechng, chng "
                  "WHERE filename='%q' AND filechng.cn=chng.cn "
                  "ORDER BY date DESC", zName);
  }

  common_header("History for /%h", zName);

  @ <table cellpadding=0 cellspacing=0 border=0>
  for(i=0; az[i]; i+=9){
    time_t t;
    struct tm *pTm;
    char zDate[100];

    t = atoi(az[i+1]);
    pTm = localtime(&t);
    strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M", pTm);
    if( i==0 ){
      @ <thread><tr><th>Date</th><th width=80>Version</th>
      @         <th>Description</th></tr>
      @ <tbody>
    }
    if( i%2 ){
      @ <tr bgcolor="%s(BG4)" class="bkgnd4">
    }else{
      @ <tr>
    }
    @ <td align="right" valign="top"><nobr>%s(zDate)</nobr></td>
    if( az[i][0]==0 ){
      @ <td align="center" valign="top">
      common_icon("box");
      @ </td>
      if( az[i+8] && az[i+8][0] ){
        @ <td align="left" bgcolor="%s(BG5)" class="bkgnd5">
        @ Milestone
        output_chng(atoi(az[i+2]));
        @    on branch %h(az[i+8]):
      }else{
        @ <td align="left">
        @ Milestone
        output_chng(atoi(az[i+2]));
      }
    }else{
      @ <td valign="top" align="center">&nbsp;&nbsp;
      @ <a href="fileview?f=%T(zName)&v=%T(az[i+2])">
      @    %h(printable_vers(az[i+2]))</a>
      @ &nbsp;&nbsp;</td>
      if( az[i+8] && az[i+8][0] ){
        @ <td bgcolor="%s(BG5)" class="bkgnd5">Check-in
        output_chng(atoi(az[i]));
        @     on branch %h(az[i+8]):
      }else{
        @ <td>Check-in
        output_chng(atoi(az[i]));
        @ :
      }
    }
    output_formatted(az[i+6], 0);
    @&nbsp;By %z(format_user(az[i+7])).
    if( az[i][0]!=0 ){ /* Can't diff a Milestone */
      if( g.okCheckout && az[i+5] && az[i+5][0] ){
        @ <a href="filediff?f=%T(zName)&v1=%T(az[i+5])&v2=%T(az[i+2])">
        @ (diff)</a>
      }
    }
    @ </td></tr>
  }
  if( i==0 ){
    @ <tr><td>Nothing is known about this file</td></tr>
  }
  @ </table>
  other_methods_footer();
}

/*
** Adds all appropriate action bar links for file tools
*/
static void add_file_tools(
  const char *zExcept,
  const char *zFile,
  const char *zVers1,
  const char *zVers2
){
  int i;
  char *zLink;
  char **azTools;
  db_add_functions();
  azTools = db_query("SELECT tool.name FROM tool,user "
                     "WHERE tool.object='file' AND user.id='%q' "
                     "      AND cap_and(tool.perms,user.capabilities)!=''",
                     g.zUser);

  for(i=0; azTools[i]; i++){
    if( zExcept && 0==strcmp(zExcept,azTools[i]) ) continue;

    zLink = mprintf("filetool?t=%T&f=%T%s%T%s%T",
                          azTools[i], zFile,
                          zVers1?"&v1=":"", zVers1?zVers1:"",
                          zVers2?"&v2=":"", zVers2?zVers2:"");
    common_add_action_item(zLink, azTools[i]);
  }
}

/*
** Adds all appropriate action bar links for directory tools
*/
static void add_dir_tools( const char *zExcept, const char *zDir ){
  int i;
  char *zLink;
  char **azTools;
  db_add_functions();
  azTools = db_query("SELECT tool.name FROM tool,user "
                     "WHERE tool.object='dir' AND user.id='%q' "
                     "      AND cap_and(tool.perms,user.capabilities)!=''",
                     g.zUser);

  for(i=0; azTools[i]; i++){
    if( zExcept && 0==strcmp(zExcept,azTools[i]) ) continue;

    zLink = mprintf("dtool?t=%T&d=%T", azTools[i], zDir);
    common_add_action_item(zLink, azTools[i]);
  }
}

/*
** WEBPAGE: /dtool
**
** Execute an external tool on a given directory
*/
void dirtool(void){
  const char *zDir = PD("d","");
  const char *zTool = P("t");
  char *zDirUrl;
  char *zAction;
  const char *azSubst[32];
  int n = 0;

  if( zDir==0 || zTool==0 ) cgi_redirect("index");

  login_check_credentials();
  if( !g.okCheckout ){ login_needed(); return; }
  throttle(1,0);
  history_update(0);

  zDirUrl = mprintf("%T?d=%T", default_browse_url(), zDir);

  zAction = db_short_query("SELECT command FROM tool "
                           "WHERE name='%q'", zTool);
  if( zAction==0 || zAction[0]==0 ) cgi_redirect(zDirUrl);

  common_standard_menu(0, "search?f=1");
  common_add_action_item(zDirUrl,"Directory");

  add_dir_tools(zTool,zDir);

  common_header("%s for /%h", zTool, zDir);

  azSubst[n++] = "F";
  azSubst[n++] = quotable_string(zDir);
  azSubst[n++] = 0;

  n = execute_tool(zTool,zAction,0,azSubst);
  free(zAction);
  if( n<=0 ){
    cgi_redirect(zDirUrl);
  }
  common_footer();
}

/*
** WEBPAGE: /filetool
**
** Execute an external tool on a given target
*/
void filetool(void){
  const char *zFile = P("f");
  const char *zVers1 = PD("v1","");
  const char *zVers2 = PD("v2","");
  const char *zTool = P("t");
  char *zAction;
  const char *azSubst[32];
  int n = 0;

  if( zFile==0 || zTool==0 ) cgi_redirect("index");

  login_check_credentials();
  if( !g.okCheckout ){ login_needed(); return; }
  throttle(1,0);
  history_update(0);

  zAction = db_short_query("SELECT command FROM tool "
                           "WHERE name='%q'", zTool);
  if( zAction==0 || zAction[0]==0 ) cgi_redirect("index");

  common_standard_menu(0, "search?f=1");
  common_add_action_item(mprintf("rlog?f=%T", zFile), "History");
  add_file_tools(zTool,zFile,zVers1,zVers2);

  common_header("%s for /%h", zTool, zFile);

  @ <a href="rlog?f=%T(zFile)">%h(zFile)</a>
  if( zVers1 ){
    char *zFV = mprintf("fileview?f=%T&v=%T", zFile, zVers1);
    @ <a href="%T(zFV)">%h(zVers1)</a><hr>
  }

  azSubst[n++] = "F";
  azSubst[n++] = quotable_string(zFile);
  azSubst[n++] = "V1";
  azSubst[n++] = quotable_string(zVers1);
  azSubst[n++] = "V2";
  azSubst[n++] = quotable_string(zVers2);
  azSubst[n++] = 0;

  n = execute_tool(zTool,zAction,0,azSubst);
  free(zAction);
  if( n<=0 ){
    cgi_redirect(mprintf("rlog?f=%T",zFile));
  }
  common_footer();
}

/*
** WEBPAGE: /rlog
**
** This page lists the revision history for a single file.   Hyperlinks
** allow the file to be diffed or annotated.
*/
void browse_rlog(void){
  char *zDir, *z;
  int showMilestones;
  const char *zFile;

  login_check_credentials();
  if( !g.okCheckout ){ login_needed(); return; }
  throttle(1,0);
  common_standard_menu(0, "search?f=1");
  showMilestones = atoi(PD("sms","0"));
  history_update(0);
  zFile = PD("f","");
  /* Make sure we always have '/' in zFile, otherwise link to parent
  ** directory won't work for file in repository root.
  */
  if( strrchr(zFile, '/') ){
    zDir = mprintf("%T?d=%T", default_browse_url(), zFile);
  }else{
    zDir = mprintf("%T?d=/%T", default_browse_url(), zFile);
  }
  z = strrchr(zDir, '/' );
  if( z ){ *z = 0;}
  common_add_action_item(zDir, "Directory");
  add_file_tools(0,zFile,0,0);
  common_add_help_item("CvstracFileHistory");
  revision_history(zFile, showMilestones);
  common_footer();
}

/*
** WEBPAGE: /filediff
**
** Show the differences between two versions of a file
*/
void browse_filediff(void){
  const char *zFile = P("f");
  const char *zV1 = P("v1");
  const char *zV2 = P("v2");

  login_check_credentials();
  if( !g.okCheckout ){ login_needed(); return; }
  throttle(1,0);
  if( zFile==0 || zV1==0 || zV2==0 ){ cgi_redirect("index"); return; }
  common_standard_menu(0, "search?f=1");
  common_add_action_item(mprintf("rlog?f=%T", zFile), "History");
  add_file_tools(0,zFile,zV1,zV2);
  common_add_help_item("CvstracFileHistory");
  common_header("Difference in %h versions %h and %h", zFile,
      printable_vers(zV1), printable_vers(zV2));
  if( diff_versions(zV1, zV2, zFile) ){
    @ <b>Diff failed!</b>
  }
  other_methods_footer();
  common_footer();
}

/*
** WEBPAGE: /dir
**
** List all of the repository files in a single directory.
*/
void browse_dir(void){
  const char *zName;
  char *zDir;
  char *zBase;
  char **az;
  int i, j;
  int n;
  int nRow;
  const char *zCookieName;
  int nCookieLife;

  login_check_credentials();
  if( !g.okCheckout ){ login_needed(); return; }
  throttle(1,0);
  history_update(0);
  common_standard_menu("dir", "search?f=1");
  /* P("sc") is set only when user explicitly switches to Long/Short view,
  ** via action bar link. In that case we make that users preference
  ** persistent via cookie.
  */
  if( P("sc") ){
    zCookieName = mprintf("%t_browse_url",g.zName);
    nCookieLife = 86400*atoi(db_config("browse_url_cookie_life","90"));
    if( nCookieLife ){
      cgi_set_cookie(zCookieName, "dir", 0, nCookieLife);
    }
  }
  zName = PD("d","");
  if( zName==0 ){
    zName = "";
  }
  common_add_help_item("CvstracBrowse");
  if( zName[0] ){
    common_add_action_item(
      mprintf("timeline?x=1&c=2&dm=1&px=%h",zName),
      "Activity"
    );
  }
  add_dir_tools(0,zName);
  zDir = mprintf("%s", zName);
  zBase = strrchr(zDir, '/');
  if( zBase==0 ){
    zBase = zDir;
    zDir = "";
  }else{
    *zBase = 0;
    zBase++;
  }
  if( zName && zName[0] ){
    /* this looks like navigation, but it's relative to the current page
    */
    common_add_action_item("dir", "Top");
    common_add_action_item(mprintf("dir?d=%T",zDir), "Up");
    common_add_action_item(mprintf("dirview?d=%T&sc=1",zName), "Long");
  }else{
    common_add_action_item("dirview?sc=1","Long");
  }
  az = db_query("SELECT base, isdir FROM file WHERE dir='%q' ORDER BY base",
                 zName);
  for(n=0; az[n*2]; n++){}
  if( zName[0] ) n++;
  nRow = (n+3)/4;
  if( zName[0] ){ zName = mprintf("%s/",zName); }
  common_header("Directory /%h", zName);
  /* @ <h2>Contents of directory /%h(zName)</h2> */
  @ <table width="100%%">
  @ <tr>
  for(i=j=0; i<4; i++){
    @ <td valign="top" width="25%%">
    n = 0;
    if( i==0 && zName[0] ){
      @ <a href="dir?d=%T(zDir)">
      common_icon("backup");
      @ </a>&nbsp;<a href="dir?d=%T(zDir)">..</a><br>
      n++;
    }
    while( n<nRow && az[j] ){
      if( atoi(az[j+1]) ){
        @ <a href="dir?d=%T(zName)%T(az[j])">
        common_icon("dir");
        @ </a>&nbsp;<a href="dir?d=%T(zName)%T(az[j])">%h(az[j])/</a><br>
      }else{
        char *zIcon;
        char *zFilename = mprintf("%s%s", zName, az[j]);
        if(is_file_available(zFilename)){
          zIcon = "file";
        }else{
          zIcon = "del";
        }
        if( zFilename!=0 ) free(zFilename);
        @ <a href="rlog?f=%T(zName)%T(az[j])">
        common_icon(zIcon);
        @ </a>&nbsp;<a href="rlog?f=%T(zName)%T(az[j])">%h(az[j])</a><br>
      }
      n++;
      j += 2;
    }
    @ </td>
  }
  @ </tr></table>
  other_methods_footer();
  common_footer();
}

/*
** This routine is used to represent age of files in english text.
** For example "1 week", "3 days", etc.
** It takes integer representing unix time of file's last modification and
** calculates it's age relative to current time.
*/
static char *file_age_to_text(int nModified){
  int nAge, n;
  int nYear  = 31536000; /* Number of seconds in a year */
  int nMonth =  2592000; /* Number of seconds in a month */
  int nWeek  =   604800; /* Number of seconds in a week */
  int nDay   =    86400; /* Number of seconds in a day */
  
  if( nModified<=0 ){
    /* FIXME: some error handling would be nice here */
    return NULL;
  }
  
  nAge = (int)time(0)-nModified;
  if( nAge<0 ){
    /* FIXME: some error handling would be nice here */
    return NULL;
  }
  
  if( (n = nAge/nYear)>1 ){
    return mprintf("%d years", n);
  }else if( (n = nAge/nMonth)>1 ){
    return mprintf("%d months", n);
  }else if( (n = nAge/nWeek)>1 ){
    return mprintf("%d weeks", n);
  }else if( (n = nAge/nDay)>1 ){
    return mprintf("%d days", n);
  }else if( (n = nAge/3600)>1 ){
    return mprintf("%d hours", n);
  }else{
    n = nAge/60;
    if( n<=1 ){
      return mprintf("1 minute");
    }else{
      return mprintf("%d minutes", n);
    }
  }
}

static void column_header(
  const char *zNameNS,
  char zFld,
  const char *zField,
  const char *zColumn
){
  int set = (zFld==zField[0]);
  int desc = P("desc")!=0;
  const char *zDesc = set ? (desc ? "" : "&desc" ) : "";

  /* Clicking same column header 3 times in a row resets any sorting.
  */
  if(set && desc){
    @ <th align="left" bgcolor="%s(BG1)" class="bkgnd1">
    @   <a href="dirview?d=%T(zNameNS)">%h(zColumn)</a></th>
    return;
  }
  if(set){
    @ <th align="left" bgcolor="%s(BG1)" class="bkgnd1"><a
  }else{
    @ <th align="left"><a
  }
  @     href="dirview?d=%T(zNameNS)&o=%s(zField)%s(zDesc)">%h(zColumn)</a></th>
}

/*
** Output a long directory row
*/
static void row_content(
  const char *zName,
  const char *zSortUrl,
  int nCol,
  char **az
){
  if( (nCol%2)==0 ){
    @ <tr bgcolor="%s(BG4)" class="bkgnd4">
  }else{
    @ <tr>
  }
  
  if( atoi(az[0])==1 ){
    @ <td colspan="3">
    @ <a href="dirview?d=%T(zName)%T(az[1])&%s(zSortUrl)">
    common_icon("dir");
    @ </a>&nbsp;<a href="dirview?d=%T(zName)%T(az[1])&%s(zSortUrl)">%h(az[1])/</a></td>
    @ <td valign="middle" width="10%%">%h(file_age_to_text(atoi(az[5])))</td>
    @ <td></td>
  }else{
    @ <td valign="middle" width="30%%">
    @ <a href="rlog?f=%T(zName)%T(az[1])">
    if( atoi(az[3])==2 ){
      common_icon("del");
    }else{
      common_icon("file");
    }
    @ </a>&nbsp;<a href="rlog?f=%T(zName)%T(az[1])">%h(az[1])</a></td>
    @ <td valign="middle" width="5%%">
    @ <a href="fileview?f=%T(zName)%T(az[1])&v=%T(az[2])">
    @ %h(printable_vers(az[2]))</a></td> 
    @ <td valign="middle" width="8%%">%z(format_user(az[4]))</td>
    @ <td valign="middle" width="8%%">%h(file_age_to_text( atoi(az[5]) ))</td>
    @ <td valign="middle">
    
    if( output_trim_message(az[6], MN_CKIN_MSG, MX_CKIN_MSG) ){
      output_formatted(az[6], 0);
      @ &nbsp;[...]
    }else{
      output_formatted(az[6], 0);
    }
    @ </td>
  }
  @</tr>
}

/*
** WEBPAGE: /dirview
**
** This is a "long view" version of /dir page.
** List all of the repository files in a single directory and display 
** information about their last change.
*/
void browse_dirview(void){
  const char *zName;
  const char *zNameNS; /* NoSlash */
  char *zDir;
  char *zBase;
  char **az;
  int i;
  const char *zCookieName;
  int nCookieLife;
  char *zDesc;
  char *zOrderBy = "1 DESC, 2";
  const char *z;
  char zFld = 0;
  char *zSortUrl = "";

  login_check_credentials();
  if( !g.okCheckout ){ login_needed(); return; }
  throttle(1,0);
  history_update(0);
  common_standard_menu("dirview", "search?f=1");
  /* P("sc") is set only when user explicitly switches to Long/Short view,
  ** via action bar link. In that case we make that users preference
  ** persistent via cookie.
  */
  if( P("sc") ){
    zCookieName = mprintf("%t_browse_url",g.zName);
    nCookieLife = 86400*atoi(db_config("browse_url_cookie_life","90"));
    if( nCookieLife ){
      cgi_set_cookie(zCookieName, "dirview", 0, nCookieLife);
    }
  }
  zName = PD("d","");
  if( zName==0 ){
    zName = "";
  }
  if( zName[0] ){
    common_add_action_item(
      mprintf("timeline?x=1&c=2&dm=1&px=%T",zName),
      "Activity"
    );
  }
  add_dir_tools(0,zName);
  zDir = mprintf("%s", zName);
  zBase = strrchr(zDir, '/');
  if( zBase==0 ){
    zBase = zDir;
    zDir = "";
  }else{
    *zBase = 0;
    zBase++;
  }

  /* Figure out how should we order this and display our intent in <th>
  ** If no ordering preference is found, don't display anything in <th>
  */
  zDesc = P("desc") ? "DESC" : "ASC";
  
  z = P("o");
  if( z ){
    zSortUrl = mprintf("o=%t%s", z, (zDesc[0]=='D')?"&desc":"");
    zFld = z[0];
    switch( zFld ){
      case 'f':
        zOrderBy = mprintf("2 %s", zDesc);
        break;
      case 'v':
        zOrderBy = mprintf("3 %s", zDesc);
        break;
      case 'u':
        zOrderBy = mprintf("5 %s", zDesc);
        break;
      case 'd':
        zOrderBy = mprintf("6 %s", (strcmp(zDesc,"ASC")==0)?"DESC":"ASC");
        break;
      case 'm':
        zOrderBy = mprintf("7 %s", zDesc);
        break;
      default:
        zFld = 0;
        break;
    }
  }

  if( zName && zName[0] ){
    /* this looks like navigation, but it's relative to the current page
    */
    common_add_action_item(
      mprintf("dirview%s%s",(zSortUrl[0])?"?":"",zSortUrl), "Top");
    common_add_action_item(
      mprintf("dirview?d=%T%s%s",zDir,(zSortUrl[0])?"&":"",zSortUrl), "Up");
    common_add_action_item(mprintf("dir?d=%T&sc=1",zName), "Short");
  }else{
    common_add_action_item("dir?sc=1", "Short");
  }
  common_add_help_item("CvstracBrowse");
  
  zNameNS = mprintf("%s",zName);
  if( zName[0] ){ zName = mprintf("%s/",zName); }
  
  db_add_functions();
  az = db_query(
    "SELECT 0, f.base, fc.vers, fc.chngtype, c.user, c.date, "
    "       '[' || f.lastcn || '] ' || c.message, f.lastcn "
    "FROM file f, chng c, filechng fc "
    "WHERE f.dir='%q' "
    "  AND f.isdir=0 "
    "  AND fc.filename=path(isdir,dir,base) "
    "  AND f.lastcn=fc.cn "
    "  AND f.lastcn=c.cn "
    "UNION ALL "
    "SELECT 1, f.base, NULL, NULL, NULL, c.date, "
    "       NULL, f.lastcn "
    "FROM file f, chng c "
    "WHERE f.dir='%q' "
    "  AND f.isdir=1 "
    "  AND f.lastcn=c.cn "
    "ORDER BY %s",
    zNameNS, zNameNS, zOrderBy
  );
  
  common_header("Directory /%h", zName);
  @ <table width="100%%" border=0 cellspacing=0 cellpadding=3>
  @ <tr>
  column_header(zNameNS,zFld,"file","File");
  column_header(zNameNS,zFld,"vers","Vers");
  column_header(zNameNS,zFld,"user","By");
  column_header(zNameNS,zFld,"date","Age");
  column_header(zNameNS,zFld,"msg","Check-in");
  @ </tr>

  if( zName[0] ){
    @ <tr><td colspan="5">
    @ <a href="dirview?d=%T(zDir)&%s(zSortUrl)">
    common_icon("backup");
    @ </a>&nbsp;<a href="dirview?d=%T(zDir)&%s(zSortUrl)">..</a></td></tr>
  }
  
  /* In case dir is empty, exit nicely */
  if( !az || !az[0] ){
    @ </table>
    common_footer();
    return;
  }
  
  for(i=0; az[i]; i+=8){
    row_content(zName,zSortUrl,i/8,&az[i]);
  }
  @ </table>
  db_query_free(az);
  common_footer();
}

/*
** WEBPAGE: /fileview
**
** Show the file in a HTML page. In the case of things like images, show the
** content embedded in the page.
*/
void browse_fileview(void){
  const char *zFile = g.zExtra ? g.zExtra : P("f");
  const char *zVers = PD("v","");
  char *zGetFile;
  char *zDir, *z;
  char *zSuffix;
  char *zMime = "text/plain";  /* The default MIME type */

  /* The following table lists some alternative MIME types based on
  ** the file suffix
  */
  static const struct {
    char *zSuffix;
    char *zMime;
  } aMime[] = {
    { "html",  "text/html" },
    { "htm",   "text/html" },
    { "gif",   "image/gif" },
    { "jpeg",  "image/jpeg" },
    { "jpg",   "image/jpeg" },
    { "png",   "image/png" },
    { "pdf",   "application/pdf" },
    { "ps",    "application/postscript" },
    { "eps",   "application/postscript" },
  };

  login_check_credentials();
  if( !g.okCheckout ){ login_needed(); return; }
  throttle(1,0);
  common_standard_menu(0, "search?f=1");
  history_update(0);

  /* Make sure we always have '/' in zFile, otherwise link to parent
  ** directory won't work for file in repository root.
  */
  if( strrchr(zFile, '/') ){
    zDir = mprintf("%T?d=%T", default_browse_url(), zFile);
  }else{
    zDir = mprintf("%T?d=/%T", default_browse_url(), zFile);
  }
  z = strrchr(zDir, '/' );
  if( z ){ *z = 0;}
  common_add_nav_item(zDir, "Directory");

  zGetFile = mprintf("getfile?f=%T&v=%T", zFile, zVers);
  common_add_action_item(zGetFile, "Raw");
  add_file_tools(0,zFile,zVers,0);

  common_add_help_item("CvstracFileview");
  common_header("%h %h", zFile, printable_vers(zVers));

  /* sort out the MIME type. We output HTML, but some things are embeddable. */
  zSuffix = strrchr(zFile, '.');
  if( zSuffix ){
    char zLine[2000];
    int i;
    zSuffix++;
    for(i=0; zSuffix[i] && i<sizeof(zLine)-1; i++){
      zLine[i] = tolower(zSuffix[i]);
    }
    zLine[i] = 0;
    for(i=0; i<sizeof(aMime)/sizeof(aMime[0]); i++){
      if( strcmp(zLine, aMime[i].zSuffix)==0 ){
        zMime = aMime[i].zMime;
        break;
      }
    }
  }

  @ <a href="rlog?f=%T(zFile)">%h(zFile)</a>
  @ <a href="%s(zGetFile)">%h(zVers)</a><hr>

  /* For image types, embed in the page. Anything else, try to inline */
  if( !strncmp(zMime,"image/",6) ){
    @ <img src="%s(zGetFile)" alt="%h(zFile) %h(zVers)">
  }else{
    if( dump_version(zVers,zFile,0) ){
      cgi_redirect("index");
      return;
    }
  }

  @ <hr>
  common_footer();
}

/*
** WEBPAGE: /getfile
**
** Return the complete content of a file
*/
void browse_getfile(void){
  const char *zFile = g.zExtra ? g.zExtra : P("f");
  const char *zVers = P("v");
  char *zSuffix;
  const char *zName;
  char *zMime = "text/plain";  /* The default MIME type */

  /* The following table lists some alternative MIME types based on
  ** the file suffix
  */
  static const struct {
    char *zSuffix;
    char *zMime;
  } aMime[] = {
    { "html",  "text/html" },
    { "htm",   "text/html" },
    { "gif",   "image/gif" },
    { "jpeg",  "image/jpeg" },
    { "jpg",   "image/jpeg" },
    { "png",   "image/png" },
    { "pdf",   "application/pdf" },
    { "ps",    "application/postscript" },
    { "eps",   "application/postscript" },
  };

  login_check_credentials();
  if( !g.okCheckout || zFile==0 ){ login_needed(); return; }
  throttle(1,0);

  if( zVers!= 0 ){
    /* A database query is almost definitely going to be faster than having
    ** to pull from from the repository, so we might as well try this first.
    */
    char *z = db_short_query("SELECT chng.date FROM filechng, chng "
                             "WHERE filechng.filename='%q' "
                             "      AND filechng.vers='%q' "
                             "      AND filechng.cn=chng.cn ",
                             zFile, zVers);
    if( z ){
      cgi_modified_since(atoi(z));
      cgi_append_header(mprintf("Last-Modified: %h\r\n",
                        cgi_rfc822_datestamp(atoi(z))));
      free(z);
    }
  }

  if( dump_version(zVers,zFile,1) ){
    cgi_redirect("index");
    return;
  }

  /* sort out the MIME type */
  zSuffix = strrchr(zFile, '.');
  if( zSuffix ){
    char zLine[2000];
    int i;
    zSuffix++;
    for(i=0; zSuffix[i] && i<sizeof(zLine)-1; i++){
      zLine[i] = tolower(zSuffix[i]);
    }
    zLine[i] = 0;
    for(i=0; i<sizeof(aMime)/sizeof(aMime[0]); i++){
      if( strcmp(zLine, aMime[i].zSuffix)==0 ){
        zMime = aMime[i].zMime;
        break;
      }
    }
  }

  /*
  ** This means the user gets something meaningful as a default filename
  ** when they try to save to file (depending on the browser).
  */
  zName = strrchr(zFile, '/');
  if (zName) zName += 1;
  cgi_append_header(mprintf("Content-disposition: attachment; "
        "filename=\"%T\"\r\n", zName ? zName : zFile));

  if( zVers && zVers[0] ){
    g.isConst = 1;
  }
  cgi_set_content_type(zMime);
  cgi_set_status(200, "OK");
  return;
}

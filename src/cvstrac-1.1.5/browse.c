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
@ and grab the module <tt>mysociety</tt>. We recommend <a href="http://www.tortoisecvs.org">TortoiseCVS</a>
@ as a CVS client on Windows. On Unix or on a Mac just type 
@ "<tt>cvs -d :pserver:anonymous@cvs.mysociety.org:/repos co mysociety</tt>" at a terminal.
@ </p>
@ <p><strong>HTTP download</strong>. This compressed archive is updated once a day.
@ <a href="http://www.mysociety.org/cvs/mysociety.tar.gz">http://www.mysociety.org/cvs/mysociety.tar.gz</a>
@ </p>
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
  common_header("History for /%s", zName);
  /* @ <h2>History of /%h(zName)</h2> */
  if( showMilestones ){
    @ <p><a href="rlog?f=%t(zName)&amp;sms=0">Omit Milestones</a></p>
    az = db_query("SELECT filechng.cn,date,vers,nins,ndel,message,user,branch "
                  "FROM filechng, chng "
                  "WHERE filename='%q' AND filechng.cn=chng.cn "
                  "UNION ALL "
                  "SELECT '',date,cn,NULL,NULL,message,user,branch "
                  "FROM chng "
                  "WHERE milestone=1 "
                  "ORDER BY 2 DESC", zName);
  } else {
    @ <p><a href="rlog?f=%t(zName)&amp;sms=1">Show Milestones</a></p>
    az = db_query("SELECT filechng.cn,date,vers,nins,ndel,message,user,branch "
                  "FROM filechng, chng "
                  "WHERE filename='%q' AND filechng.cn=chng.cn "
                  "ORDER BY date DESC", zName);
  }
  @ <table cellpadding=0 cellspacing=0 border=0>
  for(i=0; az[i]; i+=8){
    time_t t;
    struct tm *pTm;
    char zDate[100];
    char zPriorVers[100];

    t = atoi(az[i+1]);
    pTm = localtime(&t);
    strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M", pTm);
    if( i==0 ){
      @ <tr><th>Date</th><th width=80>Version</th><th>Description</th></tr>
    }
    @ <tr>
    @ <td align="right" valign="center"><nobr>%s(zDate)</nobr></td>
    if( az[i][0]==0 ){
      @ <td align="center" valign="top"><img src="box.gif"></td>
      if( az[i+7] && az[i+7][0] ){
        @ <td align="left" bgcolor="#dddddd">
        @ Milestone <a href="chngview?cn=%s(az[i+2])">[%s(az[i+2])]</a>
        @    on branch %h(az[i+7]): %h(az[i+5])
      }else{
        @ <td align="left">
        @ Milestone <a href="chngview?cn=%s(az[i+2])">[%s(az[i+2])]</a>:
        @    %h(az[i+5])
      }
    }else{
      @ <td valign="top" align="center">&nbsp;&nbsp;
      @ <a href="getfile/%T(zName)?v=%T(az[i+2])">%h(az[i+2])</a>
      @ &nbsp;&nbsp;</td>
      if( az[i+7] && az[i+7][0] ){
        @ <td bgcolor="#dddddd">Check-in <a href="chngview?cn=%s(az[i])">
        @     [%s(az[i])]</a> on branch %h(az[i+7]):
      }else{
        @ <td>Check-in <a href="chngview?cn=%s(az[i])">[%s(az[i])]</a>:
      }
      output_formatted(az[i+5], 0);
      @ 
      @ By %h(az[i+6]).
      sprintf(zPriorVers,"%.*s", (int)sizeof(zPriorVers)-2, az[i+2]);
      previous_version(zPriorVers);
      if( g.okCheckout && zPriorVers[0] ){
        @ <a href="filediff?f=%T(zName)&v1=%s(zPriorVers)&v2=%s(az[i+2])">
        @ (diff)</a>
      }
    }
    @ </td></tr>
  }
  if( i==0 ){
    @ <tr><td>Nothing is known about this file</td></tr>
  }
  @ </table>
}

/*
** WEBPAGE: /rlog
**
** This page lists the revision history for a single file.   Hyperlinks
** allow the file to be diffed or annotated.
*/
void browse_fileview(void){
  char *zDir, *z;
  int showMilestones;
  const char *zFile;

  login_check_credentials();
  if( !g.okCheckout ){ login_needed(); return; }
  throttle(1);
  common_standard_menu(0, 0);
  showMilestones = atoi(PD("sms","0"));
  history_update(0);
  zFile = PD("f","");
  zDir = mprintf("dir?d=%T", zFile);
  z = strrchr(zDir, '/' );
  if( z ){ *z = 0;}
  common_add_menu_item(zDir, "Directory");
  revision_history(zFile, showMilestones);
}

/*
** Given the name of a file relative to the repository root,
** return the complete pathname of the file.
*/
static char *real_path_name(const char *zPath){
  char *zName, *zBase, *zDir;
  char *zReal;
  const char *zRoot;
  int i, j;

  zRoot = db_config("cvsroot", 0);
  if( zRoot==0 ){ return 0; }
  zName = mprintf("%s", zPath);
  for(i=j=0; zName[i]; i++){
    if( zName[i]=='/' ){
      while( zName[i+1]=='/' ){ i++; }
      if( zName[i+1]==0 ) break;
    }
    zName[j++] = zName[i];
  }
  zName[j] = 0;
  zDir = mprintf("%s/%s", zRoot, zName);
  zBase = strrchr(zDir, '/');
  if( zBase==0 ){
    zBase = zDir;
    zDir = ".";
  }else{
    *zBase = 0;
    zBase++;
  }
  zReal = find_repository_file(zDir, zBase);
  free(zName);
  free(zDir);
  return zReal;
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
  char *zReal;

  login_check_credentials();
  if( !g.okCheckout ){ login_needed(); return; }
  throttle(1);
  if( zFile==0 || zV1==0 || zV2==0 ){ cgi_redirect("index"); return; }
  zReal = real_path_name(zFile);
  if( zReal==0 ){ 
    common_err("No file named \"%h\" exists in the CVS repository. "
       "Somebody must have manually renamed or removed that file.",
       zFile);
  }
  common_standard_menu(0, 0);
  common_add_menu_item(mprintf("rlog?f=%T", zFile), "History");
  common_header("Difference in %s versions %s and %s", zFile, zV1, zV2);
  /* @ <h2>Difference in %h(zFile) versions %h(zV1) and %h(zV2)</h2> */
  diff_versions(zV1, zV2, zReal);
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
  const char *zRoot;

  login_check_credentials();
  if( !g.okCheckout ){ login_needed(); return; }
  throttle(1);
  history_update(0);
  zName = PD("d","");
  if( zName==0 ){
    zName = "";
  }else{
    common_replace_menu_item(
      mprintf("timeline?x=1&c=2&dm=1&px=%h",zName),
      "Timeline"
    );
  }
  zDir = mprintf("%s", zName);
  zBase = strrchr(zDir, '/');
  common_standard_menu("dir", 0);
  if( zBase==0 ){
    zBase = zDir;
    zDir = "";
  }else{
    *zBase = 0;
    zBase++;
    common_add_menu_item(mprintf("dir?d=%T",zDir), "Up");
  }
  az = db_query("SELECT base, isdir FROM file WHERE dir='%q' ORDER BY base",
                 zName);
  for(n=0; az[n*2]; n++){}
  if( zName[0] ) n++;
  nRow = (n+3)/4;
  if( zName[0] ){ zName = mprintf("%s/",zName); }
  zRoot = db_config("cvsroot","");
  common_header("Directory /%s", zName);
  /* @ <h2>Contents of directory /%h(zName)</h2> */
  @ <table width="100%%">
  @ <tr>
  for(i=j=0; i<4; i++){
    @ <td valign="top" width="25%%">
    n = 0;
    if( i==0 && zName[0] ){
      @ <a href="dir?d=%T(zDir)">
      @ <img src="backup.gif" width=20 height=22 align="middle" border=0
      @  alt="up"></a>&nbsp;<a href="dir?d=%T(zDir)">..</a><br>
      n++;
    }
    while( n<nRow && az[j] ){
      if( atoi(az[j+1]) ){
        @ <a href="dir?d=%T(zName)%T(az[j])">
        @ <img src="dir.gif" width=20 height=22 align="middle" border=0
        @  alt="dir"></a>&nbsp;<a
        @  href="dir?d=%T(zName)%T(az[j])">%h(az[j])/</a><br>
      }else{
        char *zIcon;
        char *zFilename = 0;
        if( zRoot!=0 
         && (zFilename = mprintf("%s/%s%s,v", zRoot, zName, az[j]))!=0
         && access(zFilename,0)==0 ){
          zIcon = "file.gif";
        }else{
          zIcon = "del.gif";
        }
        if( zFilename!=0 ) free(zFilename);
        @ <a href="getfile/%T(zName)%T(az[j])">
        @ <img src="%s(zIcon)" width=20 height=22 align="middle" border=0
        @  alt="file"></a>&nbsp;<a
        @  href="rlog?f=%T(zName)%T(az[j])">%h(az[j])</a><br>
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
** WEBPAGE: /getfile
**
** Return the complete content of a file
*/
void browse_getfile(void){
  const char *zFile = g.zExtra ? g.zExtra : P("f");
  const char *zVers = P("v");
  char *zDir, *zBase;
  char *zReal;
  const char *zRoot;
  char *zCmd;
  FILE *in;
  char zLine[2000];
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
  if( !g.okCheckout || zFile==0 ){ login_needed(); return; }
  throttle(1);
  zRoot = db_config("cvsroot", "");
  zDir = mprintf("%s/%s", zRoot, zFile);
  zBase = strrchr(zDir, '/');
  if( zBase ){ 
    *zBase = 0;
    zBase++;
  }else{
    zBase = zDir;
    zDir = "";
  }
  zReal = find_repository_file(zDir, zBase);
  if( zReal==0 ){ cgi_redirect("index"); return; }
  zCmd = mprintf("co -q '-p%s' '%s'", 
    quotable_string(zVers), quotable_string(zReal));
  in = popen(zCmd, "r");
  if( in==0 ){ cgi_redirect("index"); return; }
  while( !feof(in) ){
    int amt = fread(zLine, 1, sizeof(zLine), in);
    if( amt<=0 ) break;
    cgi_append_content(zLine, amt);
  }
  pclose(in);
  zSuffix = strrchr(zFile, '.');
  if( zSuffix ){
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
  cgi_set_content_type(zMime);
  cgi_set_status(200, "OK");
  return;
}

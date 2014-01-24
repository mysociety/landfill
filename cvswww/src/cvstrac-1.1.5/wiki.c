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
** This file contains code used to generated the Wiki pages
*/
#include "config.h"
#include "wiki.h"
#include <time.h>

/*
** Expand a wiki page name by adding a single space before each
** capital letter after the first.  The returned string is written
** into space obtained from malloc().
*/
char *wiki_expand_name(const char *z){
  int i, n;
  char *zOut;
  for(n=i=0; z[i]; i++, n++){
    if( isupper(z[i]) ) n++;
  }
  zOut = malloc(n+1);
  if( zOut==0 ) return "<out of memory>";
  for(n=i=0; z[i]; i++, n++){
    if( n>0 && isupper(z[i]) ){ zOut[n++] = ' '; }
    zOut[n] = z[i];
  }
  zOut[n] = 0;
  return zOut;
}

/*
** Write a string in zText into a temporary file.  Write the name of
** the temporary file in zFile.  Return 0 on success and 1 if there is
** any kind of error.
*/
static int write_to_temp(const char *zText, char *zFile){
  extern int sqliteOsTempFileName(char*);
  FILE *f;
  if( sqliteOsTempFileName(zFile) ){ zFile[0] = 0; return 1; }
  f = fopen(zFile, "w");
  if( f==0 ){ zFile[0] = 0;  return 1; }
  fwrite(zText, 1, strlen(zText), f);
  fprintf(f, "\n");
  fclose(f);
  return 0;
}

/*
** WEBPAGE: /wiki
**
** View a single page of wiki.
**
** Query parameters are "p" and "t".  "p" is the name of the page to
** view.  If "p" is omitted, the "WikiIndex" page is shown.  "t" is
** the time (seconds since 1970) that determines which page to view.
** If omitted, the current time is substituted for "t".
**
** A history of all versions of the page is displayed if the "t"
** parameter is present and is omitted if absent.
*/
void wiki_page(void){
  const char *pg = P("p");
  const char *zTime = P("t");
  int doDiff = atoi(PD("diff","0"));
  int tm;
  int i;
  char **azPage;               /* Query result: page to display */
  char **azHist = 0;           /* Query result: history of the page */
  int isLocked;
  char *zTimeFmt;              /* Human readable translation of "t" parameter */
  char *zTruncTime = 0;
  char *zTruncTimeFmt = 0;
  int truncCnt = 0;
  int overload;

  login_check_credentials();
  if( !g.okRdWiki ){ login_needed(); return; }
  overload = throttle(0);
  if( overload ){
    zTime = 0;
    doDiff = 0;
  }
  db_add_functions();
  if( zTime==0 || (tm = atoi(zTime))==0 ){
    time_t now;
    time(&now);
    tm = now;
  }
  if( pg==0 || is_wiki_name(pg)!=strlen(pg) ){
    pg = "WikiIndex";
  }
  azPage = db_query(
    "SELECT -invtime, locked, who, ipaddr, text "
    "FROM wiki WHERE name='%s' AND invtime>=%d LIMIT 2", pg, -tm
  );
  if( azPage[0]==0 || azPage[5]==0 ){ doDiff = 0; }
  if( zTime && !doDiff ){
    zTimeFmt = db_short_query("SELECT ldate(%d)",tm);
    azHist = db_query(
        "SELECT ldate(-invtime), who, -invtime FROM wiki "
        "WHERE name='%s'", pg
    );
  }
  isLocked = azPage[0] && atoi(azPage[1])!=0;

  if( pg && strcmp(pg,"WikiIndex")!=0 ){
    common_standard_menu(0, "search?w=1");
  }else{
    common_standard_menu("wiki", "search?w=1");
  }
  common_add_menu_item("wikitoc", "Contents");
  if( azPage[0] && g.okWiki && attachment_max()>0 && !overload ){
    common_add_menu_item( mprintf("attach_add?tn=%s",pg), "Attach");
  }
  if( zTime==0 && (g.okAdmin || (g.okWiki && !isLocked)) && !overload ){
    common_add_menu_item( mprintf("wikiedit?p=%s", pg), "Edit");
  }
  if( zTime==0 && azPage[0] && azPage[5] && !overload ){
    common_add_menu_item(mprintf("wiki?p=%s&t=%s", pg, azPage[0]), "History");
  }
  if( doDiff ){
    common_add_menu_item(mprintf("wiki?p=%s&t=%s",pg,azPage[0]), "No-Diff");
  }else if( azPage[0] && azPage[5] ){
    common_add_menu_item(mprintf("wiki?p=%s&t=%s&diff=1",pg,azPage[0]),"Diff");
  }
  if( g.okAdmin && azPage[0] && !isLocked ){
    const char *zLink;
    if( zTime==0 ){
      zLink = mprintf("wikidel?p=%s", pg);
    }else{
      zLink = mprintf("wikidel?p=%s&t=%h", pg, zTime);
    }
    common_add_menu_item( zLink, "Delete");
  }

  common_header(wiki_expand_name(pg));
  if( zTime && !doDiff ){
    @ <table align="right" cellspacing=2 cellpadding=0 border=0
    @  bgcolor="%s(BORDER1)" class="border1">
    @ <tr><td>
    @   <table width="100%%" cellspacing=1 cellpadding=5 border=0
    @    bgcolor="%s(BG1)" class="bkgnd1">
    @   <tr><th bgcolor="%s(BG1)" class="bkgnd1">Page History</th></tr>
    @   </table>
    @ </td></tr>
    @ <tr><td>
    @   <table width="100%%" cellspacing=1 cellpadding=5 border=0
    @    bgcolor="white">
    @   <tr><td>
    for(i=0; azHist[i]; i+=3){
      if( azPage[0] && strcmp(azHist[i+2],azPage[0])==0 ){
        @   <nobr><b>%s(azHist[i]) %h(azHist[i+1])</b><nobr><br>
        if( i>0 && g.okAdmin ){
          zTruncTime = azHist[i+2];
          zTruncTimeFmt = azHist[i];
          truncCnt = 1;
        }
      }else{
        @   <nobr><a href="wiki?p=%s(pg)&amp;t=%s(azHist[i+2])">
        @   %s(azHist[i]) %h(azHist[i+1])</a>&nbsp;&nbsp;&nbsp;</nobr><br>
        if( zTruncTime ) truncCnt++;
      }
    }
    @   <p><nobr><a href="wiki?p=%s(pg)">Turn Off History</a></nobr></p>
    @   </td></tr>
    @   </table>
    @ </td></tr>
    @ </table>
    /* @ <p><big><b>%s(wiki_expand_name(pg))&nbsp;&nbsp;</b></big> */
    /* @ <small><i>(as of %s(zTimeFmt))</i></small></p> */
  }else{
    /* @ <p><big><b>%s(wiki_expand_name(pg))</b></big></p> */
  }
  if( doDiff ){
    char zF1[200], zF2[200];

    zF1[0] = zF2[0] = 0;
    if( !write_to_temp(azPage[9], zF1) && !write_to_temp(azPage[4], zF2) ){
      char *zCmd;
      FILE *p;
      char zLine[1000];
      int cnt = 0;
      zCmd = mprintf("diff -c '%s' '%s'", quotable_string(zF1),
                      quotable_string(zF2));
      p = popen(zCmd, "r");
      @ <pre>
      while( fgets(zLine, sizeof(zLine), p) ){
        cnt++;
        if( cnt>3 ) cgi_printf("%h", zLine);
      }
      @ </pre>
    } else {
      common_err("Unable to create a temporary file");
    }
    if( zF1[0] ) unlink(zF1);
    if( zF2[0] ) unlink(zF2);
  }else if( azPage[0] ){
    char *zLinkSuffix;
    zLinkSuffix = zTime ? mprintf("&amp;%h",zTime) : "";
    output_wiki(azPage[4], zLinkSuffix, pg);
    isLocked = atoi(azPage[1]);
    attachment_html(pg,
      "<h3>Attachments:</h3>\n<blockquote>",
      "</blockquote>"
    );
  } else {
    @ <i>This page has not been created...</i>
    isLocked = 0;
  }
  common_footer(); 
}

/*
** WEBPAGE: /wikidiff
**
** Display the difference between two wiki pages
*/
void wiki_diff(void){
}

/*
** WEBPAGE: /wikiedit
**
** Edit a page of wiki.
*/
void wikiedit_page(void){
  const char *pg = P("p");
  const char *text = P("x");
  char **az;
  int isLocked;

  login_check_credentials();
  throttle(1);
  if( pg==0 || is_wiki_name(pg)!=strlen(pg) ){
    pg = "WikiIndex";
  }
  az = db_query(
    "SELECT invtime, locked, who, ipaddr, text "
    "FROM wiki WHERE name='%s' LIMIT 1", pg
  );
  isLocked = az[0] ? atoi(az[1]) : 0;
  if( !g.okAdmin && (!g.okWiki || isLocked) ){
    cgi_redirect(mprintf("wiki?p=%s", pg));
  }
  if( g.okAdmin && az[0] && P("lock")!=0 ){
    isLocked = !isLocked;
    db_execute("UPDATE wiki SET locked=%d WHERE name='%s'", isLocked, pg);
    if( text && strcmp(remove_blank_lines(text),remove_blank_lines(az[4]))==0 ){
      cgi_redirect(mprintf("wiki?p=%s",pg));
      return;
    }
  }
  if( P("submit")!=0 && text!=0 ){
    time_t now;
    const char *zIp = getenv("REMOTE_ADDR");
    if( zIp==0 ){ zIp = ""; }
    time(&now);
    db_execute(
      "INSERT INTO wiki(name,invtime,locked,who,ipaddr,text) "
      "VALUES('%s',%d,%d,'%q','%q','%q')",
      pg, -(int)now, isLocked, g.zUser, zIp, remove_blank_lines(text)
    );
    cgi_redirect(mprintf("wiki?p=%s", pg));
    return;
  }
  if( text==0 ) text = az[0] ? az[4] : "";
  text = remove_blank_lines(text);
  common_add_menu_item( mprintf("wiki?p=%s",pg), "Cancel");
  common_header("Edit Wiki %s", pg);
  @ <p><big><b>Edit: "%s(wiki_expand_name(pg))"</b></big></p>
  @ <form action="wikiedit" method="POST">
  @ <input type="hidden" name="p" value="%s(pg)">
  if( P("preview") ){
    @ <input type="hidden" name="x" value="%h(text)">
    @ <p>The following is what the page will look like:</p>
    @ <p><table border=2 cellpadding=5 width="100%%"><tr><td>
    output_wiki(text,"",pg);
    @ </td></tr></table></p><br>
  }else{
    @ Make changes to the document text below.  
    @ See <a href="#formatting">Formatting Hints</a>.
    @ <br><textarea cols=80 rows=30 name="x" wrap="physical">
    if( text ){
      @ %h(text)
    }
    @ </textarea><br>
  }
  if( g.okAdmin ){
    if( isLocked ){
      @ <input type="submit" name="lock" value="Unlock Page">
      @ This page is currently locked, meaning only administrators
      @ can edit it.<br>
    }else{
      @ <input type="submit" name="lock" value="Lock Page">
      @ This page is currently unlocked.  Anyone can edit it.<br>
    }
  }
  if( P("preview") ){
    @ <input type="submit" name="edit" value="Make Additional Edits"><br>
    @ <input type="submit" name="submit" value="Submit Changes As Shown">
  }else{
    @ <input type="submit" name="preview" value="Preview Your Changes">
  }
  @ </form>
  attachment_html(pg,
    "<hr><h3>Attachments:</h3>\n<blockquote>",
    "</blockquote>"
  );
  if( !P("preview") ){
    @ <a name="formatting">
    @ <hr>
    @ <h3>Formatting Hints</h3>
    append_formatting_hints();
  }
  common_footer(); 
}

/*
** WEBPAGE: /wikitoc
**
** Show a wiki table of contents.
*/
void wikitoc_page(void){
  int i;
  char **az;
  const char *zOrderBy = "1";
  const char *zDesc = "";

  login_check_credentials();
  throttle(0);
  if( !g.okRdWiki ){ login_needed(); return; }
  if( P("ctime") ){
    zOrderBy = "min(-invtime)";
  }else if( P("mtime") ){
    zOrderBy = "max(-invtime)";
  }
  if( P("desc") ){
    zDesc = " DESC";
  }
  db_add_functions();
  az = db_query(
    "SELECT name, ldate(min(-invtime)), ldate(max(-invtime)) FROM wiki "
    "GROUP BY name ORDER BY %s%s", zOrderBy, zDesc
  );
  common_standard_menu("wiki", "search?w=1");
  common_header("Wiki Table Of Contents");
  @ <table>
  @ <tr>
  @ <th bgcolor="%s(BG3)" class="bkgnd3">
  @ <a href="%s(g.zPath)">Page Name</th><th width="20"></th>
  @ <th bgcolor="%s(BG3)" class="bkgnd3">
  @ <a href="%s(g.zPath)?ctime=1&desc=1">Created</th><th width="20"></th>
  @ <th bgcolor="%s(BG3)" class="bkgnd3">
  @ <a href="%s(g.zPath)?mtime=1&desc=1">Last Modified</a></th>
  @ </tr>
  for(i=0; az[i]; i+=3){
    @ <tr>
    @ <td><a href="wiki?p=%s(az[i])">%s(az[i])</a></td><td></td>
    @ <td>%s(az[i+1])</td><td></td><td>%s(az[i+2])</td>
    @ </tr>
  }
  @ </table>
  common_footer(); 
}

/*
** WEBPAGE: /wikidel
**
** The confirmation page for deleting a page of wiki.
*/
void wikidel_page(void){
  const char *pg = P("p");
  const char *zTime = P("t");
  char *zTimeFmt;
  int nBefore, nAfter;
  int tm = 0;
  int isLocked;

  login_check_credentials();
  db_add_functions();
  if( !g.okAdmin || pg==0 || is_wiki_name(pg)!=strlen(pg) ){
    login_needed();
    return;
  }
  if( zTime==0 || (tm = atoi(zTime))==0 ){
    char *z;
    zTime = 0;
    z = db_short_query(
       "SELECT max(-invtime) FROM wiki WHERE name='%s'", pg);
    if( z==0 || (tm = atoi(z))==0 ){
      cgi_redirect("index");
    }
  }
  nBefore = atoi( db_short_query(
     "SELECT count(*) FROM wiki WHERE name='%s' AND invtime>%d", pg, -tm));
  nAfter = atoi( db_short_query(
     "SELECT count(*) FROM wiki WHERE name='%s' AND invtime<%d", pg, -tm));
  zTimeFmt = db_short_query("SELECT ldate(%d)", tm);
  isLocked = atoi( db_short_query(
     "SELECT locked FROM wiki WHERE name='%s' LIMIT 1", pg));
  common_add_menu_item(
     zTime ? mprintf("wiki?p=%s&t=%h", pg, zTime) : mprintf("wiki?p=%s", pg),
     "Cancel"     
  );
  common_header("Verify Delete");
  @ <p><big><b>Delete Wiki Page "%s(wiki_expand_name(pg))"?</b></big></p>
  @ <p>All delete actions are irreversible. Make your choice carefully!</p>
  @ <form action="wikidodel" method="GET">
  @ <input type="hidden" name="p" value="%s(pg)">
  if( P("t") ){
    @ <input type="hidden" name="t" value="%s(zTime)">
  }
  @ <input type="hidden" name="t2" value="%s(zTime)">
  @ <table border=0 cellpadding=5>
  @
  if( !isLocked ){
    @ <tr><td align="right">
    if( nBefore==0 && nAfter==0 ){
      @ <input type="submit" name="all" value="Yes">
    }else{
      @ <input type="submit" name="all" value="All">
    }
    @ </td><td>
    @ Delete this page with all its history.
    @ </td></tr>
    @
  }
  if( nBefore>0 && nAfter>0 ){
    @
    @ <tr><td align="right">
    @ <input type="submit" name="after" value="Older">
    @ </td><td>
    @ Delete %d(nBefore+1) historical version(s) from %s(zTimeFmt) and older
    @ but retain the %d(nAfter) most recent version(s) of the page.
    @ </td></tr>
  }
  if( nBefore+nAfter>0 ){
    @
    @ <tr><td align="right">
    @ <input type="submit" name="one" value="One">
    @ </td><td>
    @ Delete a single page from %s(zTimeFmt) 
    @ but retain the %d(nBefore+nAfter) other version(s) of the page.
    @ </td></tr>
  }
  @
  @ <tr><td align="right">
  @ <input type="submit" name="cancel" value="Cancel">
  @ </td><td>
  @ Do not delete anything.
  @ </td></tr>
  @ </table>  
  @ </form>
  common_footer();
}

/*
** WEBPAGE: /wikidodel
**
** Do the actual work of deleting a page.  Nothing is displayed.
** After the delete is accomplished, we redirect to a different page.
*/
void wikidodel_page(void){
  const char *pg = P("p");
  const char *t = P("t");
  const char *t2 = P("t2");
  char *zLast;

  login_check_credentials();
  if( !g.okAdmin || pg==0 || is_wiki_name(pg)!=strlen(pg) || t2==0 ){
    login_needed();
    return;
  }
  if( P("cancel") ){
    if( t==0 ){
      cgi_redirect(mprintf("wiki?p=%s",pg));
    }else{
      cgi_redirect(mprintf("wiki?p=%s&t=%s",pg,t));
    }
    return;
  }
  if( P("all") ){
    db_execute(
      "BEGIN;"
      "DELETE FROM wiki WHERE name='%s';"
      "DELETE FROM attachment WHERE tn='%s';"
      "COMMIT",
      pg, pg
    );
    cgi_redirect("wiki?p=WikiIndex");
    return;
  }
  if( P("one") ){
    db_execute("DELETE FROM wiki WHERE name='%s' AND invtime=%d", pg,-atoi(t2));
  }else if( P("after") ){
    db_execute("DELETE FROM wiki WHERE name='%s' AND invtime>=%d",pg,-atoi(t2));
  }
  zLast = db_short_query("SELECT min(-invtime) FROM wiki WHERE name='%s'",pg);
  if( zLast ){
    cgi_redirect(mprintf("wiki?p=%s&t=%s",pg,zLast));
  }else{
    cgi_redirect(mprintf("wiki?p=%s",pg));
  }
}

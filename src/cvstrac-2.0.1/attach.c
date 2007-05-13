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
** Code for handling attachments
*/
#include "config.h"
#include "attach.h"
#include <time.h>

/*
** The default maximum size of an attachment.  This value can be changed
** by the setup users.
*/
#define MX_ATTACH_SIZE  "102400"


/*
** Return the maximum size of an attachment in bytes
*/
int attachment_max(void){
  return atoi(db_config("max_attach_size",MX_ATTACH_SIZE));
}

int is_integer(const char *zString){
  if( zString==0 ) return 0;
  while( *zString ){
    if( !isdigit(*zString) ) return 0;
    zString ++;
  }
  return 1;
}

/*
** WEBPAGE: /attach_add
**
** This web-page gives the user the opportunity to add an attachment to
** an existing ticket.  The "tn" query parameter specifies the ticket
** number. A tn of zero (an invalid ticket number) means attachments to
** the setup page (i.e. stylesheets, logos, etc).
**
** This routine has been extended so that "tn" can now be the name of
** a Wiki page.  This allows attachments to be added to wiki.
*/
void attachment_add(void){
  const char *zPage;
  char *zTitle = NULL;
  char *zErr = 0;
  const char *zBack;
  int mxSize = attachment_max();
  int tn = atoi(PD("tn","0"));

  zPage = P("tn");
  if( zPage==0 ){
    common_err("Invalid or missing \"tn\" query parameter");
  }
  login_check_credentials();
  throttle(1,1);
  if( is_integer(zPage) ){
    if( tn ){
      zBack = mprintf("tktview?tn=%d", tn);
      if( !g.okWrite ){
        cgi_redirect(zBack);
      }
    }else{
      zBack = "setup_style";
      if( !g.okSetup ){
        cgi_redirect("index");
      }
    }
  }else{
    zBack = mprintf("wiki?p=%t", zPage);
    if( is_user_page(zPage)  ){
      /* only admins and the user ccan attach to the users home page */
      if( !g.okAdmin && !is_home_page(zPage) ){
        cgi_redirect(zBack);
      }
    }else if( is_wiki_name(zPage)!=strlen(zPage) ){
      common_err("Invalid wiki page name \"tn=%h\"", zPage);
    }else if( !g.okWiki ){
      cgi_redirect(zBack);
    }
  }
  common_add_help_item("CvstracAttachment");
  common_add_action_item(zBack, "Cancel");
  if( P("can") || mxSize<=0 ){
    cgi_redirect(zBack);
  }
  if( P("ok") ){
    const char *zData = P("f");
    const char *zDescription = PD("d","");
    const char *zName = P("f:filename");
    int size = atoi(PD("f:bytes","0"));
    const char *zType = PD("f:mimetype","text/plain");
    const char *z;
    time_t now = time(0);
    char **az;
    int atn;
    if( zData==0 || zName==0 || zName[0]==0 || size<=0 || zType==0 ){
      common_err("Attachment information is missing from the query content");
    }
    if( size>mxSize ){
      zErr = mprintf("Your attachment is too big.  The maximum allowed size "
               "is %dKB but your attachment was %dKB", mxSize/1024, 
               (size+1023)/1024);
    }else{
      sqlite3 *pDb;
      sqlite3_stmt *pStmt;
      const char *zTail;
      int rc;

      for(z=zName; *z; z++){
        if( (*z=='/' || *z=='\\') && z[1]!=0 ){ zName = &z[1]; }
      }

      /*
      ** In order to insert a blob, we need to drop down to raw SQLite 3
      ** calls.
      */
      pDb = db_open();
      rc = sqlite3_prepare( pDb,
          "INSERT INTO "
          "   attachment(atn,tn,size,date,user,description,mime,fname,content) "
          "VALUES(NULL,?,?,?,?,?,?,?,?);",
          -1, &pStmt, &zTail);
      if( rc!=SQLITE_OK ) {
        db_err( sqlite3_errmsg(pDb), 0,
               "/attach_add: unable to add \"%h\"", zName );
      }
      sqlite3_bind_text(pStmt, 1, zPage, -1, SQLITE_STATIC);
      sqlite3_bind_int(pStmt, 2, size);
      sqlite3_bind_int(pStmt, 3, now);
      sqlite3_bind_text(pStmt, 4, g.zUser, -1, SQLITE_STATIC);
      sqlite3_bind_text(pStmt, 5, zDescription, -1, SQLITE_STATIC);
      sqlite3_bind_text(pStmt, 6, zType, -1, SQLITE_STATIC);
      sqlite3_bind_text(pStmt, 7, zName, -1, SQLITE_STATIC);
      sqlite3_bind_blob(pStmt, 8, zData, size, SQLITE_STATIC);
      rc = sqlite3_step(pStmt);
      if( rc!=SQLITE_DONE ) {
        db_err( sqlite3_errmsg(pDb), 0,
               "/attach_add: unable to add \"%h\"", zName );
      }
      sqlite3_finalize(pStmt);

      az = db_query(
          "SELECT MAX(ROWID) FROM attachment"
          );
      atn = atoi(az[0]);
      if( tn ) ticket_notify(atoi(zPage), 0, 0, atn);
      cgi_redirect(zBack);
    }
  }
  if( is_integer(zPage) ){
    if( tn==0 ){
      /* FIXME: Not sure we need a separate page unless there's an error... */
      common_header("Attachments To Setup");
    }else{
      zTitle = db_short_query("SELECT title FROM ticket WHERE tn=%d", tn);
      if( zTitle==0 ){ common_err("No such ticket: #%d", tn); }
      common_header("Attachments To Ticket #%d", tn);
    }
  }else{
    common_header("Attachments to %h", wiki_expand_name(zPage));
  }
  if( zErr ){
    @ <p><font color="red"><b>Error:</b> %h(zErr)</font></p>
  }
  if( is_integer(zPage) && tn ){
    @ <h2>Ticket #%d(tn): %h(zTitle)</h2>
  }
  if( attachment_html(zPage, "<p>Existing attachments:</p>", "")==0 ){
    @ <p>There are currently no attachments on this document.</p>
  }
  @ <p>To add a new attachment 
  @ select the file to attach below an press the "Add Attachment" button.
  @ Attachments may not be larger than %d(mxSize/1024)KB.</p>
  @ <form method="POST" action="attach_add" enctype="multipart/form-data">
  @ File to attach: <input type="file" name="f"><br>
  @ Description:
  @ (<small>See <a href="#format_hints">formatting hints</a></small>)<br>
  @ <textarea name="d" rows="4" cols="70" wrap="virtual">
  @ </textarea><br>
  @ <input type="submit" name="ok" value="Add Attachment">
  @ <input type="submit" name="can" value="Cancel">
  @ <input type="hidden" name="tn" value="%h(zPage)">
  @ </form>
  @ <hr>
  @ <a name="format_hints">
  @ <h3>Formatting Hints:</h3>
  append_formatting_hints();
  common_footer();
}

static int output_attachment_callback(
  void *nGot_,     /* Set if we got results */
  int nArg,        /* Number of columns in this result row */
  char **azArg,    /* Text of data in all columns */
  char **azName    /* Names of the columns */
){
  int *nGot = (int *)nGot_;
  if( nArg != 5 ) return 0;
  (*nGot) ++;
  cgi_set_content_type(azArg[1]);
  cgi_modified_since(atoi(azArg[3]));
  cgi_append_header(
    mprintf("Last-Modified: %s\r\n",cgi_rfc822_datestamp(atoi(azArg[3]))));
  cgi_append_header(mprintf("Content-disposition: attachment; "
        "filename=\"%T\"\r\n", azArg[4]));
  cgi_append_content(azArg[2], atoi(azArg[0]));
  g.isConst = 1;
  return 0;
}

void attachment_output(int atn){
  /*
  ** We need to use a callback here since the content is a BLOB type object
  ** and the usual db_query() won't handle NUL characters in a returned
  ** row. The callback has the full row buffer available and will handle
  ** all the output duties. got will be set if we get a row.
  */
  int got = 0;
  db_callback_query( output_attachment_callback, &got,
                     "SELECT size, mime, content, date, fname "
                     "FROM attachment "
                     "WHERE atn=%d", atn);
  if( !got ){
    common_err("No such attachment: %d", atn);
  }
}


/*
** WEBPAGE: /attach_get
**
** Retrieve an attachment. g.zExtra looks something like "90/file.gif", which
** the atoi() call turns into just the integer "90". The filename is ignored,
** although some browsers use it as an initial name when saving to disk.
*/
void attachment_get(void){
  int atn = g.zExtra ? atoi(g.zExtra) : 0;
  char *z;
  login_check_credentials();
  throttle(1,0);
  if( atn==0 ) common_err("No attachment specified");
  z = db_short_query("SELECT tn FROM attachment WHERE atn=%d", atn);
  if( z && z[0] ){
    if( is_integer(z) ){
      if( !g.okRead ){ login_needed(); return; }
    }else{
      if( !g.okRdWiki ){ login_needed(); return; }
    }
    attachment_output(atn);
  }else{
    common_err("No attachment specified");
  }
}

/*
** Return true if it is ok to delete an attachment created by zUser
** at time addTime.  Rules:
**
**   *  The Setup user can delete any attachment no matter who added
**      it or how old it is.
**
**   *  Any registered user can delete an attachment that they
**      themselves created less than 24 hours ago.
**
**   *  Users with Delete privilege can delete an attachment added
**      by anonymous within the past 24 hours.
**
*/
int ok_to_delete_attachment(int addTime, const char *zUser){
  if( g.okSetup ){
    return 1;
  }
  if( addTime<time(0)-86400 ){
    return 0;
  }
  if( !g.isAnon && strcmp(zUser, g.zUser)==0 ){
    return 1;
  }
  if( g.okDelete && strcmp(zUser, "anonymous")==0 ){
    return 1;
  }
  return 0;
}

/*
** WEBPAGE: /attach_del
**
** Delete an attachment
*/
void attachment_delete(void){
  int atn = atoi(PD("atn","0"));
  char *zDocView;
  struct tm *pTm;
  time_t t;
  char **az;
  char zDate[200];

  login_check_credentials();
  throttle(1,1);
  az = db_query("SELECT tn, size, date, user, mime, fname, description "
                "FROM attachment WHERE atn=%d", atn);
  if( az[0]==0 ){
    if( !g.okDelete ){
      common_err("Access denied");
    }else{
      common_err("No such attachment: %d", atn);
    }
  }
  t = atoi(az[2]);
  if( is_user_page(az[0]) ) {
    /* only admin and the user can manipulate a user's home page */
    if( !g.okAdmin && !is_home_page(az[0]) ){
      common_err("Access denied");
    }
  }else if( !ok_to_delete_attachment(t,az[3]) ){
    common_err("Access denied");
  }
  if( is_integer(az[0]) ){
    if( atoi(az[0]) ){
      zDocView = mprintf("tktview?tn=%t",az[0]);
    }else{
      if( !g.okSetup ){
        common_err("Access denied");
      }
      zDocView = "setup_style";
    }
  }else{
    zDocView = mprintf("wiki?p=%t",az[0]);
  }
  if( P("can") ){
    cgi_redirect(zDocView);
    return;
  }
  if( P("ok") ){
    db_execute("DELETE FROM attachment WHERE atn=%d", atn);
    cgi_redirect(zDocView);
    return;
  }
  common_add_action_item(zDocView, "Cancel");
  common_add_help_item("CvstracAttachment");
  pTm = gmtime(&t);
  strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M:%S", pTm);
  common_header("Delete Attachment?");
  @ <p>Are you sure you want to delete this attachments?</p>
  @ <blockquote>
  @ %h(az[5]) %h(az[1]) bytes added by %h(az[3]) on %h(zDate) UTC.
  if(az[6] && az[6][0]){
    @ <br>
    output_formatted(az[6], NULL);
    @ <br>
  }
  @ </blockquote>
  @
  @ <form method="POST" action="attach_del">
  @ <input type="hidden" name="atn" value="%d(atn)">
  @ &nbsp;&nbsp;&nbsp;&nbsp;
  @ <input type="submit" name="ok" value="Yes, Delete">
  @ &nbsp;&nbsp;&nbsp;&nbsp;
  @ <input type="submit" name="can" value="No, Cancel">
  @ </form>
  common_footer();
}


/*
** This routine generates HTML that shows a list of attachments for
** the given ticket number or wiki page.  If there are no attachments,
** nothing is generated.  Return the number of attachments.
*/
int attachment_html(const char *zPage, const char *zBefore, const char *zAfter){
  char **az;
  int i = 0;
  time_t now;
  if( is_integer(zPage) ){
    if( !g.okRead ) return 0;
  }else{
    if( !g.okRdWiki ) return 0;
  }
  az = db_query("SELECT atn, size, date, user, mime, fname, description "
                "FROM attachment WHERE tn='%q' ORDER BY date", zPage);
  time(&now);
  if( az[0] ){
    @ %s(zBefore)
    @ <ul>
    for(i=0; az[i]; i+=7){
      int atn = atoi(az[i]);
      char zDate[200];
      struct tm *pTm;
      time_t t = atoi(az[i+2]);
      pTm = gmtime(&t);
      strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M:%S", pTm);
      @ <li><a href="attach_get/%d(atn)/%t(az[i+5])">%h(az[i+5])</a>
      @ %h(az[i+1]) bytes added by %h(az[i+3]) on %h(zDate) UTC.
      if(az[i+6] && az[i+6][0]){
        @ <br>
        output_formatted(az[i+6], NULL);
        @ <br>
      }
      if( ok_to_delete_attachment(t, az[i+3]) ){
        @ [<a href="attach_del?atn=%d(atn)">delete</a>]
      }
    }
    @ </ul>
    @ %s(zAfter)
  }
  db_query_free(az);
  return i/7;
}

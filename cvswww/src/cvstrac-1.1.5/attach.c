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

/*
** WEBPAGE: /attach_add
**
** This web-page gives the user the opportunity to add an attachment to
** an existing ticket.  The "tn" query parameter specifies the ticket
** number.
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

  zPage = P("tn");
  if( zPage==0 ){
    common_err("Invalid or missing \"tn\" query parameter");
  }
  login_check_credentials();
  throttle(1);
  if( isdigit(zPage[0]) ){
    char zBuf[30];
    sprintf(zBuf,"%d",atoi(zPage));
    if( strcmp(zBuf,zPage)!=0 ){
      common_err("Invalid ticket number \"tn=%h\"", zPage);
    }
    zBack = mprintf("tktview?tn=%s", zPage);
    if( !g.okWrite ){
      cgi_redirect(zBack);
    }
  }else{
    if( is_wiki_name(zPage)!=strlen(zPage) ){
      common_err("Invalid wiki page name \"tn=%h\"", zPage);
    }
    zBack = mprintf("wiki?p=%s", zPage);
    if( !g.okWiki ){
      cgi_redirect(zBack);
    }
  }
  common_add_menu_item(zBack, "Cancel");
  if( P("can") || mxSize<=0 ){
    cgi_redirect(zBack);
  }
  if( P("ok") ){
    const char *zData = P("f");
    const char *zName = P("f:filename");
    int size = atoi(PD("f:bytes","0"));
    const char *zType = PD("f:mimetype","text/plain");
    char *zBlob;
    const char *z;
    time_t now = time(0);
    if( zData==0 || zName==0 || size==0 || zType==0 ){
      common_err("Attachment information is missing from the query content");
    }
    if( size>mxSize ){
      zErr = mprintf("Your attachment is too big.  The maximum allowed size "
               "is %dKB but your attachment was %dKB", mxSize/1024, 
               (size+1023)/1024);
    }else{
      zBlob = malloc( (256*size + 1263)/253 );
      if( zBlob==0 ){
        common_err("Malloc failed");
      }
      blob_encode(zData, size, zBlob);
      for(z=zName; *z; z++){
        if( (*z=='/' || *z=='\\') && z[1]!=0 ){ zName = &z[1]; }
      }
      db_execute(
        "INSERT INTO attachment(atn,tn,size,date,user,mime,fname,content) " 
        "VALUES(NULL,'%s',%d,%d,'%q','%q','%q','%s');",
        zPage, size, now, g.zUser, zType, zName, zBlob);
      free(zBlob);
      cgi_redirect(zBack);
    }
  }
  if( isdigit(zPage[0]) ){
    zTitle = db_short_query("SELECT title FROM ticket WHERE tn=%d",atoi(zPage));
    if( zTitle==0 ){ common_err("No such ticket: #%s", zPage); }
    common_header("Attachments To Ticket #%s", zPage);
  }else{
    common_header("Attachments to %s", wiki_expand_name(zPage));
  }
  if( zErr ){
    @ <p><font color="red"><b>Error:</b> %s(zErr)</font></p>
  }
  if( isdigit(zPage[0]) ){
    @ <h2>Ticket #%s(zPage): %h(zTitle)</h2>
  }
  if( attachment_html(zPage, "<p>Existing attachments:</p>", "")==0 ){
    @ <p>There are currently no attachments on this document.</p>
  }
  @ <p>To add a new attachment 
  @ select the file to attach below an press the "Add Attachment" button.
  @ Attachments may not be larger than %d(mxSize/1024)KB.</p>
  @ <form method="POST" action="attach_add" enctype="multipart/form-data">
  @ File to attach: <input type="file" name="f"><br>
  @ <input type="submit" name="ok" value="Add Attachment">
  @ <input type="submit" name="can" value="Cancel">
  @ <input type="hidden" name="tn" value="%s(zPage)">
  @ </form>
  common_footer();
}

/*
** WEBPAGE: /attach_get
**
** Retrieve an attachment.  The "atn" query parameter is used to retrieve
** the file.  The "fn" query parameter is optional.  If present, "fn" must
** match the file name.
*/
void attachment_get(void){
  char **az;
  int i, atn;

  login_check_credentials();
  throttle(1);
  if( !g.okRead && !g.okRdWiki ){ login_needed(); return; }
  if( g.zExtra==0 || g.zExtra[0]==0 ){
    common_err("No attachment specified");
  }
  atn = atoi(g.zExtra);
  az = db_query("SELECT size, mime, fname, content FROM attachment "
                "WHERE atn=%d", atn);
  if( az[0]==0 ){
    common_err("No such attachment: %d", atn);
  }
  blob_decode(az[3], az[3]);
  cgi_set_content_type(az[1]);
  cgi_append_content(az[3], atoi(az[0]));
  g.isConst = 1;
  return;
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
  throttle(1);
  az = db_query("SELECT tn, size, date, user, mime, fname "
                "FROM attachment WHERE atn=%d", atn);
  if( az[0]==0 ){
    if( !g.okAdmin ){
      common_err("Access denied");
    }else{
      common_err("No such attachment: %d", atn);
    }
  }
  if( !g.okAdmin && strcmp(g.zUser,az[3]) ){
    common_err("Access denied");
  }
  if( isdigit(az[0][0]) ){
    zDocView = mprintf("tktview?tn=%s",az[0]);
  }else{
    zDocView = mprintf("wiki?p=%s",az[0]);
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
  common_add_menu_item(zDocView, "Cancel");
  t = atoi(az[2]);
  pTm = gmtime(&t);
  strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M:%S", pTm);
  common_header("Delete Attachment?");
  @ <p>Are you sure you want to delete this attachments?</p>
  @ <blockquote>
  @ %h(az[5]) %s(az[1]) bytes added by %h(az[3]) on %s(zDate) UTC.
  @ </blockquote>
  @
  @ <form method="GET" action="attach_del">
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
  if( !g.okRead && !g.okRdWiki ) return 0;
  az = db_query("SELECT atn, size, date, user, mime, fname "
                "FROM attachment WHERE tn='%q' ORDER BY date", zPage);
  if( az[0] ){
    @ %s(zBefore)
    @ <ul>
    for(i=0; az[i]; i+=6){
      char zDate[200];
      struct tm *pTm;
      time_t t = atoi(az[i+2]);
      pTm = gmtime(&t);
      strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M:%S", pTm);
      @ <li><a href="attach_get/%s(az[i])/%h(az[i+5])">%h(az[i+5])</a>
      @ %s(az[i+1]) bytes added by %h(az[i+3]) on %s(zDate) UTC.
      if( g.okAdmin || strcmp(g.zUser,az[i+3])==0 ){
        @ [<a href="attach_del?atn=%s(az[i])">delete</a>]
      }
    }
    @ </ul>
    @ %s(zAfter)
  }
  db_query_free(az);
  return i/6;
}

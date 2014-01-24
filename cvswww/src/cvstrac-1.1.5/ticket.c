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
** This file contains code used to generate web pages for
** processing trouble and enhancement tickets.
*/
#include "config.h"
#include "ticket.h"
#include <time.h>

/*
** If the "notify" configuration parameter exists in the CONFIG
** table and is not an empty string, then make various % substitutions
** on that string and execute the result.
*/
static void ticket_notify(int tn){
  const char *zNotify;
  char *zCmd;
  int i, j, c;
  int cmdSize;
  int cnt[128];
  const char *azSubst[128];

  static const struct { int key; char *zColumn; } aKeys[] = {
      { 'a',  "assignedto"  },
      /* A - e-mail address of assignedto person */
      { 'c',  "contact"     },
      { 'd',  "description" },
      /* n - ticket number */
      /* p - project name  */
      { 'r',  "remarks"     },
      { 's',  "status"      },
      { 't',  "title"       },
      /* u - current user  */
      { 'w',  "owner"       },
      { 'y',  "type"        },
  };
     

  zNotify = db_config("notify",0);
  if( zNotify==0 || zNotify[0]==0 ) return;
  memset(cnt, 0, sizeof(cnt));
  memset(azSubst, 0, sizeof(azSubst));
  for(i=0; zNotify[i]; i++){
    if( zNotify[i]=='%' ){
      c = zNotify[i+1] & 0x7f;
      cnt[c&0x7f]++;
    }
  }
  if( cnt['n']>0 ){
    azSubst['n'] = mprintf("%d", tn);
  }
  if( cnt['u']>0 ){
    azSubst['u'] = mprintf("%s", g.zUser);
  }
  if( cnt['p']>0 ){
    azSubst['p'] = mprintf("%s", g.zName);
  }
  if( cnt['A']>0 ){
    azSubst['A'] = 
      db_short_query("SELECT user.email FROM ticket, user "
                     "WHERE ticket.tn=%d and ticket.assignedto=user.id", tn);
  }
  for(i=0; i<sizeof(aKeys)/sizeof(aKeys[0]); i++){
    c = aKeys[i].key;
    if( cnt[c]>0 ){
      azSubst[c] =
        db_short_query("SELECT %s FROM ticket WHERE tn=%d",
                       aKeys[i].zColumn, tn);
    }
  }
  if( cnt['c']>0 && azSubst['c'][0]==0 ){
    azSubst['c'] = 
      db_short_query("SELECT user.email FROM ticket, user "
                     "WHERE ticket.tn=%d and ticket.owner=user.id", tn);
  }

  /* Sanitize the strings to be substituted by removing any single-quotes
  ** and backslashes.
  **
  ** That way, the notify command can contains strings like '%d' or '%r'
  ** (surrounded by quotes) and a hostile user cannot insert arbitrary
  ** shell commands.  Also figure out how much space is needed to hold
  ** the string after substitutes occur.
  */
  cmdSize = strlen(zNotify)+1;
  for(i=0; i<sizeof(azSubst)/sizeof(azSubst[0]); i++){
    if( azSubst[i]==0 || cnt[i]<=0 ) continue;
    azSubst[i] = quotable_string(azSubst[i]);
    cmdSize += cnt[i]*strlen(azSubst[i]);
  }

  zCmd = malloc( cmdSize + 1 );
  if( zCmd==0 ) return;
  for(i=j=0; zNotify[i]; i++){
    if( zNotify[i]=='%' && (c = zNotify[i+1]&0x7f)!=0 && azSubst[c]!=0 ){
      int k;
      const char *z = azSubst[c];
      for(k=0; z[k]; k++){ zCmd[j++] = z[k]; }
      i++;
    }else{
      zCmd[j++] = zNotify[i];
    }
  }
  zCmd[j] = 0;
  assert( j<=cmdSize );
  system(zCmd);
  free(zCmd);
}

/*
** WEBPAGE: /tktnew
**
** A web-page for entering a new ticket.
*/
void ticket_new(void){
  const char *zTitle = trim_string(PD("t",""));
  const char *zType = P("y");
  const char *zVers = PD("v","");
  const char *zDesc = remove_blank_lines(PD("d",""));
  const char *zContact = PD("c","");
  const char *zWho = P("w");
  const char *zSubsys = PD("s","");
  const char *zSev = PD("r",db_config("dflt_severity","1"));
  const char *zPri = PD("p",db_config("dflt_priority","1"));
  int isPreview = P("preview")!=0;
  int severity, priority;
  char **az;
  char *zErrMsg = 0;
  int i;

  login_check_credentials();
  if( !g.okNewTkt ){
    login_needed();
    return;
  }
  throttle(1);
  severity = atoi(zSev);
  priority = atoi(zPri);
  if( zType==0 ){
    zType = db_config("dflt_tkt_type","code");
  }
  if( zWho==0 ){
    zWho = db_config("assignto","");
  }
  if( zTitle && strlen(zTitle)>70 ){
    zErrMsg = "Please make the title no more than 70 characters long.";
  }
  if( zErrMsg==0 && zTitle[0] && zType[0] && zDesc[0] && P("submit")
      && (zContact[0] || !g.isAnon) ){
    /* Check magic spam-avoidance word if they aren't logged in */
    if (strcmp(g.zUser, "anonymous") == 0) {
        if (!P("mw") || strcmp(P("mw"), "tangible")!=0) {
            print_spam_trap_failure();
            return;
        }
    }

    int tn;
    time_t now;
    const char *zState;

    db_execute("BEGIN");
    az = db_query("SELECT max(tn)+1 FROM ticket");
    tn = atoi(az[0]);
    if( tn<=0 ) tn = 1;
    time(&now);
    zState = db_config("initial_state", "new");
    db_execute(
       "INSERT INTO ticket(tn, type, status, origtime,  changetime, "
       "                   version, assignedto, severity, priority, "
       "                   subsystem, owner, title, description, contact) "
       "VALUES(%d,'%q','%q',%d,%d,'%q','%q',%d,%d,'%q','%q','%q','%q','%q')",
       tn, zType, zState, now, now, zVers, zWho, severity, priority, zSubsys,
       g.zUser, zTitle, zDesc, zContact
    );
    for(i=1; i<=5; i++){
      const char *zVal;
      char zX[3];
      sprintf(zX,"x%d",i);
      zVal = P(zX);
      if( zVal && zVal[0] ){
        db_execute("UPDATE ticket SET extra%d='%q' WHERE tn=%d", i, zVal, tn);
      }
    }
    db_execute("COMMIT");
    ticket_notify(tn);
    cgi_redirect(mprintf("tktview?tn=%d",tn));
    return;
  }else if( P("submit") ){
    if( zTitle[0]==0 ){
      zErrMsg = "Please enter a title.";
    }else if( zDesc[0]==0 ){
      zErrMsg = "Please enter a description.";
    }else if( zContact[0]==0 && g.isAnon ){
      zErrMsg = "Please enter your contact information.";
    }
  }
  
  common_standard_menu("tktnew", 0);
  common_header("Create A New Ticket");
  if( zErrMsg ){
    @ <blockquote>
    @ <font color="red">%s(zErrMsg)</font>
    @ </blockquote>
  }
  @ <form action="%s(g.zPath)" method="POST">
  @ <table cellpadding="5">
  @
  @ <tr>
  @ <td colspan=2>
  @ Enter a one-line summary of the problem:<br>
  @ <input type="text" name="t" size=70 value="%h(zTitle)">
  @ </td>
  @ </tr>
  @
  @ <tr>
  @ <td align="right">Type:
  cgi_v_optionmenu2(0, "y", zType, (const char**)db_query(
      "SELECT name, value FROM enums WHERE type='type'"));
  @ </td>
  @ <td>What type of ticket is this?</td>
  @ </tr> 
  @
  /* We don't use version */
  @ <input type="hidden" name="v" value="%h(zVers)">
  /* 
  @ <tr>
  @   <td align="right"><nobr>
  @     Version: <input type="text" name="v" value="%h(zVers)" size="10">
  @   </nobr></td>
  @   <td>
  @      Enter the version and/or build number of the product
  @      that exhibits the problem.
  @   </td>
  @ </tr>
  @
  */
  /* We don't use severity */
  @ <input type="hidden" name="r" value="1">
  /*
  @ <tr>
  @   <td align="right"><nobr>
  @     Severity:
  cgi_optionmenu(0, "r", zSev,
         "1", "1", "2", "2", "3", "3", "4", "4", "5", "5", 0);
  @   </nobr></td>
  @   <td>
  @     How debilitating is the problem?  "1" is a show-stopper defect with
  @     no workaround.  "2" is a major defect with a workaround.  "3"
  @     is a mid-level defect.  "4" is an annoyance.  "5" is a cosmetic
  @     defect or a nice-to-have feature request.
  @   </td>
  @ </tr>
  */
  @
  @ <tr>
  @   <td align="right"><nobr>
  @     Priority:
  cgi_optionmenu(0, "p", zPri,
         "1", "1", "2", "2", "3", "3", "4", "4", "5", "5", 0);
  @   </nobr></td>
  @   <td>
  @     How quickly do you need this ticket to be resolved?
  @     "1" means immediately.
  @     "2" means before the next build.  
  @     "3" means before the next release.
  @     "4" means implement as time permits.
  @     "5" means defer indefinitely.
  @   </td>
  @ </tr>
  @
  if( g.okWrite ){
    @ <tr>
    @   <td align="right"><nobr>
    @     Assigned To:
    az = db_query("SELECT id FROM user UNION SELECT '' ORDER BY id");
    cgi_v_optionmenu(0, "w", zWho, (const char **)az);
    db_query_free(az);
    @   </nobr></td>
    @   <td>
    @     To what user should this problem be assigned?
    @   </td>
    @ </tr>
    @
    az = db_query("SELECT '', '' UNION ALL "
            "SELECT name, value  FROM enums WHERE type='subsys'");
    if( az[0] && az[1] && az[2] ){
      @ <tr>
      @   <td align="right"><nobr>
      @     Subsystem:
      cgi_v_optionmenu2(4, "s", zSubsys, (const char**)az);
      db_query_free(az);
      @   </nobr></td>
      @   <td>
      @     Which component is showing a problem?
      @   </td>
      @ </tr>
    }
  }
  if( g.isAnon ){
    @ <tr>
    @   <td align="right"><nobr>
    @     Contact: <input type="text" name="c" value="%h(zContact)" size="20">
    @   </nobr></td>
    @   <td>
    @      Enter a phone number or e-mail address where a developer can
    @      contact you with questions about this ticket.  The information
    @      you enter will be available to the developers only and will not
    @      be visible to general users.
    @   </td>
    @ </tr>
    @
  }
  for(i=1; i<=5; i++){
    char **az;
    const char *zDesc;
    const char *zName;
    char zX[3];
    char zExName[100];

    sprintf(zExName,"extra%d_desc",i);
    zDesc = db_config(zExName, 0);
    if( zDesc==0 ) continue;
    sprintf(zExName,"extra%d_name",i);
    zName = db_config(zExName, 0);
    if( zName==0 ) continue;
    az = db_query("SELECT name, value FROM enums "
                   "WHERE type='extra%d'", i);
    sprintf(zX, "x%d", i);
    @ <tr>
    @   <td align="right"><nobr>
    @     %h(zName):
    if( az==0 || az[0]==0 ){
      @     <input type="text" name="%s(zX)" value="%h(PD(zX,""))" size="20">
    }else{
      cgi_v_optionmenu2(0, zX, PD(zX,az[0]), (const char**)az);
    }
    @   </nobr></td>
    @   <td>
    @      %s(zDesc)
    @   </td>
    @ </tr>
    @
  }
  @ <tr>
  @   <td colspan="2">
  if( isPreview ){
    @     Description Preview:
    @     <table border=1 cellpadding=15 width="100%%"><tr><td>
    output_formatted(zDesc, 0);
    @     </td></tr></table>
    @     <input type="hidden" name="d" value="%h(zDesc)">
  }else{
    @     Enter a detailed description of the problem.  For code defects,
    @     be sure to provide details on exactly how the problem can be
    @     reproduced.  Provide as much detail as possible. 
    @     <a href="#format_hints">Formatting hints</a>.
    @     <br>
    @ <textarea rows="10" cols="70" wrap="virtual" name="d">%h(zDesc)</textarea>
    if( g.okWrite ){
      @     <br>Note: If you want to include a large script or binary file
      @     with this ticket you will be given an opportunity to add attachments
      @     to the ticket after the ticket has been created.  Do not paste
      @     large scripts or screen dumps in the description.
    }
  }
  @   </td>
  @ </tr>
  @ 
  if(strcmp(g.zUser, "anonymous") == 0){
      @ <tr>
      @   <td colspan="2">
      @ Enter the magic word, which is 'tangible': <input type="text" name="mw" value="%h(P("mw"))" size=10>
      @   </td>
      @ </tr>
  }
  if( isPreview ){
    @ <tr>
    @   <td align="right">
    @     <input type="submit" value="Edit">
    @   </td>
    @   <td>
    @     Edit the description again.
    @   </td>
    @ </tr>
  }else{
    @ <tr>
    @   <td align="right">
    @     <input type="submit" name="preview" value="Preview">
    @   </td>
    @   <td>
    @     Preview the formatting of the description.
    @   </td>
    @ </tr>
  }
  @ 
  @ <tr>
  @   <td align="right">
  @     <input type="submit" name="submit" value="Submit">
  @   </td>
  @   <td>
  @     After filling in the information about, press this button to create
  @     the new ticket.
  @   </td>
  @ </tr>
  @ </table>
  @ </form>
  @ <a name="format_hints">
  @ <hr>
  @ <h3>Formatting Hints:</h3>
  append_formatting_hints();
  common_footer();
}

/*
** Output the complete change history for the ticket given.
*/
static void ticket_change_history(int tn){
  char **az;
  time_t lastTime = 0, thisTime, now;
  char *zLastUser = "";
  int once = 1;
  int i;
  char zPage[30];

  time(&now);
  sprintf(zPage,"%d",tn);
  az = db_query("SELECT user, chngtime, fieldid, oldval, newval "
                "FROM tktchng WHERE tn=%d ORDER BY chngtime, user", tn);
  for(i=0; az[i]; i+=5){
    thisTime = atoi(az[i+1]);
    if( thisTime!=lastTime || strcmp(zLastUser,az[i])!=0 ){
      struct tm *pTm;
      char zDate[200];

      if( once ){
        @ <h3>History:</h3>
        @ <ol type="1">
        once = 0;
      }else{
        @   </ol>
        @ </p></li>
      }
      pTm = localtime(&thisTime);
      strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M:%S", pTm);
      @
      @ <li><p>By %h(az[i]) on %s(zDate)
      @   <ol type="a">
      zLastUser = az[i];
      lastTime = thisTime;
    }
    if( strcmp(az[i+2],"description")==0 || strcmp(az[i+2],"remarks")==0 ){
      int len1, len2;
      len1 = strlen(az[i+3]);
      len2 = strlen(az[i+4]);
      if( len1==0 ){
        @   <li>Added %s(az[i+2]).</li>
      }else if( len2>len1+5 && strncmp(az[i+3],az[i+4],len1)==0 ){
        @   <li>Appended to %s(az[i+2]):<blockquote>
        output_formatted(&az[i+4][len1], zPage);
        @   </blockquote></li>
      }else{
        @   <li>Changed %s(az[i+2]). Used to be:<blockquote>
        output_formatted(az[i+3], zPage);
        @   </blockquote></li>
      }
    }else if( (!g.okWrite || g.isAnon) && strcmp(az[i+2],"contact")==0 ){
      /* Do not show contact information to unprivileged users */
      @   <li>Changed %h(az[i+2]).</li>
    }else if( strncmp(az[i+2],"extra",5)==0 ){
      char zLabel[30];
      const char *zAlias;
      sprintf(zLabel,"%s_name", az[i+2]);
      zAlias = db_config(zLabel, az[i+2]);
      @   <li>Changed %h(zAlias) from "%h(az[i+3])" to "%h(az[i+4])".</li>
    }else{
      @   <li>Changed %h(az[i+2]) from "%h(az[i+3])" to "%h(az[i+4])".</li>
    }
  }
  if( !once ){
    @   </ol>
    @ </p></li>
    @ </ol>
    /* Users other than anonymous can undo their own changes for 24 hours.
    ** Admin users can undo anonymous changes forever.
    */
    if( (lastTime>now-86400 && !g.isAnon && strcmp(zLastUser, g.zUser)==0) 
     || (g.okAdmin && strcmp(zLastUser, "anonymous")==0)
    ){
      @ <p>[<a href="tktundo?tn=%d(tn)&u=%t(zLastUser)&t=%d(lastTime)">Undo
      @ last change</a>]</p>
    }
  }
}

/*
** WEBPAGE: /tktundo
**
** A webpage removing a prior edit to a ticket
*/
void ticket_undo(void){
  int tn = 0;
  const char *zUser;
  time_t tm, now;
  const char *z;
  char **az;
  int i;

  login_check_credentials();
  if( !g.okWrite ){ login_needed(); return; }
  throttle(1);
  tn = atoi(PD("tn","-1"));
  zUser = PD("u","");
  tm = atoi(PD("t","0"));
  if( tn<0 || tm==0 || zUser[0]==0 ){ cgi_redirect("index"); return; }
  time(&now);
  if( (tm<now-86400 || g.isAnon || strcmp(zUser, g.zUser)!=0)
    && (!g.okAdmin || strcmp(zUser, "anonymous")!=0)
  ){
    goto undo_finished;
  }
  if( P("w")==0 ){
    common_standard_menu(0,0);
    common_header("Undo Change To Ticket?");
    @ <p>If you really want to remove the last edit to ticket #%d(tn)
    @ then click on the "OK" link below.  Otherwise, click on "Cancel".</p>
    @ <table cellpadding="30">
    @ <tr><td>
    @ <a href="tktundo?tn=%d(tn)&u=%t(zUser)&t=%d(tm)&w=1">OK</a>
    @ </td><td>
    @ <a href="tktview?tn=%d(tn)">Cancel</a>
    @ </td></tr>
    @ </table>
    common_footer();
    return;
  }

  /* Make sure the change we are requested to undo is the vary last
  ** change.
  */
  z = db_short_query("SELECT max(chngtime) FROM tktchng WHERE tn=%d", tn);
  if( z==0 || tm!=atoi(z) ){
    goto undo_finished;
  }

  /* If we get this far, it means the user has confirmed that they
  ** want to undo the last change to the ticket.
  */
  db_execute("BEGIN");
  az = db_query("SELECT fieldid, oldval FROM tktchng "
                "WHERE tn=%d AND user='%q' AND chngtime=%d",
                tn, zUser, tm);
  for(i=0; az[i]; i+=2){
    db_execute("UPDATE ticket SET %s='%q' WHERE tn=%d", az[i], az[i+1], tn);
  }
  db_execute("DELETE FROM tktchng WHERE tn=%d AND user='%q' AND chngtime=%d",
             tn, zUser, tm);
  db_execute("COMMIT");

undo_finished:
  cgi_redirect(mprintf("tktview?tn=%d",tn));
}  


/*
** Extract the ticket number and report number from the "tn" query
** parameter.
*/
#if 0 /* NOT USED */
static void extract_codes(int *pTn, int *pRn){
  *pTn = *pRn = 0;
  sscanf(PD("tn",""), "%d,%d", pTn, pRn);
}
#endif

/*
** WEBPAGE: /tktview
**
** A webpage for viewing the details of a ticket
*/
void ticket_view(void){
  int i, j;
  int tn = 0, rn = 0;
  char **az;
  char **azChng;
  static char zEditLink[50];
  static char zViewLink[50];
  static char zAttachLink[50];
  char *z;
  const char *azExtra[5];
  char zPage[30];
  const char *zTn;

  login_check_credentials();
  if( !g.okRead ){ login_needed(); return; }
  throttle(1);
  history_update(0);
  zTn = PD("tn","");
  sscanf(zTn, "%d,%d", &tn, &rn);
  if( tn<=0 ){ cgi_redirect("index"); return; }
  sprintf(zPage,"%d",tn);
  common_standard_menu("tktview", "search?t=1");
  if( rn>0 ){
    sprintf(zViewLink, "rptview?rn=%d", rn);
    common_replace_menu_item(zViewLink, "Report");
  }
  if( g.okWrite ){
    sprintf(zEditLink,"tktedit?tn=%s",zTn);
    common_add_menu_item(zEditLink, "Edit");
    if( attachment_max()>0 ){
      sprintf(zAttachLink,"attach_add?tn=%d",tn);
      common_add_menu_item(zAttachLink, "Attach");
    }
  }

  /* Check to see how many "extra" ticket fields are defined
  */
  azExtra[0] = db_config("extra1_name",0);
  azExtra[1] = db_config("extra2_name",0);
  azExtra[2] = db_config("extra3_name",0);
  azExtra[3] = db_config("extra4_name",0);
  azExtra[4] = db_config("extra5_name",0);

  /* Get the record out of the database.
  */
  db_add_functions();
  az = db_query("SELECT "
                "  type,"               /* 0 */
                "  status,"             /* 1 */
                "  ldate(origtime),"    /* 2 */
                "  ldate(changetime),"  /* 3 */
                "  derivedfrom,"        /* 4 */
                "  version,"            /* 5 */
                "  assignedto,"         /* 6 */
                "  severity,"           /* 7 */
                "  priority,"           /* 8 */
                "  subsystem,"          /* 9 */
                "  owner,"              /* 10 */
                "  title,"              /* 11 */
                "  description,"        /* 12 */
                "  remarks, "           /* 13 */
                "  contact,"            /* 14 */
                "  extra1,"             /* 15 */
                "  extra2,"             /* 16 */
                "  extra3,"             /* 17 */
                "  extra4,"             /* 18 */
                "  extra5 "             /* 19 */
                "FROM ticket WHERE tn=%d", tn);
  if( az[0]==0 ){
    cgi_redirect("index");
    return;
  }
  azChng = db_query(
    "SELECT chng.date, chng.cn, chng.branch, chng.user, chng.message "
    "FROM xref, chng WHERE xref.tn=%d AND xref.cn=chng.cn "
    "ORDER BY chng.date DESC", tn);
  common_header("Ticket #%d", tn);
  @ <h2>Ticket %d(tn): %h(az[11])</h2>
  @ <blockquote>
  output_formatted(az[12], zPage);
  @ </blockquote>
  @
  @ <table align="right" hspace="10" cellpadding=2 border=0>
  @ <tr><td bgcolor="%s(BORDER1)" class="border1">
  @ <table width="100%%" border=0 cellpadding=4 cellspacing=0>
  @ <tr bgcolor="%s(BG1)" class="bkgnd1">
  @ <td valign="top" align="left">
  if( az[13][0]==0 ){
    @ [<a href="tktappend?tn=%s(zTn)">Add remarks</a>]
  } else {
    @ [<a href="tktappend?tn=%s(zTn)">Append remarks</a>]
  }
  @ </td></tr></table></td></tr></table>
  @ <h3>Remarks:</h3>
  @ <blockquote>
  output_formatted(az[13], zPage);
  @ </blockquote>
  @
  @ <h3>Properties:</h3>
  @ 
  @ <blockquote>
  @ <table>
  @ <tr>
  @   <td align="right">Type:</td>
  @   <td bgcolor="%s(BG3)" class="bkgnd3"><b>%h(az[0])&nbsp;</b></td>
  /* We don't use version */
/*
  @ <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
  @   <td align="right">Version:</td>
  @   <td bgcolor="%s(BG3)" class="bkgnd3"><b>%h(az[5])&nbsp;</b></td>
  @ </tr>
*/
  @ <tr>
  @   <td align="right">Status:</td>
  @   <td bgcolor="%s(BG3)" class="bkgnd3"><b>%h(az[1])</b></td>
  @ <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
  @   <td align="right">Created:</td>
  @   <td bgcolor="%s(BG3)" class="bkgnd3"><b>%h(az[2])</b></td>
  @ </tr>
  /* We don't use severity */
/*
  @ <tr>
  @   <td align="right">Severity:</td>
  @   <td bgcolor="%s(BG3)" class="bkgnd3"><b>%h(az[7])&nbsp;</b></td>
  @ <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
  @   <td align="right">Last&nbsp;Change:</td>
  @   <td bgcolor="%s(BG3)" class="bkgnd3"><b>%h(az[3])</b></td>
  @ </tr>
*/
  @ <tr>
  @   <td align="right">Priority:</td>
  @   <td bgcolor="%s(BG3)" class="bkgnd3"><b>%h(az[8])&nbsp;</b></td>
  @ <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
  @   <td align="right">Subsystem:</td>
  @   <td bgcolor="%s(BG3)" class="bkgnd3"><b>%h(az[9])&nbsp;</b></td>
  @ </tr>
  @ <tr>
  @   <td align="right">Assigned&nbsp;To:</td>
  @   <td bgcolor="%s(BG3)" class="bkgnd3"><b>%h(az[6])&nbsp;</b></td>
  @ <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
  @   <td align="right">Derived From:</td>
  @   <td bgcolor="%s(BG3)" class="bkgnd3"><b>%h(az[4])&nbsp;</b></td>
  @ </tr>
  @ <tr>
  @   <td align="right">Creator:</td>
  @   <td bgcolor="%s(BG3)" class="bkgnd3"><b>%h(az[10])&nbsp;</b></td>
  if( g.okWrite && !g.isAnon ){
    @ <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
    @   <td align="right">Contact:</td>
    if( strchr(az[14],'@') ){
      @   <td bgcolor="%s(BG3)" class="bkgnd3"><b><a href="mailto:%h(az[14])">
      @        %h(az[14])</a>&nbsp;</b></td>
    }else{
      @   <td bgcolor="%s(BG3)" class="bkgnd3"><b>%h(az[14])&nbsp;</b></td>
    }
    @ </tr>
    j = 0;
  } else {
    j = 1;
  }
  for(i=0; i<5; i++){
    if( azExtra[i]==0 ) continue;
    if( j==0 ){
      @ <tr>
    }else{
      @ <td></td>
    }
    @   <td align="right">%h(azExtra[i]):</td>
    @   <td bgcolor="%s(BG3)" class="bkgnd3"><b>%s(az[15+i])&nbsp;</b></td>
    if( j==0 ){
      j = 1;
    }else{
      @ </tr>
      j = 0;
    }
  }
  if( j==1 ){
    @ </tr>
  }
  @ <tr>
  @ </tr>
  @ </table>
  @ </blockquote>
  if( azChng[0] ){
    int i;
    @ <h3>Related Check-ins:</h3>
    @ <table cellspacing=0 border=0 cellpadding=0>
    for(i=0; azChng[i]; i+=5){
      time_t thisDate;
      struct tm *pTm;
      char zDate[100];
      char zPrefix[200];
      char zSuffix[100];
      thisDate = atoi(azChng[i]);
      pTm = localtime(&thisDate);
      strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M", pTm);
      if( azChng[i+2][0] ){
        sprintf(zPrefix, "Check-in [%.20s] on branch %.50s: ",
           azChng[i+1], azChng[i+2]);
      }else{
        sprintf(zPrefix, "Check-in [%.20s]: ", azChng[i+1]);
      }
      sprintf(zSuffix, " (By %.30s)", azChng[i+3]);
      @ <tr><td valign="top" width=160 align="right">%s(zDate)</td>
      @ <td valign="top" width=30 align="center">
      @ <img width=9 height=9 align="bottom" src="dot.gif" alt="*"></td>
      @ <td valign="top" align="left"> 
      output_formatted(zPrefix, 0);
      z = azChng[i+4];
      if( output_trim_message(z, MN_CKIN_MSG, MX_CKIN_MSG) ){
        output_formatted(z, 0);
        @ &nbsp;[...]
      }else{
        output_formatted(z, 0);
      }
      output_formatted(zSuffix, 0);
      @ </td></tr>
    }
    @ </table>
  }
  attachment_html(zPage,"<h3>Attachments:</h3>\n<blockquote>","</blockquote>");
  ticket_change_history(tn);
  common_footer();
}

void print_spam_trap_failure(void){
  @ <p>Please go back and enter the magic word <strong>tangible</strong> with your
  @ changes.  
  @ <p>We ask you to do this because there are people who run software which
  @ crawls the internet and automatically posts adverts.  Asking you to enter the
  @ word is a reliable and unobtrusive way of making sure you are a human, not a
  @ piece of software. 
  @ <p>If you are using mySociety cvstrac regularly, please
  @ <a href="mailto:team@mysociety.org">contact us</a> and we will 
  @ give you a proper account.
}

/*
** WEBPAGE: /tktedit
**
** A webpage for making changes to a ticket
*/
void ticket_edit(void){
  static struct {
    char *zColumn;     /* Name of column in the database */
    char *zName;       /* Name of corresponding query parameter */
    int preserveSpace; /* Preserve initial spaces in text */
    const char *zOld;  /* Current value of this field */
    const char *zNew;  /* Value of the query parameter */
  } aParm[] = {
    { "type",         "y", 0, },  /* 0 */
    { "status",       "s", 0, },  /* 1 */
    { "derivedfrom",  "d", 0, },  /* 2 */
    { "version",      "v", 0, },  /* 3 */
    { "assignedto",   "a", 0, },  /* 4 */
    { "severity",     "e", 0, },  /* 5 */
    { "priority",     "p", 0, },  /* 6 */
    { "subsystem",    "m", 0, },  /* 7 */
    { "owner",        "w", 0, },  /* 8 */
    { "title",        "t", 0, },  /* 9 */
    { "description",  "c", 1, },  /* 10 */
    { "remarks",      "r", 1, },  /* 11 */
    { "contact",      "n", 0, },  /* 12 */
    { "extra1",      "x1", 0, },  /* 13 */
    { "extra2",      "x2", 0, },  /* 14 */
    { "extra3",      "x3", 0, },  /* 15 */
    { "extra4",      "x4", 0, },  /* 16 */
    { "extra5",      "x5", 0, },  /* 17 */
  };
  int tn = 0;
  int rn = 0;
  int nField;
  int i, j;
  int cnt;
  int isPreview;
  const char *zChngList;
  char *zSep;
  char **az;
  const char **azUsers;
  int *aChng;
  int nChng;
  int nExtra;
  const char *azExtra[5];
  char zPage[30];
  char zSQL[2000];

  login_check_credentials();
  if( !g.okWrite ){ login_needed(); return; }
  throttle(1);
  isPreview = P("pre")!=0;
  sscanf(PD("tn",""), "%d,%d", &tn, &rn);
  if( tn<=0 ){ cgi_redirect("index"); return; }
  sprintf(zPage, "%d", tn);
  history_update(0);

  if( g.okSetup && P("del1") ){
    common_add_menu_item(mprintf("tktedit?tn=%s",P("tn")), "Cancel");
    common_header("Are You Sure?");
    @ <form action="tktedit" method="POST">
    @ <p>Your are about to delete all traces of ticket #%d(tn) from
    @ the database.  This is an irreversible operation.  All records
    @ related to this ticket will be removed and cannot be recovered.</p>
    @
    @ <input type="hidden" name="tn" value="%s(PD("tn",""))">
    @ <input type="submit" name="del2" value="Delete The Ticket">
    @ <input type="submit" name="can" value="Cancel">
    @ </form>
    common_footer();
    return;
  }
  if( g.okSetup && P("del2") ){
    db_execute(
       "BEGIN;"
       "DELETE FROM ticket WHERE tn=%d;"
       "DELETE FROM tktchng WHERE tn=%d;"
       "DELETE FROM xref WHERE tn=%d;"
       "DELETE FROM attachment WHERE tn=%d;"
       "COMMIT;", tn, tn, tn);
    if( rn>0 ){
      cgi_redirect(mprintf("rptview?rn=%d",rn));
    }else{
      cgi_redirect("index");
    }
    return;
  }

  /* Check to see how many "extra" ticket fields are defined
  */
  nField = sizeof(aParm)/sizeof(aParm[0]);
  azExtra[0] = db_config("extra1_name",0);
  azExtra[1] = db_config("extra2_name",0);
  azExtra[2] = db_config("extra3_name",0);
  azExtra[3] = db_config("extra4_name",0);
  azExtra[4] = db_config("extra5_name",0);
  for(i=nExtra=0; i<5; i++){
    if( azExtra[i]!=0 ){
      nExtra++;
    }else{
      aParm[13+i].zColumn = 0;
    }
  }

  /* Construct a SELECT statement to extract all information we
  ** need from the ticket table.
  */
  strcpy(zSQL, "SELECT");
  j = strlen(zSQL);
  zSep = " ";
  for(i=0; i<nField; i++){
    sprintf(&zSQL[j], "%s%s", zSep, aParm[i].zColumn ? aParm[i].zColumn : "''");
    j += strlen(&zSQL[j]);
    zSep = ",";
  }
  sprintf(&zSQL[j], " FROM ticket WHERE tn=%d", tn);

  /* Execute the SQL.  Load all existing values into aParm[].zOld.
  */
  az = db_query(zSQL);
  if( az==0 || az[0]==0 ){
    cgi_redirect("index");
    return;
  }
  for(i=0; i<nField; i++){
    if( aParm[i].zColumn==0 ) continue;
    aParm[i].zOld = remove_blank_lines(az[i]);
  }
  
  /* Find out which fields may need to change due to query parameters.
  ** record the new values in aParm[].zNew.
  */
  for(i=cnt=0; i<nField; i++){
    if( aParm[i].zColumn==0 ){ cnt++; continue; }
    aParm[i].zNew = P(aParm[i].zName);
    if( aParm[i].zNew==0 ){
      aParm[i].zNew = aParm[i].zOld;
      if( g.isAnon && aParm[i].zName[0]=='n' ) cnt++;
    }else if( aParm[i].preserveSpace ){
      aParm[i].zNew = remove_blank_lines(aParm[i].zNew);
      cnt++;
    }else{
      aParm[i].zNew = trim_string(aParm[i].zNew);
      cnt++;
    }
  }

  /* The "cl" query parameter holds a list of integer check-in numbers that
  ** this ticket is associated with.  Convert the string into a list of
  ** nChng integers in aChng[].  Or if there is no "cl" query parameter,
  ** extract the list from the database.
  */
  zChngList = P("cl");
  if( zChngList ){
    for(i=nChng=0; zChngList[i]; i++){
      if( isdigit(zChngList[i]) ){
        nChng++;
        while( isdigit(zChngList[i+1]) ) i++;
      }
    }
    aChng = malloc( sizeof(int)*nChng );
    if( aChng==0 ) nChng = 0;
    for(i=j=0; j<nChng && zChngList[i]; i++){
      if( isdigit(zChngList[i]) ){
        aChng[j++] = atoi(&zChngList[i]);
        while( isdigit(zChngList[i+1]) ) i++;
      }
    }
  }else{
    az = db_query("SELECT cn FROM xref WHERE tn=%d ORDER BY cn", tn);
    for(nChng=0; az[nChng]; nChng++){}
    aChng = malloc( sizeof(int)*nChng );
    if( aChng==0 ) nChng = 0;
    for(i=0; i<nChng; i++){
      aChng[i] = atoi(az[i]);
    }
  }

  /* Update the record in the TICKET table.  Also update the XREF table.
  */
  if( cnt==nField && P("submit")!=0 ){

    /* Check magic spam-avoidance word if they aren't logged in */
    if (strcmp(g.zUser, "anonymous") == 0) {
        if (!P("mw") || strcmp(P("mw"), "tangible")!=0) {
            print_spam_trap_failure();
            return;
        }
    }

    time_t now;
    time(&now);
    db_execute("BEGIN");
    for(i=cnt=0; i<nField; i++){
      if( aParm[i].zColumn==0 ) continue;
      if( strcmp(aParm[i].zOld,aParm[i].zNew)==0 ) continue;
      db_execute("UPDATE ticket SET %s='%q' WHERE tn=%d",
         aParm[i].zColumn, aParm[i].zNew, tn);
      db_execute("INSERT INTO tktchng(tn,user,chngtime,fieldid,oldval,newval) "
          "VALUES(%d,'%q',%d,'%s','%q','%q')",
          tn, g.zUser, now, aParm[i].zColumn, aParm[i].zOld, aParm[i].zNew);
      cnt++;
    }
    if( cnt ){
      db_execute("UPDATE ticket SET changetime=%d WHERE tn=%d", now, tn);
    }
    db_execute("DELETE FROM xref WHERE tn=%d", tn);
    for(i=0; i<nChng; i++){
      db_execute("INSERT INTO xref(tn,cn) VALUES(%d,%d)", tn, aChng[i]);
    }
    db_execute("COMMIT");
    ticket_notify(tn);
    if( rn>0 ){
      cgi_redirect(mprintf("rptview?rn=%d",rn));
    }else{
      cgi_redirect(mprintf("tktview?tn=%d,%d",tn,rn));
    }
    return;
  }

  /* Print the header.
  */
  common_add_menu_item( mprintf("tktview?tn=%d,%d", tn, rn), "Cancel");
  if( g.okSetup ){
    common_add_menu_item( mprintf("tktview?tn=%d,%d&del1=1", tn, rn), "Delete");
  }
  common_header("Edit Ticket #%d", tn);

  @ <form action="tktedit" method="POST">
  @ 
  @ <input type="hidden" name="tn" value="%d(tn),%d(rn)">
  @ <nobr>Ticket Number: %d(tn)</nobr><br>
  @
  @ <nobr>
  @ Title: <input type="text" name="t" value="%h(aParm[9].zNew)" size=70>
  @ </nobr><br>
  @ 
  @ Description:
  @ (<small>See <a href="#format_hints">formatting hints</a></small>)<br>
  if( isPreview ){
    @ <table border=1 cellpadding=15 width="100%%"><tr><td>
    output_formatted(aParm[10].zNew, zPage);
    @ &nbsp;</td></tr></table><br>
    @ <input type="hidden" name="c" value="%h(aParm[10].zNew)">
  }else{
    @ <textarea name="c" rows="8" cols="70" wrap="virtual">
    @ %h(aParm[10].zNew)
    @ </textarea><br>
  }
  @
  @ Remarks:
  @ (<small>See <a href="#format_hints">formatting hints</a></small>)<br>
  if( isPreview ){
    @ <table border=1 cellpadding=15 width="100%%"><tr><td>
    output_formatted(aParm[11].zNew, zPage);
    @ &nbsp;</td></tr></table><br>
    @ <input type="hidden" name="r" value="%h(aParm[11].zNew)">
  }else{
    @ <textarea name="r" rows="8" cols="70" wrap="virtual">
    @ %h(aParm[11].zNew)
    @ </textarea><br>
  }
  @ 
  @ <nobr>
  @ Status:
  cgi_v_optionmenu2(0, "s", aParm[1].zNew, (const char**)db_query(
     "SELECT name, value FROM enums WHERE type='status'"));
  @ </nobr>
  @ &nbsp;&nbsp;&nbsp;
  @ 
  @ <nobr>
  @ Type: 
  cgi_v_optionmenu2(0, "y", aParm[0].zNew, (const char**)db_query(
     "SELECT name, value FROM enums WHERE type='type'"));
  @ </nobr>
  @ &nbsp;&nbsp;&nbsp;
  @ 
  @ 
  /* We don't use severity */
  @ <input type="hidden" name="e" value="%h(aParm[5].zNew)">
  /*
  @ <nobr>
  @ Severity: 
  cgi_optionmenu(0, "e", aParm[5].zNew,
         "1", "1", "2", "2", "3", "3", "4", "4", "5", "5", 0);
  @ </nobr>
  @ &nbsp;&nbsp;&nbsp;
  */
  @ 
  @ <nobr>
  @ Assigned To: 
  azUsers = (const char**)db_query(
              "SELECT id FROM user UNION SELECT '' ORDER BY id");
  cgi_v_optionmenu(0, "a", aParm[4].zNew, azUsers);
  @ </nobr>
  @ &nbsp;&nbsp;&nbsp;
  @ 
  @ <nobr>
  @ Subsystem:
  cgi_v_optionmenu2(0, "m", aParm[7].zNew, (const char**)db_query(
      "SELECT '','' UNION ALL "
      "SELECT name, value FROM enums WHERE type='subsys'"));
  @ </nobr>
  @ &nbsp;&nbsp;&nbsp;
  @ 
  /* we don't use version */
  @ <input type="hidden" name="v" value="%h(aParm[3].zNew)">
  /*
  @ <nobr>
  @ Version: <input type="text" name="v" value="%h(aParm[3].zNew)" size=10>
  @ </nobr>
  @ &nbsp;&nbsp;&nbsp;
  */
  @ 
  @ <nobr>
  @ Derived From: <input type="text" name="d" value="%h(aParm[2].zNew)" size=10>
  @ </nobr>
  @ &nbsp;&nbsp;&nbsp;
  @ 
  @ <nobr>
  @ Priority:
  cgi_optionmenu(0, "p", aParm[6].zNew,
         "1", "1", "2", "2", "3", "3", "4", "4", "5", "5", 0);
  @ </nobr>
  @ &nbsp;&nbsp;&nbsp;
  @ 
  @ <nobr>
  @ Owner: 
  cgi_v_optionmenu(0, "w", aParm[8].zNew, azUsers);
  @ </nobr>
  @ &nbsp;&nbsp;&nbsp;
  @
  if( !g.isAnon ){
    @ <nobr>
    @ Contact: <input type="text" name="n" value="%h(aParm[12].zNew)" size=20>
    @ </nobr>
    @ &nbsp;&nbsp;&nbsp;
    @
  }
  for(i=0; i<5; i++){
    char **az;
    char zX[3];

    if( azExtra[i]==0 ) continue;
    az = db_query("SELECT name, value FROM enums "
                   "WHERE type='extra%d'", i+1);
    sprintf(zX, "x%d", i+1);
    @ <nobr>
    @ %h(azExtra[i]):
    if( az && az[0] ){
      cgi_v_optionmenu2(0, zX, aParm[13+i].zNew, (const char **)az);
    }else{
      @ <input type="text" name="%s(zX)" value="%h(aParm[13+i].zNew)" size=20>
    }
    db_query_free(az);
    @ </nobr>
    @ &nbsp;&nbsp;&nbsp;
    @
  }
  @ <nobr>
  @ Associated Check-ins:
  cgi_printf("<input type=\"text\" name=\"cl\" size=70 value=\"");
  zSep = "";
  for(i=0; i<nChng; i++){
    cgi_printf("%s%d", zSep, aChng[i]);
    zSep = " ";
  }
  cgi_printf("\">\n");
  @ </nobr>
  @ &nbsp;&nbsp;&nbsp;
  @ 
  if(strcmp(g.zUser, "anonymous") == 0){
      @ <nobr>
      @ Enter the magic word, which is 'tangible': <input type="text" name="mw" value="%h(P("mw"))" size=10>
      @ </nobr>
      @ &nbsp;&nbsp;&nbsp;
      @ 
  }
  @ <p align="center">
  @ <input type="submit" name="submit" value="Apply Changes">
  @ &nbsp;&nbsp;&nbsp;
  if( isPreview ){
    @ <input type="submit" value="Edit Description And Remarks">
  }else{
    @ <input type="submit" name="pre" value="Preview Description And Remarks">
  }
  if( g.okSetup ){
    @ &nbsp;&nbsp;&nbsp;
    @ <input type="submit" name="del1" value="Delete This Ticket">
  }
  @ </p>
  @ 
  @ </form>
  attachment_html(mprintf("%d",tn),"<h3>Attachments</h3><blockquote>",
      "</blockquote>");
  ticket_change_history(tn);
  @
  @ <a name="format_hints">
  @ <hr>
  @ <h3>Formatting Hints:</h3>
  append_formatting_hints();
  common_footer();
}

/*
** WEBPAGE: /tktappend
**
** Append remarks to a ticket
*/
void ticket_append(void){
  int tn, rn;
  char zPage[30];
  int doPreview;
  int doSubmit;
  const char *zText;
  const char *zTn;

  login_check_credentials();
  if( !g.okWrite ){ login_needed(); return; }
  throttle(1);
  tn = rn = 0;
  zTn = PD("tn","");
  sscanf(zTn, "%d,%d", &tn, &rn);
  if( tn<=0 ){ cgi_redirect("index"); return; }
  sprintf(zPage,"%d",tn);
  doPreview = P("pre")!=0;
  doSubmit = P("submit")!=0;
  zText = remove_blank_lines(PD("r",""));
  if( doSubmit ){
    if( zText[0] ){
      /* Check magic spam-avoidance word if they aren't logged in */
      if (strcmp(g.zUser, "anonymous") == 0) {
          if (!P("mw") || strcmp(P("mw"), "tangible")!=0) {
              print_spam_trap_failure();
              return;
          }
      }
      time_t now;
      struct tm *pTm;
      char zDate[200];
      const char *zOrig;
      char *zNew;
      char *zSpacer = " {linebreak}\n";
      char *zHLine = "\n\n----\n";
      zOrig = db_short_query("SELECT remarks FROM ticket WHERE tn=%d", tn);
      zOrig = remove_blank_lines(zOrig);
      time(&now); 
      pTm = localtime(&now);
      strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M:%S", pTm);
      if( isspace(zText[0]) && isspace(zText[1]) ) zSpacer = "\n\n";
      if( zOrig[0]==0 ) zHLine = "";
      zNew = mprintf("%s_%s by %s:_%s%s",
                     zHLine, zDate, g.zUser, zSpacer, zText);
      db_execute(
        "BEGIN;"
        "UPDATE ticket SET remarks='%q%q' WHERE tn=%d;"
        "INSERT INTO tktchng(tn,user,chngtime,fieldid,oldval,newval) "
           "VALUES(%d,'%q',%d,'remarks','%q','%q%q');"
        "COMMIT;",
        zOrig, zNew, tn,
        tn, g.zUser, now, zOrig, zOrig, zNew
      );
    }
    cgi_redirect(mprintf("tktview?tn=%s",zTn));
  }

  common_add_menu_item( mprintf("tktview?tn=%s", zTn), "Cancel");
  common_header("Append Remarks To Ticket #%d", tn);

  @ <form action="tktappend" method="POST">
  @ <input type="hidden" name="tn" value="%s(zTn)">
  @ 
  @ Append to Remarks:
  @ (<small>See <a href="#format_hints">formatting hints</a></small>)<br>
  if( doPreview ){
    @ <table border=1 cellpadding=15 width="100%%"><tr><td>
    output_formatted(zText, zPage);
    @ &nbsp;</td></tr></table><br>
    @ <input type="hidden" name="r" value="%h(zText)">
  }else{
    @ <textarea name="r" rows="8" cols="70" wrap="virtual">
    @ %h(zText)
    @ </textarea><br>
  }
  if(strcmp(g.zUser, "anonymous") == 0){
      @ <nobr>
      @ Enter the magic word, which is 'tangible': <input type="text" name="mw" value="" size=10>
      @ </nobr>
      @ &nbsp;&nbsp;&nbsp;
      @ 
  }
  @ <p align="center">
  @ <input type="submit" name="submit" value="Apply">
  @ &nbsp;&nbsp;&nbsp;
  if( doPreview ){
    @ <input type="submit" value="Edit">
  }else{
    @ <input type="submit" name="pre" value="Preview">
  }
  @ </p>
  @ 
  @ </form>
  @ <a name="format_hints">
  @ <hr>
  @ <h3>Formatting Hints:</h3>
  append_formatting_hints();
  common_footer();
}

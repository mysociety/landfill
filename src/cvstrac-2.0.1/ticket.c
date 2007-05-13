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
void ticket_notify(int tn, int first_change, int last_change, int atn){
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
      /* D - description, HTML formatted */
      /* f - First TKTCHNG rowid of change set; zero if new record */
      /* h = attacHment number if change is a new attachment; zero otherwise */
      /* l - Last TKTCHNG rowid of change set; zero if new record */
      /* n - ticket number */
      /* p - project name  */
      { 'r',  "remarks"     },
      /* R - remarks, HTML formatted */
      { 's',  "status"      },
      { 't',  "title"       },
      /* u - current user  */
      { 'w',  "owner"       },
      { 'y',  "type"        },
      { '1',  "extra1"        },
      { '2',  "extra2"        },
      { '3',  "extra3"        },
      { '4',  "extra4"        },
      { '5',  "extra5"        },
  };

  if( !db_exists("SELECT tn FROM ticket WHERE tn=%d",tn) ){
    return;
  }

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
  if( cnt['f']>0 ){
    azSubst['f'] = mprintf("%d", first_change);
  }
  if( cnt['h']>0 ){
    azSubst['h'] = mprintf("%d", atn);
  }
  if( cnt['l']>0 ){
    azSubst['l'] = mprintf("%d", last_change);
  }
  if( cnt['u']>0 ){
    azSubst['u'] = mprintf("%s", g.zUser);
  }
  if( cnt['p']>0 ){
    azSubst['p'] = mprintf("%s", g.zName);
  }
  if( cnt['D']>0 ){
    /* ensure we grab a description */
    cnt['d']++;
  }
  if( cnt['R']>0 ){
    /* ensure we grab remarks */
    cnt['r']++;
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
  if( cnt['c']>0 && azSubst['c']==0 && azSubst['c'][0]==0 ){
    azSubst['c'] = 
      db_short_query("SELECT user.email FROM ticket, user "
                     "WHERE ticket.tn=%d and ticket.owner=user.id", tn);
  }
  if( cnt['D'] ){
    azSubst['D'] = format_formatted( azSubst['d'] );
    cnt['d']--;
  }
  if( cnt['R'] ){
    azSubst['R'] = format_formatted( azSubst['r'] );
    cnt['r']--;
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
** Adds all appropriate action bar links for ticket tools
*/
static void add_tkt_tools(
  const char *zExcept,
  int tn
){
  int i;
  char *zLink;
  char **azTools;
  db_add_functions();
  azTools = db_query("SELECT tool.name FROM tool,user "
                     "WHERE tool.object='tkt' AND user.id='%q' "
                     "      AND cap_and(tool.perms,user.capabilities)!=''",
                     g.zUser);

  for(i=0; azTools[i]; i++){
    if( zExcept && 0==strcmp(zExcept,azTools[i]) ) continue;

    zLink = mprintf("tkttool?t=%T&tn=%d", azTools[i], tn);
    common_add_action_item(zLink, azTools[i]);
  }
}

/*
** WEBPAGE: /tkttool
**
** Execute an external tool on a given ticket
*/
void tkttool(void){
  int tn = atoi(PD("tn","0"));
  const char *zTool = P("t");
  char *zAction;
  const char *azSubst[32];
  int n = 0;

  if( tn==0 || zTool==0 ) cgi_redirect("index");

  login_check_credentials();
  if( !g.okRead ){ login_needed(); return; }
  throttle(1,0);
  history_update(0);

  zAction = db_short_query("SELECT command FROM tool "
                           "WHERE name='%q'", zTool);
  if( zAction==0 || zAction[0]==0 ){
    cgi_redirect(mprintf("tktview?tn=%d",tn));
  }

  common_standard_menu(0, "search?t=1");
  common_add_action_item(mprintf("tktview?tn=%d", tn), "View");
  common_add_action_item(mprintf("tkthistory?tn=%d", tn), "History");
  add_tkt_tools(zTool,tn);

  common_header("#%d: %h", tn, zTool);

  azSubst[n++] = "TN";
  azSubst[n++] = mprintf("%d",tn);
  azSubst[n++] = 0;

  n = execute_tool(zTool,zAction,0,azSubst);
  free(zAction);
  if( n<=0 ){
    cgi_redirect(mprintf("tktview?tn=%d", tn));
  }
  common_footer();
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
  const char *zFrom = P("f");
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
  throttle(1,1);
  severity = atoi(zSev);
  priority = atoi(zPri);

  /* user can enter #tn or just tn, and empty is okay too */
  zFrom = extract_integer(zFrom);

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
       "                   version, assignedto, severity, priority, derivedfrom, "
       "                   subsystem, owner, title, description, contact) "
       "VALUES(%d,'%q','%q',%d,%d,'%q','%q',%d,%d,'%q','%q','%q','%q','%q','%q')",
       tn, zType, zState, now, now, zVers, zWho, severity, priority, zFrom, zSubsys,
       g.zUser, zTitle, zDesc, zContact
    );
    for(i=1; i<=5; i++){
      const char *zVal;
      char zX[3];
      bprintf(zX,sizeof(zX),"x%d",i);
      zVal = P(zX);
      if( zVal && zVal[0] ){
        db_execute("UPDATE ticket SET extra%d='%q' WHERE tn=%d", i, zVal, tn);
      }
    }
    db_execute("COMMIT");
    ticket_notify(tn, 0, 0, 0);
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
  common_add_help_item("CvstracTicket");
  common_add_action_item( "index", "Cancel");
  common_header("Create A New Ticket");
  if( zErrMsg ){
    @ <blockquote>
    @ <font color="red">%h(zErrMsg)</font>
    @ </blockquote>
  }
  @ <form action="%T(g.zPath)" method="POST">
  @ <table cellpadding="5">
  @
  @ <tr>
  @ <td colspan=2>
  @ Enter a one-line summary of the problem:<br>
  @ <input type="text" name="t" size=70 maxlength=70 value="%h(zTitle)">
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
  @ <tr>
  @   <td align="right"><nobr>
  @     Severity:
  cgi_optionmenu(0, "r", zSev,
         "1", "1", "2", "2", "3", "3", "4", "4", "5", "5", NULL);
  @   </nobr></td>
  @   <td>
  @     How debilitating is the problem?  "1" is a show-stopper defect with
  @     no workaround.  "2" is a major defect with a workaround.  "3"
  @     is a mid-level defect.  "4" is an annoyance.  "5" is a cosmetic
  @     defect or a nice-to-have feature request.
  @   </td>
  @ </tr>
  @
  @ <tr>
  @   <td align="right"><nobr>
  @     Priority:
  cgi_optionmenu(0, "p", zPri,
         "1", "1", "2", "2", "3", "3", "4", "4", "5", "5", NULL);
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
  @ <tr>
  @   <td align="right"><nobr>
  @     Derived From: <input type="text" name="f" value="%h(zFrom)" size="5">
  @   </nobr></td>
  @   <td>
  @      Is this related to an existing ticket?
  @   </td>
  @ </tr>
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

    bprintf(zExName,sizeof(zExName),"extra%d_desc",i);
    zDesc = db_config(zExName, 0);
    if( zDesc==0 ) continue;
    bprintf(zExName,sizeof(zExName),"extra%d_name",i);
    zName = db_config(zExName, 0);
    if( zName==0 ) continue;
    az = db_query("SELECT name, value FROM enums "
                   "WHERE type='extra%d'", i);
    bprintf(zX, sizeof(zX), "x%d", i);
    @ <tr>
    @   <td align="right"><nobr>
    @     %h(zName):
    if( az==0 || az[0]==0 ){
      @     <input type="text" name="%h(zX)" value="%h(PD(zX,""))" size="20">
    }else{
      cgi_v_optionmenu2(0, zX, PD(zX,az[0]), (const char**)az);
    }
    @   </nobr></td>
    @   <td>
    /* description is already HTML markup */
    @      %s(zDesc)
    @   </td>
    @ </tr>
    @
  }
  @ <tr>
  @   <td colspan="2">
  @     Enter a detailed description of the problem.  For code defects,
  @     be sure to provide details on exactly how the problem can be
  @     reproduced.  Provide as much detail as possible. 
  @     <a href="#format_hints">Formatting hints</a>.
  @     <br>
  @ <textarea rows="10" cols="70" wrap="virtual" name="d">%h(zDesc)</textarea>
  if( isPreview ){
    @     <br>Description Preview:
    @     <table border=1 cellpadding=15 width="100%%"><tr><td>
    output_formatted(zDesc, 0);
    @     </td></tr></table>
  }
  if( g.okWrite ){
    @     <br>Note: If you want to include a large script or binary file
    @     with this ticket you will be given an opportunity to add attachments
    @     to the ticket after the ticket has been created.  Do not paste
    @     large scripts or screen dumps in the description.
  }
  @   </td>
  @ </tr>
  @ <tr>
  @   <td align="right">
  @     <input type="submit" name="preview" value="Preview">
  @   </td>
  @   <td>
  @     Preview the formatting of the description.
  @   </td>
  @ </tr>
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
** Return TRUE if it is ok to undo a ticket change that occurred at
** chngTime and was made by user zUser.
**
** A ticket change can be undone by:
**
**    *  The Setup user at any time.
**
**    *  By the registered user who made the change within 24 hours of
**       the change.
**
**    *  By the Delete user within 24 hours of the change if the change
**       was made by anonymous.
*/
static int ok_to_undo_change(int chngTime, const char *zUser){
  if( g.okSetup ){
    return 1;
  }
  if( g.isAnon || chngTime<time(0)-86400 ){
    return 0;
  }
  if( strcmp(g.zUser,zUser)==0 ){
    return 1;
  }
  if( g.okDelete && strcmp(zUser,"anonymous")==0 ){
    return 1;
  }
  return 0;
}

/*
** WEBPAGE: /tktundo
**
** A webpage removing a prior edit to a ticket
*/
void ticket_undo(void){
  int tn = 0;
  const char *zUser;
  time_t tm;
  const char *z;
  char **az;
  int i;

  login_check_credentials();
  if( !g.okWrite ){ login_needed(); return; }
  throttle(1,1);
  tn = atoi(PD("tn","-1"));
  zUser = PD("u","");
  tm = atoi(PD("t","0"));
  if( tn<0 || tm==0 || zUser[0]==0 ){ cgi_redirect("index"); return; }
  if( !ok_to_undo_change(tm, zUser) ){
    goto undo_finished;
  }
  if( P("can") ){
    /* user cancelled */
    goto undo_finished;
  }
  if( P("w")==0 ){
    common_standard_menu(0,0);
    common_add_help_item("CvstracTicket");
    common_add_action_item(mprintf("tkthistory?tn=%d",tn), "Cancel");
    common_header("Undo Change To Ticket?");
    @ <p>If you really want to remove the last edit to ticket #%d(tn)
    @ then click on the "OK" link below.  Otherwise, click on "Cancel".</p>
    @ <form method="POST" action="tktundo">
    @ <input type="hidden" name="tn" value="%d(tn)">
    @ <input type="hidden" name="u" value="%t(zUser)">
    @ <input type="hidden" name="t" value="%d(tm)">
    @ <table cellpadding="30">
    @ <tr><td>
    @ <input type="submit" name="w" value="OK">
    @ </td><td>
    @ <input type="submit" name="can" value="Cancel">
    @ </td></tr>
    @ </table>
    @ </form>
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
  cgi_redirect(mprintf("tkthistory?tn=%d",tn));
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

static void output_tkt_chng(char **azChng){
  time_t thisDate;
  struct tm *pTm;
  char zDate[100];
  char zPrefix[200];
  char zSuffix[100];
  char *z;
  const char *zType = (atoi(azChng[5])==0) ? "Check-in" : "Milestone";

  thisDate = atoi(azChng[0]);
  pTm = localtime(&thisDate);
  strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M", pTm);
  if( azChng[2][0] ){
    bprintf(zPrefix, sizeof(zPrefix), "%h [%.20h] on branch %.50h: ",
            zType, azChng[1], azChng[2]);
  }else{
    bprintf(zPrefix, sizeof(zPrefix), "%h [%.20h]: ", zType, azChng[1]);
  }
  bprintf(zSuffix, sizeof(zSuffix), " (By %.30h)", azChng[3]);
  @ <tr><td valign="top" width=160 align="right">%h(zDate)</td>
  @ <td valign="top" width=30 align="center">
  common_icon("dot");
  @ </td>
  @ <td valign="top" align="left"> 
  output_formatted(zPrefix, 0);
  z = azChng[4];
  if( output_trim_message(z, MN_CKIN_MSG, MX_CKIN_MSG) ){
    output_formatted(z, 0);
    @ &nbsp;[...]
  }else{
    output_formatted(z, 0);
  }
  output_formatted(zSuffix, 0);
  @ </td></tr>
}

/*
** WEBPAGE: /tktview
**
** A webpage for viewing the details of a ticket
*/
void ticket_view(void){
  int i, j, nChng;
  int tn = 0, rn = 0;
  char **az;
  char **azChng;
  char **azDrv;
  char *z;
  const char *azExtra[5];
  char zPage[30];
  const char *zTn;

  login_check_credentials();
  if( !g.okRead ){ login_needed(); return; }
  throttle(1,0);
  history_update(0);
  zTn = PD("tn","");
  sscanf(zTn, "%d,%d", &tn, &rn);
  if( tn<=0 ){ cgi_redirect("index"); return; }
  bprintf(zPage,sizeof(zPage),"%d",tn);
  common_standard_menu("tktview", "search?t=1");
  if( rn>0 ){
    common_replace_nav_item(mprintf("rptview?rn=%d", rn), "Report");
    common_add_action_item(mprintf("tkthistory?tn=%d,%d", tn, rn), "History");
  }else{
    common_add_action_item(mprintf("tkthistory?tn=%d", tn), "History");
  }
  if( g.okWrite ){
    if( rn>0 ){
      common_add_action_item(mprintf("tktedit?tn=%d,%d",tn,rn), "Edit");
    }else{
      common_add_action_item(mprintf("tktedit?tn=%d",tn), "Edit");
    }
    if( attachment_max()>0 ){
      common_add_action_item(mprintf("attach_add?tn=%d",tn), "Attach");
    }
  }
  add_tkt_tools(0,tn);
  common_add_help_item("CvstracTicket");

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
    "SELECT chng.date, chng.cn, chng.branch, chng.user, chng.message, chng.milestone "
    "FROM xref, chng WHERE xref.tn=%d AND xref.cn=chng.cn "
    "ORDER BY chng.milestone ASC, chng.date DESC", tn);
  azDrv = db_query(
    "SELECT tn,title FROM ticket WHERE derivedfrom=%d", tn);
  common_header("Ticket #%d", tn);
  @ <h2>Ticket %d(tn): %h(az[11])</h2>
  @ <blockquote>
  output_formatted(az[12], zPage);
  @ </blockquote>
  @
  @ <table align="right" hspace="10" cellpadding=2 border=0>
  @ <tr><td bgcolor="%h(BORDER1)" class="border1">
  @ <table width="100%%" border=0 cellpadding=4 cellspacing=0>
  @ <tr bgcolor="%h(BG1)" class="bkgnd1">
  @ <td valign="top" align="left">
  if( az[13][0]==0 ){
    @ [<a href="tktappend?tn=%h(zTn)">Add remarks</a>]
  } else {
    @ [<a href="tktappend?tn=%h(zTn)">Append remarks</a>]
  }
  @ </td></tr></table></td></tr></table>
  @ <h3>Remarks:</h3>
  @ <blockquote>
  output_formatted(az[13], zPage);
  @ </blockquote>

  if( az[13][0]!=0 ){
    @ <table align="right" hspace="10" cellpadding=2 border=0>
    @ <tr><td bgcolor="%h(BORDER1)" class="border1">
    @ <table width="100%%" border=0 cellpadding=4 cellspacing=0>
    @ <tr bgcolor="%h(BG1)" class="bkgnd1">
    @ <td valign="top" align="left">
    @ [<a href="tktappend?tn=%h(zTn)">Append remarks</a>]
    @ </td></tr></table></td></tr></table>
    @
  }

  @
  @ <h3>Properties:</h3>
  @ 
  @ <blockquote>
  @ <table>
  @ <tr>
  @   <td align="right">Type:</td>
  @   <td bgcolor="%h(BG3)" class="bkgnd3"><b>%h(az[0])&nbsp;</b></td>
  @ <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
  @   <td align="right">Version:</td>
  @   <td bgcolor="%h(BG3)" class="bkgnd3"><b>%h(az[5])&nbsp;</b></td>
  @ </tr>
  @ <tr>
  @   <td align="right">Status:</td>
  @   <td bgcolor="%h(BG3)" class="bkgnd3"><b>%h(az[1])</b></td>
  @ <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
  @   <td align="right">Created:</td>
  @   <td bgcolor="%h(BG3)" class="bkgnd3"><b>%h(az[2])</b></td>
  @ </tr>
  @ <tr>
  @   <td align="right">Severity:</td>
  @   <td bgcolor="%h(BG3)" class="bkgnd3"><b>%h(az[7])&nbsp;</b></td>
  @ <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
  @   <td align="right">Last&nbsp;Change:</td>
  @   <td bgcolor="%h(BG3)" class="bkgnd3"><b>%h(az[3])</b></td>
  @ </tr>
  @ <tr>
  @   <td align="right">Priority:</td>
  @   <td bgcolor="%h(BG3)" class="bkgnd3"><b>%h(az[8])&nbsp;</b></td>
  @ <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
  @   <td align="right">Subsystem:</td>
  @   <td bgcolor="%h(BG3)" class="bkgnd3"><b>%h(az[9])&nbsp;</b></td>
  @ </tr>
  @ <tr>
  @   <td align="right">Assigned&nbsp;To:</td>
  @   <td bgcolor="%h(BG3)" class="bkgnd3"><b>%h(az[6])&nbsp;</b></td>
  @ <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
  @   <td align="right">Derived From:</td>
  @   <td bgcolor="%h(BG3)" class="bkgnd3"><b>
  z = extract_integer(az[4]);
  if( z && z[0] ){
    z = mprintf("#%s",z);
    output_formatted(z,zPage);
  }else{
    @   &nbsp;
  }
  @   </b></td>
  @ </tr>
  @ <tr>
  @   <td align="right">Creator:</td>
  @   <td bgcolor="%h(BG3)" class="bkgnd3"><b>%h(az[10])&nbsp;</b></td>
  if( g.okWrite && !g.isAnon ){
    @ <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
    @   <td align="right">Contact:</td>
    if( strchr(az[14],'@') ){
      @   <td bgcolor="%h(BG3)" class="bkgnd3"><b><a href="mailto:%h(az[14])">
      @        %h(az[14])</a>&nbsp;</b></td>
    }else{
      @   <td bgcolor="%h(BG3)" class="bkgnd3"><b>%h(az[14])&nbsp;</b></td>
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
    @   <td bgcolor="%h(BG3)" class="bkgnd3"><b>%h(az[15+i])&nbsp;</b></td>
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
  if( azDrv[0] ){
    int i;
    @ <h3>Derived Tickets:</h3>
    @ <table cellspacing=0 border=0 cellpadding=0>
    for(i=0; azDrv[i]; i+=2){
      @ <tr><td valign="top" width=160 align="right">
      z = mprintf("#%s",azDrv[i]);
      output_formatted(z,zPage);
      @ </td>
      @ <td valign="center" width=30 align="center">
      common_icon("ptr1");
      @ </td>
      @ <td valign="top" align="left">
      output_formatted(azDrv[i+1],0);
      @ </td></tr>
    }
    @ </table>
  }
  nChng = 0;
  if( azChng[0] && azChng[5] && atoi(azChng[5])==0 ){
    int i;
    @ <h3>Related Check-ins:</h3>
    @ <table cellspacing=0 border=0 cellpadding=0>
    for(i=0; azChng[i]; i+=6){
      /* Milestones are handeld in loop below */
      if( atoi(azChng[i+5]) ) break;

      nChng++;
      output_tkt_chng(&azChng[i]);
    }
    @ </table>
  }
  
  if( azChng[0] && azChng[nChng*6] ){
    int i;
    @ <h3>Related Milestones:</h3>
    @ <table cellspacing=0 border=0 cellpadding=0>
    for(i=nChng*6; azChng[i]; i+=6){
      output_tkt_chng(&azChng[i]);
    }
    @ </table>
  }
  attachment_html(zPage,"<h3>Attachments:</h3>\n<blockquote>","</blockquote>");
  common_footer();
}

/*
** Check to see if the current user is authorized to delete ticket tn.
** Return true if they are and false if not.
**
** Ticket deletion rules:
**
**     * The setup user can delete any ticket at any time.
**
**     * Users other than anonymous with Delete privilege can delete
**       a ticket that was originated by anonymous and has no change
**       by anyone other than anonymous and is less than 24 hours old.
**
**     * Anonymous users can never delete tickets even if they have
**       Delete privilege
*/
static int ok_to_delete_ticket(int tn){
  time_t cutoff = time(0)-86400;
  if( g.okSetup ){
    return 1;
  }
  if( g.isAnon || !g.okDelete ){
    return 0;
  }
  if( db_exists(
     "SELECT 1 FROM ticket"
     " WHERE tn=%d AND (owner!='anonymous' OR origtime<%d)"
     "UNION ALL "
     "SELECT 1 FROM tktchng"
     " WHERE tn=%d AND (user!='anonymous' OR chngtime<%d)",
     tn, cutoff, tn, cutoff)
  ){
    return 0;
  }
  return 1;
}

static int tok_compare(const void *zA_, const void *zB_){
  const char **zA = (const char **)zA_;
  const char **zB = (const char **)zB_;
  return strcmp(*zA,*zB);
}

/*
** Tokenize a string and return it in a newly allocated NULL-terminated
** list. The resulting list is sorted alphabetically.
*/
static char **tokenize_new_line(const char *zString){
  int nTok = 0;
  char **azToks;
  int i, j, k;

  for(i=nTok=0; zString[i]; i++){
    if( !isspace(zString[i]) ){
      nTok++;
      while( zString[i+1] && !isspace(zString[i+1]) ) i++;
    }
  }
  azToks = malloc( sizeof(char *)*(nTok+1) );
  if( azToks==0 ) return NULL;
  for(i=j=0; j<nTok && zString[i]; i++){
    if( !isspace(zString[i]) ){
      k = i+1;
      while( zString[k] && !isspace(zString[k]) ) k++;
      azToks[j++] = mprintf("%.*s",k-i,&zString[i]);
      i = k;
    }
  }
  azToks[j] = 0;

  /* sort the result list. This makes it easy to catch duplicates.
  */
  qsort(azToks, j, sizeof(char*), tok_compare);

  return azToks;
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
    int numeric;       /* Field is a numeric value */
    const char *zOld;  /* Current value of this field */
    const char *zNew;  /* Value of the query parameter */
  } aParm[] = {
    { "type",         "y", 0, 0, },  /* 0 */
    { "status",       "s", 0, 0, },  /* 1 */
    { "derivedfrom",  "d", 0, 1, },  /* 2 */
    { "version",      "v", 0, 0, },  /* 3 */
    { "assignedto",   "a", 0, 0, },  /* 4 */
    { "severity",     "e", 0, 1, },  /* 5 */
    { "priority",     "p", 0, 0, },  /* 6 */
    { "subsystem",    "m", 0, 0, },  /* 7 */
    { "owner",        "w", 0, 0, },  /* 8 */
    { "title",        "t", 0, 0, },  /* 9 */
    { "description",  "c", 1, 0, },  /* 10 */
    { "remarks",      "r", 1, 0, },  /* 11 */
    { "contact",      "n", 0, 0, },  /* 12 */
    { "extra1",      "x1", 0, 0, },  /* 13 */
    { "extra2",      "x2", 0, 0, },  /* 14 */
    { "extra3",      "x3", 0, 0, },  /* 15 */
    { "extra4",      "x4", 0, 0, },  /* 16 */
    { "extra5",      "x5", 0, 0, },  /* 17 */
  };
  int tn = 0;
  int rn = 0;
  int nField;
  int i, j;
  int cnt;
  int isPreview;
  char *zSep;
  char **az;
  const char **azUsers = 0;
  char **azChng = 0;
  char **azMs = 0;
  int nExtra;
  const char *azExtra[5];
  char zPage[30];
  char zSQL[2000];
  char *zErrMsg = 0;

  login_check_credentials();
  if( !g.okWrite ){ login_needed(); return; }
  throttle(1,1);
  isPreview = P("pre")!=0;
  sscanf(PD("tn",""), "%d,%d", &tn, &rn);
  if( tn<=0 ){ cgi_redirect("index"); return; }
  bprintf(zPage,sizeof(zPage),"%d",tn);
  history_update(0);

  if( P("del1") && ok_to_delete_ticket(tn) ){
    char *zTitle = db_short_query("SELECT title FROM ticket "
                                  "WHERE tn=%d", tn);
    if( zTitle==0 ) cgi_redirect("index");

    common_add_action_item(mprintf("tktedit?tn=%h",PD("tn","")), "Cancel");
    common_header("Are You Sure?");
    @ <form action="tktedit" method="POST">
    @ <p>You are about to delete all traces of ticket
    output_ticket(tn,0);
    @ &nbsp;<strong>%h(zTitle)</strong> from
    @ the database.  This is an irreversible operation.  All records
    @ related to this ticket will be removed and cannot be recovered.</p>
    @
    @ <input type="hidden" name="tn" value="%h(PD("tn",""))">
    @ <input type="submit" name="del2" value="Delete The Ticket">
    @ <input type="submit" name="can" value="Cancel">
    @ </form>
    common_footer();
    return;
  }
  if( P("del2") && ok_to_delete_ticket(tn) ){
    db_execute(
       "BEGIN;"
       "DELETE FROM ticket WHERE tn=%d;"
       "DELETE FROM tktchng WHERE tn=%d;"
       "DELETE FROM xref WHERE tn=%d;"
       "DELETE FROM attachment WHERE tn=%d;"
       "COMMIT;", tn, tn, tn, tn);
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
  j = 0;
  appendf(zSQL,&j,sizeof(zSQL),"SELECT");
  zSep = " ";
  for(i=0; i<nField; i++){
    appendf(zSQL,&j,sizeof(zSQL), "%s%s", zSep,
            aParm[i].zColumn ? aParm[i].zColumn : "''");
    zSep = ",";
  }
  appendf(zSQL,&j,sizeof(zSQL), " FROM ticket WHERE tn=%d", tn);

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

      /* Only remarks and description fields (i.e. Wiki fields) have
      ** preserve space set. Perfect place to run through edit
      ** heuristics. If it's not allowed, the change won't go through
      ** since the counter won't match.
      */
      zErrMsg = is_edit_allowed(aParm[i].zOld,aParm[i].zNew);
      if( 0==zErrMsg ){
        cnt++;
      }
    }else if( aParm[i].numeric ){
      aParm[i].zNew = extract_integer(aParm[i].zNew);
      cnt++;
    }else{
      aParm[i].zNew = trim_string(aParm[i].zNew);
      cnt++;
    }
  }

  if( g.okCheckout ){
    if( P("cl") && P("ml") ){
      /* The "cl" query parameter holds a list of integer check-in numbers that
      ** this ticket is associated with.  Convert the string into a list of
      ** tokens. We'll filter out non-integers later.
      */
      azChng = tokenize_new_line(P("cl"));
      azMs = tokenize_new_line(P("ml"));
    }else{
      /*
      ** Probably a new form, so get the info from the database.
      */
      azChng = db_query( "SELECT xref.cn FROM xref, chng "
                         "WHERE xref.cn=chng.cn AND "
                         "       chng.milestone=0 AND xref.tn=%d", tn);

      azMs = db_query( "SELECT xref.cn FROM xref, chng "
                       "WHERE xref.cn=chng.cn AND "
                       "       chng.milestone>0 AND xref.tn=%d", tn);
    }
  }

  /* Update the record in the TICKET table.  Also update the XREF table.
  */
  if( cnt==nField && P("submit")!=0 ){
    time_t now;
    char **az;
    int first_change;
    int last_change;
    
    time(&now);
    db_execute("BEGIN");
    az = db_query(
        "SELECT MAX(ROWID)+1 FROM tktchng"
    );
    first_change = atoi(az[0]);
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
    az = db_query(
        "SELECT MAX(ROWID) FROM tktchng"
        );
    last_change = atoi(az[0]);
    if( cnt ){
      db_execute("UPDATE ticket SET changetime=%d WHERE tn=%d", now, tn);
    }

    if( g.okCheckout && P("cl") && P("ml") ){
      db_execute("DELETE FROM xref WHERE tn=%d", tn);

      /*
      ** Insert the values into the cross reference table, but only
      ** once (xref could _really_ use a uniqueness constraint).
      */

      if( azChng!=0 ){
        for(i=0; azChng[i]; i++){
          if( is_integer(azChng[i]) && (i==0 || strcmp(azChng[i],azChng[i-1]))){
            db_execute("INSERT INTO xref(tn,cn) "
                       "SELECT %d,cn FROM chng "
                       "       WHERE cn=%d AND milestone=0", 
                       tn, atoi(azChng[i]));
          }
        }
      }
      if( azMs!=0 ){
        for(i=0; azMs[i]; i++){
          if( is_integer(azMs[i]) && (i==0 || strcmp(azMs[i],azMs[i-1]))){
            db_execute("INSERT INTO xref(tn,cn) "
                       "SELECT %d,cn FROM chng "
                       "       WHERE cn=%d AND milestone>0",
                       tn, atoi(azMs[i]));
          }
        }
      }
    }
    db_execute("COMMIT");
    if( cnt ){
      ticket_notify(tn, first_change, last_change, 0);
    }
    if( rn>0 ){
      cgi_redirect(mprintf("rptview?rn=%d",rn));
    }else{
      cgi_redirect(mprintf("tktview?tn=%d,%d",tn,rn));
    }
    return;
  }

  /* Print the header.
  */
  common_add_action_item( mprintf("tktview?tn=%d,%d", tn, rn), "Cancel");
  if( ok_to_delete_ticket(tn) ){
    common_add_action_item( mprintf("tktedit?tn=%d,%d&del1=1", tn, rn),
                            "Delete");
  }
  common_add_help_item("CvstracTicket");
  common_header("Edit Ticket #%d", tn);

  @ <form action="tktedit" method="POST">
  @ 
  @ <input type="hidden" name="tn" value="%d(tn),%d(rn)">
  @ <nobr>Ticket Number: %d(tn)</nobr><br>
  if( zErrMsg ){
    @ <blockquote>
    @ <font color="red">%h(zErrMsg)</font>
    @ </blockquote>
  }
  @ <nobr>
  @ Title: <input type="text" name="t" value="%h(aParm[9].zNew)"
  @   maxlength=70 size=70>
  @ </nobr><br>
  @ 
  @ Description:
  @ (<small>See <a href="#format_hints">formatting hints</a></small>)<br>
  @ <textarea name="c" rows="8" cols="70" wrap="virtual">
  @ %h(aParm[10].zNew)
  @ </textarea><br>
  if( isPreview ){
    @ <table border=1 cellpadding=15 width="100%%"><tr><td>
    output_formatted(aParm[10].zNew, zPage);
    @ &nbsp;</td></tr></table><br>
  }
  @
  @ Remarks:
  @ (<small>See <a href="#format_hints">formatting hints</a></small>)<br>
  @ <textarea name="r" rows="8" cols="70" wrap="virtual">
  @ %h(aParm[11].zNew)
  @ </textarea><br>
  if( isPreview ){
    @ <table border=1 cellpadding=15 width="100%%"><tr><td>
    output_formatted(aParm[11].zNew, zPage);
    @ &nbsp;</td></tr></table><br>
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
  @ <nobr>
  @ Severity: 
  cgi_optionmenu(0, "e", aParm[5].zNew,
         "1", "1", "2", "2", "3", "3", "4", "4", "5", "5", NULL);
  @ </nobr>
  @ &nbsp;&nbsp;&nbsp;
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
  @ <nobr>
  @ Version: <input type="text" name="v" value="%h(aParm[3].zNew)" size=10>
  @ </nobr>
  @ &nbsp;&nbsp;&nbsp;
  @ 
  @ <nobr>
  @ Derived From: <input type="text" name="d" value="%h(aParm[2].zNew)" size=10>
  @ </nobr>
  @ &nbsp;&nbsp;&nbsp;
  @ 
  @ <nobr>
  @ Priority:
  cgi_optionmenu(0, "p", aParm[6].zNew,
         "1", "1", "2", "2", "3", "3", "4", "4", "5", "5", NULL);
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
    bprintf(zX, sizeof(zX), "x%d", i+1);
    @ <nobr>
    @ %h(azExtra[i]):
    if( az && az[0] ){
      cgi_v_optionmenu2(0, zX, aParm[13+i].zNew, (const char **)az);
    }else{
      @ <input type="text" name="%h(zX)" value="%h(aParm[13+i].zNew)" size=20>
    }
    db_query_free(az);
    @ </nobr>
    @ &nbsp;&nbsp;&nbsp;
    @
  }
  if( g.okCheckout ){
    /*
    ** Note that we don't filter the output here. If the user typed in
    ** something bad, they should be able to see it.
    */

    @ <nobr>
    @ Associated Check-ins:
    @ <input type="text" name="cl" size=70 value="\
    if( azChng!=0 ){
      for(i=0; azChng[i]; i++){
        @ %s(i?" ":"")%h(azChng[i])\
      }
    }
    @ ">
    @ </nobr>
    @ &nbsp;&nbsp;&nbsp;
    @ <nobr>
    @ Associated Milestones:
    @ <input type="text" name="ml" size=70 value="\
    if( azMs!=0 ){
      for(i=0; azMs[i]; i++){
        @ %s(i?" ":"")%h(azMs[i])\
      }
    }
    @ ">
    @ </nobr>
    @ &nbsp;&nbsp;&nbsp;
    @ 
  }
  @ <p align="center">
  @ <input type="submit" name="submit" value="Apply Changes">
  @ &nbsp;&nbsp;&nbsp;
  @ <input type="submit" name="pre" value="Preview Description And Remarks">
  if( ok_to_delete_ticket(tn) ){
    @ &nbsp;&nbsp;&nbsp;
    @ <input type="submit" name="del1" value="Delete This Ticket">
  }
  @ </p>
  @ 
  @ </form>
  attachment_html(mprintf("%d",tn),"<h3>Attachments</h3><blockquote>",
      "</blockquote>");
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
  char *zErrMsg = 0;
  char *zTktTitle;

  login_check_credentials();
  if( !g.okWrite ){ login_needed(); return; }
  throttle(1,1);
  tn = rn = 0;
  zTn = PD("tn","");
  sscanf(zTn, "%d,%d", &tn, &rn);
  if( tn<=0 ){ cgi_redirect("index"); return; }
  bprintf(zPage,sizeof(zPage),"%d",tn);
  doPreview = P("pre")!=0;
  doSubmit = P("submit")!=0;
  zText = remove_blank_lines(PD("r",""));
  if( doSubmit ){
    zErrMsg = is_edit_allowed(0,zText);
    if( zText[0] && 0==zErrMsg ){
      time_t now;
      struct tm *pTm;
      char zDate[200];
      const char *zOrig;
      char *zNew;
      char *zSpacer = " {linebreak}\n";
      char *zHLine = "\n\n----\n";
      char **az;
      int change;
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
        "UPDATE ticket SET remarks='%q%q', changetime=%d WHERE tn=%d;"
        "INSERT INTO tktchng(tn,user,chngtime,fieldid,oldval,newval) "
           "VALUES(%d,'%q',%d,'remarks','%q','%q%q');"
        "COMMIT;",
        zOrig, zNew, now, tn,
        tn, g.zUser, now, zOrig, zOrig, zNew
      );
      az = db_query(
          "SELECT MAX(ROWID) FROM tktchng"
          );
      change = atoi(az[0]);
      ticket_notify(tn, change, change, 0);
      cgi_redirect(mprintf("tktview?tn=%h",zTn));
    }
  }
  zTktTitle = db_short_query("SELECT title FROM ticket WHERE tn=%d", tn);
  
  common_add_help_item("CvstracTicket");
  common_add_action_item( mprintf("tktview?tn=%h", zTn), "Cancel");
  common_header("Append Remarks To Ticket #%d", tn);

  if( zErrMsg ){
    @ <blockquote>
    @ <font color="red">%h(zErrMsg)</font>
    @ </blockquote>
  }

  @ <form action="tktappend" method="POST">
  @ <input type="hidden" name="tn" value="%h(zTn)">
  @ Append to #%d(tn):
  cgi_href(zTktTitle, 0, 0, 0, 0, 0, "tktview?tn=%d", tn);
  @ &nbsp;
  @ (<small>See <a href="#format_hints">formatting hints</a></small>)<br>
  @ <textarea name="r" rows="8" cols="70" wrap="virtual">%h(zText)</textarea>
  @ <br>
  @ <p align="center">
  @ <input type="submit" name="submit" value="Apply">
  @ &nbsp;&nbsp;&nbsp;
  @ <input type="submit" name="pre" value="Preview">
  @ </p>
  if( doPreview ){
    @ <table border=1 cellpadding=15 width="100%%"><tr><td>
    output_formatted(zText, zPage);
    @ &nbsp;</td></tr></table><br>
  }
  @ 
  @ </form>
  @ <a name="format_hints">
  @ <hr>
  @ <h3>Formatting Hints:</h3>
  append_formatting_hints();
  common_footer();
}

/*
** Output a ticket change record. isLast indicates it's the last
** ticket change and _might_ be subject to undo.
*/
static void ticket_change(
  time_t date,        /* date/time of the change */
  int tn,             /* ticket number */
  const char *zUser,  /* user that made the change */
  const char *zField, /* field that changed */
  const char *zOld,   /* old value */
  const char *zNew,   /* new value */
  int isLast          /* non-zero if last ticket change in the history */
){
  struct tm *pTm;
  char zDate[100];
  char zPage[30];

  bprintf(zPage,sizeof(zPage),"%d",tn);

  pTm = localtime(&date);
  strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M", pTm);

  @ <li>

  if( strcmp(zField,"description")==0 || strcmp(zField,"remarks")==0 ){
    int len1, len2;
    len1 = strlen(zOld);
    len2 = strlen(zNew);
    if( len1==0 ){
      @ Added <i>%h(zField)</i>:<blockquote>
      output_formatted(&zNew[len1], zPage);
      @ </blockquote>
    }else if( len2>len1+5 && strncmp(zOld,zNew,len1)==0 ){
      @ Appended to <i>%h(zField)</i>:<blockquote>
      output_formatted(&zNew[len1], zPage);
      @ </blockquote>
    }else{
      @ Changed <i>%h(zField)</i>.
      diff_strings(1,zOld,zNew);
    }
  }else if( (!g.okWrite || g.isAnon) && strcmp(zField,"contact")==0 ){
    /* Do not show contact information to unprivileged users */
    @ Change <i>%h(zField)</i>
  }else if( strncmp(zField,"extra",5)==0 ){
    char zLabel[30];
    const char *zAlias;
    bprintf(zLabel,sizeof(zLabel),"%h_name", zField);
    zAlias = db_config(zLabel, zField);
    @ Change <i>%h(zAlias)</i> from "%h(zOld)" to "%h(zNew)"
  }else{
    @ Change <i>%h(zField)</i> from "%h(zOld)" to "%h(zNew)"
  }

  @ by %h(zUser) on %h(zDate)

  if( isLast && ok_to_undo_change(date, zUser) ){
    @ [<a href="tktundo?tn=%d(tn)&u=%t(zUser)&t=%d(date)">Undo
    @ this change</a>]</p>
  }

  @ </li>
}

/*
** Output a checkin record.
*/
static void ticket_checkin(
  time_t date,          /* date/time of the change */
  int cn,               /* change number */
  const char *zBranch,  /* branch of the change, may be NULL */
  const char *zUser,    /* user name that made the change */
  const char *zMessage  /* log message for the change */
){
  struct tm *pTm;
  char *z;
  char zDate[100];

  @ <li> Check-in 

  output_chng(cn);
  if( zBranch && zBranch[0] ){
    @ on branch %h(zBranch):
  } else {
    cgi_printf(": "); /* want the : right up against the [cn] */
  }

  z = strdup(zMessage);
  if( output_trim_message(z, MN_CKIN_MSG, MX_CKIN_MSG) ){
    output_formatted(z, 0);
    @ &nbsp;[...]
  }else{
    output_formatted(z, 0);
  }

  pTm = localtime(&date);
  strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M", pTm);
  @ (By %h(zUser) on %h(zDate))</li>
  @ </li>
}

/*
** Output an attachment record.
*/
static void ticket_attach(
  time_t date,          /* date/time of the attachment */
  int attachn,          /* attachment number */
  size_t size,          /* size, in bytes, of the attachment */
  const char *zUser,    /* username that created it */
  const char *zDescription,    /* description of the attachment */
  const char *zFilename /* name of attachment file */
){
  char zDate[100];
  struct tm *pTm;
  pTm = localtime(&date);
  strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M:%S", pTm);
  @ <li> Attachment 
  @ <a href="attach_get/%d(attachn)/%h(zFilename)">%h(zFilename)</a>
  @ %d(size) bytes added by %h(zUser) on %h(zDate).
  if( zDescription && zDescription[0] ){
    @ <br>
    output_formatted(zDescription,NULL);
    @ <br>
  }
  if( ok_to_delete_attachment(date, zUser) ){
    @ [<a href="attach_del?atn=%d(attachn)">delete</a>]
  }
  @ </li>
}

/*
** Output an inspection note.
*/
static void ticket_inspect(
  time_t date,              /* date/time of the inspection */
  int cn,                   /* change that was inspected */
  const char *zInspector,   /* username that did the inspection */
  const char *zResult       /* string describing the result */
){
  char zDate[100];
  struct tm *pTm;
  pTm = localtime(&date);
  strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M:%S", pTm);
  @ <li> Inspection report "%h(zResult)" on 
  output_chng(cn);
  @ &nbsp;by %h(zInspector) on %h(zDate)
  @ </li>
}

/*
** Output a derived ticket creation
*/
static void ticket_derived(
  time_t date,        /* date/time derived ticket was created */
  int tn,             /* number of derived ticket */
  const char* zOwner, /* creator of derived ticket */
  const char *zTitle  /* (currently unused) title of derived ticket */
){
  char zDate[100];
  struct tm *pTm;
  pTm = localtime(&date);
  strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M:%S", pTm);
  @ <li> Derived 
  output_ticket(tn,0);
  @ &nbsp;by %h(zOwner) on %h(zDate)
  @ </li>
}

/*
** WEBPAGE: /tkthistory
**
** A webpage for viewing the history of a ticket. The history is a
** chronological mix of ticket actions, checkins, attachments, etc.
*/
void ticket_history(void){
  int tn = 0, rn = 0;
  int lasttn = 0;
  char **az;
  int i;
  char zPage[30];
  const char *zTn;
  time_t orig;
  char zDate[200];
  struct tm *pTm;

  login_check_credentials();
  if( !g.okRead ){ login_needed(); return; }
  throttle(1,0);
  history_update(0);
  zTn = PD("tn","");
  sscanf(zTn, "%d,%d", &tn, &rn);
  if( tn<=0 ){ cgi_redirect("index"); return; }

  bprintf(zPage,sizeof(zPage),"%d",tn);
  common_standard_menu("tktview", "search?t=1");

  if( rn>0 ){
    common_add_action_item(mprintf("tktview?tn=%d,%d",tn,rn), "View");
  }else{
    common_add_action_item(mprintf("tktview?tn=%d",tn), "View");
  }

  common_add_help_item("CvstracTicket");

  if( g.okWrite ){
    if( rn>0 ){
      common_add_action_item(mprintf("tktedit?tn=%d,%d",tn,rn), "Edit");
    }else{
      common_add_action_item(mprintf("tktedit?tn=%d",tn), "Edit");
    }
    if( attachment_max()>0 ){
      common_add_action_item(mprintf("attach_add?tn=%d",tn), "Attach");
    }
  }
  add_tkt_tools(0,tn);

  /* Get the record from the database.
  */
  db_add_functions();
  az = db_query("SELECT title,origtime,owner FROM ticket WHERE tn=%d", tn);
  if( az == NULL || az[0]==0 ){
    cgi_redirect("index");
    return;
  }

  orig = atoi(az[1]);
  pTm = localtime(&orig);
  strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M:%S", pTm);

  common_header("Ticket #%d History", tn);
  @ <h2>Ticket %d(tn) History: %h(az[0])</h2>
  @ <ol>
  @ <li>Created %h(zDate) by %h(az[2])</li>

  /* Grab various types of ticket activities from the db.
  ** All must be sorted by ascending time and the first field of each
  ** record should be epoch time. Second field is the record type.
  */
  az = db_query(
    /* Ticket changes
    */
    "SELECT chngtime AS 'time', 1 AS 'type', "
      "user, fieldid, oldval, newval, NULL "
    "FROM tktchng WHERE tn=%d "
    "UNION ALL "

    /* Checkins
    */
    "SELECT chng.date AS 'time', 2 AS 'type', "
       " chng.cn, chng.branch, chng.user, chng.message, chng.milestone "
    "FROM xref, chng WHERE xref.tn=%d AND xref.cn=chng.cn "
    "UNION ALL "

    /* attachments
    */
    "SELECT date AS 'time', 3 AS 'type', atn, size, user, description, fname "
    "FROM attachment WHERE tn=%d "
    "UNION ALL "

    /* inspection reports
    */
    "SELECT inspect.inspecttime AS 'time', 4 AS 'type', "
      "inspect.cn, inspect.inspector, inspect.result, NULL, NULL "
    "FROM xref, inspect "
    "WHERE xref.cn=inspect.cn AND xref.tn=%d "
    "UNION ALL "

    /* derived tickets. This is just the derived ticket creation. Could
    ** also report derived ticket changes, but we'd probably have to
    ** use some kind of tree representation.
    */
    "SELECT origtime AS 'time', 5 AS 'type', tn, owner, title, NULL, NULL "
    "FROM ticket WHERE derivedfrom=%d "

    "ORDER BY 1, 2",
    tn, tn, tn, tn, tn);

  /* find the last ticket change in the list. This is necessary to allow
  ** someone to undo the last change.
  */
  for(i=0; az[i]; i+=7){
    int type = atoi(az[i+1]);
    if( type==1 ) lasttn = i;
  }

  for(i=0; az[i]; i+=7) {
    time_t date = atoi(az[i]);
    int type = atoi(az[i+1]);
    switch( type ){
      case 1: { /* ticket change */
        ticket_change(date, tn, az[i+2],
          az[i+3], az[i+4], az[i+5], lasttn==i);
        break;
      }
      case 2: { /* checkin */
        ticket_checkin(date, atoi(az[i+2]), az[i+3], az[i+4], az[i+5]);
        break;
      }
      case 3: { /* attachment */
        ticket_attach(date, atoi(az[i+2]), atoi(az[i+3]),
          az[i+4], az[i+5], az[i+6]);
        break;
      }
      case 4: { /* inspection report */
        ticket_inspect(date, atoi(az[i+2]), az[i+3], az[i+4]);
        break;
      }
      case 5: { /* derived ticket creation */
        ticket_derived(date, atoi(az[i+2]), az[i+3], az[i+4]);
        break;
      }
      default:
        /* Can't happen */
        /* assert( type >= 1 && type <= 5 ); */
        break;
    }
  }
  @ </ol>
  common_footer();
}

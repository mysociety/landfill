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
** Implementation of the Setup page
*/
#include <assert.h>
#include "config.h"
#include "setup.h"


/*
** Output a single entry for a menu generated using an HTML table.
** If zLink is not NULL or an empty string, then it is the page that
** the menu entry will hyperlink to.  If zLink is NULL or "", then
** the menu entry has no hyperlink - it is disabled.
*/
static void menu_entry(
  const char *zTitle,
  const char *zLink,
  const char *zDesc
){
  @ <tr><td valign="top">
  if( zLink && zLink[0] ){
    @ <a href="%s(zLink)"><b>%s(zTitle)</b></a></td>
  }else{
    @ %s(zTitle)</td>
  }
  @ <td valign="top">
  @ %s(zDesc)
  @ </td></tr>
  @
}

/*
** WEBPAGE: /setup
*/
void setup_page(void){
  /* The user must be at least the administrator in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okAdmin ){
    login_needed();
    return;
  }

  common_standard_menu("setup", 0);
  common_add_help_item("CvstracAdmin");
  common_header("Setup Menu");
  @ <table cellpadding="10">
  if( g.okSetup ){
    menu_entry(mprintf("%s&nbsp;Repository",g.scm.zName), "setup_repository",
      "Identify the repository to which this server is linked.");
    if( g.scm.pxUserWrite ){
      menu_entry("User&nbsp;Database", "setup_user", 
        mprintf("Control how CVSTrac interacts with the %h user "
                "and password database", g.scm.zName));
    }
    menu_entry("Log&nbsp;File", "setup_log",
      "Turn the access log file on and off.");
    menu_entry("Attachments", "setup_attach",
      "Set the maximum allowable size for attachments.");
    menu_entry("Abuse Control", "setup_throttle",
      "Options to control bandwidth abuse and wiki spam.");
  }
  menu_entry("Ticket&nbsp;Types", "setup_enum?e=type",
    "Enumerate the different types of tickets that can be entered into "
    "the system.");
  menu_entry("Ticket&nbsp;States", "setup_enum?e=status",
    "Configure the allowed values for the \"status\" attribute of tickets.");
  menu_entry("New&nbsp;Tickets&nbsp;Defaults", "setup_newtkt",
    "Specify the default values assigned to various ticket attributes when "
    "a new ticket is created.");
  menu_entry("Subsystem&nbsp;Names", "setup_enum?e=subsys",
    "List the names of subsystems that can be used in the \"subsystem\" "
    "attribute of tickets.");
  menu_entry("User-Defined&nbsp;Fields", "setup_udef",
    "Create user-defined database columns in the TICKET table");
  if( g.okSetup ){
    menu_entry("Diff&nbsp;and&nbsp;Filter&nbsp;Programs", "setup_diff",
      "Specify commands or scripts used to compute the difference between "
      "two versions of a file and pretty print files.");
    menu_entry("External&nbsp;Tools", "setup_tools",
      "Manage tools for processing CVSTrac objects." );
    menu_entry("Change&nbsp;Notification", "setup_chng",
      "Define an external program to run whenever a ticket is created "
      "or modified.");
    menu_entry("Customize&nbsp;Style", "setup_style",
      "Headers, footers, stylesheets, other web page elements.");
    menu_entry("User Interface", "setup_interface",
      "Control the user interface functionality." );
    menu_entry("Wiki Markup", "setup_markup",
      "Manage custom Wiki markups" );
    menu_entry("Backup&nbsp;&amp;&nbsp;Restore", "setup_backup",
      "Make a backup copy of the database or restore the database from a "
      "backup copy.");
    menu_entry("Timeline&nbsp;&amp;&nbsp;RSS", "setup_timeline", 
      "Set timeline cookie lifetime and RSS \"Time To Live\".");
  }
  @ </table>
  common_footer();
}

/*
** WEBPAGE: /setup_repository
*/
void setup_repository_page(void){
  const char *zRoot, *zOldRoot;
  const char *zModule, *zOldModule;

  /* The user must be the setup user in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okSetup ){
    cgi_redirect("setup");
    return;
  }

  /*
  ** The "r" query parameter is the name of the CVS repository root
  ** directory.  Change it if it has changed.
  */
  zOldRoot = db_config("cvsroot","");
  zRoot = P("r");
  if( zRoot && strcmp(zOldRoot,zRoot)!=0 ){
    db_execute("REPLACE INTO config(name,value) VALUES('cvsroot','%q');",
      zRoot);
    zOldRoot = zRoot;
    db_config(0,0);
  }

  /*
  ** The "m" query parameter is the name of the module within the
  ** CVS repository that this CVSTrac instance is suppose to track.
  ** Change it if it has changed.
  */
  zOldModule = db_config("module","");
  zModule = P("m");
  if( zModule && strcmp(zOldModule,zModule)!=0 ){
    db_execute("REPLACE INTO config(name,value) VALUES('module','%q');",
      zModule);
    zOldModule = zModule;
    db_config(0,0);
  }

  /*
  ** The "rrh" query parameter is present if the user presses the
  ** "Reread Revision History" button.  This causes the CVSROOT/history
  ** file to be reread.  Do this with caution as it erases any edits
  ** to the history that are currently in the database.  Only the
  ** setup user can do this.
  */
  if( P("rrh") ){
    common_add_action_item("setup_repository", "Cancel");
    common_header("Confirm Reread Of Repository");
    @ <h3">WARNING!</h3>
    @ <p>
    @ If you decide to <b>Reconstruct</b> the change history database all
    @ of your check-ins will be renumbered.  This might break links between
    @ tickets and wiki pages and check-ins.  Any edits you may have made
    @ to check-in messages will be undone as well.</p>
    @
    @ <p> A safer alternative is to select <b>Rescan</b> which will attempt
    @ to preserve existing check-in numbers and check-in message changes.
    @ </p>
    @
    @ <p>In either case, you may want to make a <a href="setup_backup">
    @ backup copy</a> of the database so that you can recover if something
    @ goes wrong.</p>
    @
    @ <form action="%s(g.zPath)" method="POST">
    @ <p>
    @ <input type="submit" name="rrh2" value="Reconstruct">
    @ Reconstruct the check-in database from scratch.
    @ </p>
    @ <p>
    @ <input type="submit" name="rrh3" value="Rescan">
    @ Attempt to reuse existing check-in numbers.
    @ </p>
    @ <p>
    @ <input type="submit" name="cancel" value="Cancel">
    @ Do no do anything.
    @ </p>
    @ </form>
    common_footer();
    return;
  }
  if( P("rrh2") ){
    db_execute(
      "BEGIN;"
      "DELETE FROM chng WHERE not milestone;"
      "DELETE FROM filechng;"
      "DELETE FROM file;"
      "UPDATE config SET value=0 WHERE name='historysize';"
      "COMMIT;"
      "VACUUM;"
    );
    db_config(0,0);
    history_update(0);
  }
  if( P("rrh3") ){
    db_execute(
      "BEGIN;"
      "DELETE FROM filechng WHERE rowid NOT IN ("
         "SELECT min(rowid) FROM filechng "
         "GROUP BY filename, vers||'x'"
      ");"
      "DELETE FROM chng WHERE milestone=0 AND cn NOT IN ("
         "SELECT cn FROM filechng"
      ");"
      "UPDATE config SET value=0 WHERE name='historysize';"
      "COMMIT;"
      "VACUUM;"
    );
    db_config(0,0);
    history_update(1);
  }

  common_add_nav_item("setup", "Main Setup Menu");
  common_add_help_item("CvstracAdminRepository");
  common_header("Configure Repository");
  @ <p>
  @ <form action="%s(g.zPath)" method="POST">
  @ Enter the full pathname of the root directory of the
  @ %s(g.scm.zName) repository in the space provided below.
  if( g.scm.canFilterModules ){
    @ If you want to restrict this 
    @ server to see only a subset of the files contained in the
    @ %s(g.scm.zName) repository
    @ (for example, if you want to see only one module in a 
    @ repository that contains many unrelated modules) then
    @ enter a pathname prefix for the files you want to see in the
    @ second entry box.
  }
  @ </p>
  @ <p><table>
  @ <tr>
  @   <td align="right">%s(g.scm.zName) repository:</td>
  @   <td><input type="text" name="r" size="40" value="%h(zOldRoot)"></td>
  @ </tr>

  if( g.scm.canFilterModules ){
    @ <tr>
    @   <td align="right">Module prefix:</td>
    @   <td><input type="text" name="m" size="40" value="%h(zOldModule)"></td>
    @ </tr>
  }

  @ </table><br>
  @ <input type="submit" value="Submit">
  @ </p>
  @
  @ <p>
  @ After changing the %s(g.scm.zName) repository above, you will generally
  @ want to press the following button to cause the repository history to be
  @ reread from the new repository.  You can also use this button to
  @ resynchronize the database if a prior read
  @ failed or if you have manually changed it (always a bad idea).
  @ <p><input type="submit" name="rrh" value="Reread Repository"></p>
  @ </form>
  @ </p>
  @ <hr>
  common_footer();
}

/*
** WEBPAGE: /setup_user
*/
void setup_user_page(void){
  const char *zWPswd, *zOldWPswd;

  /* The user must be at least the setup user in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okSetup ){
    cgi_redirect("setup");
    return;
  }

  /*
  ** The "wpw" query parameter is "yes" if the CVSROOT/passwd file is
  ** writable and "no" if not.  
  ** Change it if it has changed.
  */
  zOldWPswd = db_config("write_cvs_passwd","yes");
  zWPswd = P("wpw");
  if( zWPswd && strcmp(zOldWPswd,zWPswd)!=0 ){
    db_execute(
      "REPLACE INTO config(name,value) VALUES('write_cvs_passwd','%q');",
      zWPswd
    );
    zOldWPswd = zWPswd;
    db_config(0,0);
  }

  /*
  ** Import users out of the CVSROOT/passwd file if the user pressed
  ** the Import Users button.  Only setup can do this.
  */
  if( P("import_users") && g.scm.pxUserRead ){
    g.scm.pxUserRead();
  }

  common_add_nav_item("setup", "Main Setup Menu");
  common_add_help_item("CvstracAdminUserDatabase");
  common_header("Configure User Database Linkage");
  @ <p>
  @ <form action="%s(g.zPath)" method="POST">
  if( g.scm.pxUserWrite ){
    @ CVSTrac can update the CVSROOT/passwd file with the usernames and
    @ passwords of all CVSTrac users.  Enable or disable this feature
    @ below.</p>
    @ <p>Write User Changes to CVSROOT/passwd?
    cgi_optionmenu(0, "wpw", zOldWPswd, "Yes", "yes", "No", "no", NULL);
    @ <input type="submit" value="Submit">
    @ </p>
  }
  if( g.scm.pxUserRead ){
    @ <p>
    @ Use the following button to automatically create a CVSTrac user ID
    @ for every user currently named in CVSROOT/passwd.  The new users will
    @ be given the same access permissions as user "anonymous" plus check-out
    @ permission and check-in permission if CVS allows the user to write.</p>
    @ <p><input type="submit" name="import_users" value="Import CVS Users"></p>
    @ </form>
    @ </p>
  }
  common_footer();
}

/*
** WEBPAGE: /setup_log
*/
void setup_logfile_page(void){
  const char *zLog, *zOldLog;

  /* The user must be the setup user in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okSetup ){
    cgi_redirect("setup");
    return;
  }

  /* 
  ** The "log" query parameter specifies a log file into which a record
  ** of all HTTP hits is written.  Write this value if this has changed.
  ** Only setup can make this change.
  */
  zOldLog = db_config("logfile","");
  zLog = P("log");
  if( zLog && strcmp(zOldLog,zLog)!=0 ){
    db_execute(
      "REPLACE INTO config(name,value) VALUES('logfile','%q');",
      zLog
    );
    zOldLog = zLog;
    db_config(0,0);
  }

  common_add_nav_item("setup", "Main Setup Menu");
  common_add_help_item("CvstracAdminLog");
  common_header("Configure Log File");
  @ <p>
  @ <form action="%s(g.zPath)" method="POST">
  @ Enter the name of file into which is written a log of all accesses
  @ to this server.  Leave the entry blank to disable logging:
  @ </p>
  @ <p>Log File: <input type="text" name="log" size="40" value="%h(zOldLog)">
  @ <input type="submit" value="Submit">
  @ </form>
  @ </p>
  common_footer();
}

/*
** WEBPAGE: /setup_newtkt
*/
void setup_newticket_page(void){
  char **azResult;
  const char *zState, *zOldState;
  const char *zAsgnto, *zOldAsgnto;
  const char *zType, *zOldType;
  const char *zPri, *zOldPri;
  const char *zSev, *zOldSev;

  /* The user must be at least the administrator in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okAdmin ){
    login_needed();
    return;
  }

  /*
  ** The "asgnto" query parameter specifies a userid who is assigned to
  ** all new tickets.  Record this value in the configuration table if
  ** it has changed.
  */
  zOldAsgnto = db_config("assignto","");
  zAsgnto = P("asgnto");
  if( zAsgnto && strcmp(zOldAsgnto,zAsgnto)!=0 ){
    db_execute(
      "REPLACE INTO config(name,value) VALUES('assignto','%q');", zAsgnto
    );
    zOldAsgnto = zAsgnto;
    db_config(0,0);
  }

  /*
  ** The "istate" query parameter specifies the initial state for new
  ** tickets.  Record any changes to this value.
  */
  zOldState = db_config("initial_state","");
  zState = P("istate");
  if( zState && strcmp(zOldState,zState)!=0 ){
    db_execute(
      "REPLACE INTO config(name,value) VALUES('initial_state','%q');",
      zState
    );
    zOldState = zState;
    db_config(0,0);
  }

  /*
  ** The "type" query parameter specifies the initial type for new
  ** tickets.  Record any changes to this value.
  */
  zOldType = db_config("dflt_tkt_type","code");
  zType = P("type");
  if( zType && strcmp(zOldType,zType)!=0 ){
    db_execute(
      "REPLACE INTO config(name,value) VALUES('dflt_tkt_type','%q');",
      zType
    );
    zOldType = zType;
    db_config(0,0);
  }

  /*
  ** The "pri" query parameter specifies the initial priority for new
  ** tickets.  Record any changes to this value.
  */
  zOldPri = db_config("dflt_priority","1");
  zPri = P("pri");
  if( zPri && strcmp(zOldPri,zPri)!=0 ){
    db_execute(
      "REPLACE INTO config(name,value) VALUES('dflt_priority','%q');",
      zPri
    );
    zOldPri = zPri;
    db_config(0,0);
  }

  /*
  ** The "sev" query parameter specifies the initial severity for new
  ** tickets.  Record any changes to this value.
  */
  zOldSev = db_config("dflt_severity","1");
  zSev = P("sev");
  if( zSev && strcmp(zOldSev,zSev)!=0 ){
    db_execute(
      "REPLACE INTO config(name,value) VALUES('dflt_severity','%q');",
      zSev
    );
    zOldSev = zSev;
    db_config(0,0);
  }

  common_add_nav_item("setup", "Main Setup Menu");
  common_add_help_item("CvstracAdminNewTicket");
  common_header("Configure New Ticket Defaults");
  @ <p>
  @ <form action="%s(g.zPath)" method="POST">
  @ Select a user to whom new tickets will be assigned by default:</p><p>
  @ Assigned To:
  azResult = db_query("SELECT id FROM user UNION SELECT '' ORDER BY id");
  cgi_v_optionmenu(0, "asgnto", zOldAsgnto, (const char**)azResult);
  @ </p>
  @
  @ <p>
  @ Select the initial state that new tickets are created in:</p><p>
  @ Initial State:
  cgi_v_optionmenu2(0, "istate", zOldState, (const char**)db_query(
     "SELECT name, value FROM enums WHERE type='status'"));
  @ </p>
  @
  @ <p>
  @ Select the default type for new tickets:</p><p>
  @ Default Type:
  cgi_v_optionmenu2(0, "type", zOldType, (const char**)db_query(
     "SELECT name, value FROM enums WHERE type='type'"));
  @ </p>
  @
  @ <p>
  @ Select the default priority for new tickets:</p><p>
  @ Default Priority:
  cgi_optionmenu(0, "pri", zOldPri, "1", "1", "2", "2", "3", "3", "4", "4",
      "5", "5", NULL);
  @ </p>
  @
  @ <p>
  @ Select the default severity for new tickets:</p><p>
  @ Default Severity:
  cgi_optionmenu(0, "sev", zOldSev, "1", "1", "2", "2", "3", "3", "4", "4",
      "5", "5", NULL);
  @ </p>
  @
  @ <p>
  @ <input type="submit" value="Submit">
  @ </form>
  @ </p>
  common_footer();
}

/*
** WEBPAGE: /setup_interface
*/
void setup_interface_page(void){
  int atkt, ack, tkt, ck;
  const char *zBrowseUrl;
  int nCookieLife;

  /* The user must be at least the administrator in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okAdmin ){
    login_needed();
    return;
  }

  if( P("update") ){
    db_execute(
      "REPLACE INTO config(name,value) VALUES('anon_ticket_linkinfo','%d');"
      "REPLACE INTO config(name,value) VALUES('anon_checkin_linkinfo','%d');"
      "REPLACE INTO config(name,value) VALUES('ticket_linkinfo','%d');"
      "REPLACE INTO config(name,value) VALUES('checkin_linkinfo','%d');"
      "REPLACE INTO config(name,value) VALUES('browse_url_cookie_life',%d);"
      "REPLACE INTO config(name,value) VALUES('default_browse_url','%q');",
      atoi(PD("atkt","0")),
      atoi(PD("ack","0")),
      atoi(PD("tkt","0")),
      atoi(PD("ck","0")),
      atoi(PD("cl","0")),
      PD("bu","dir")
    );
    db_config(0, 0);
  }

  atkt = atoi(db_config("anon_ticket_linkinfo","0"));
  ack = atoi(db_config("anon_checkin_linkinfo","0"));
  tkt = atoi(db_config("ticket_linkinfo","1"));
  ck = atoi(db_config("checkin_linkinfo","0"));
  zBrowseUrl = db_config("default_browse_url","dir");
  nCookieLife = atoi(db_config("browse_url_cookie_life", "90"));

  common_add_nav_item("setup", "Main Setup Menu");
  common_add_help_item("CvstracAdminInterface");
  common_header("Configure User Interface");

  @ <p>
  @ <form action="%s(g.zPath)" method="POST">
  @ Ticket and check-in/milestone link information enables link tooltips
  @ in most browsers. For example,
  @ <a href="tktview?tn=1" title="First ticket">#1</a> and
  @ <a href="chngview?cn=1" title="Check-in [1]: First check-in
  @   (By anonymous)">[1]</a>. While this provides information to the
  @ user without having to follow a link, it is additional database
  @ load for the server and can increase the size of the web
  @ pages considerably. Check-in link information is usually only useful
  @ if your users put a lot of check-in links within wikis or
  @ remarks.
  @ </p>
  @ <p>
  @ <label for="atkt"><input type="checkbox" name="atkt" id="atkt"
  @   %s(atkt?" checked":"") value="1">
  @ Turn on ticket link information for anonymous users.</label>
  @ <br>
  @ <label for="ack"><input type="checkbox" name="ack" id="ack"
  @   %s(ack?" checked":"") value="1">
  @ Turn on check-in/milestone link information for anonymous users.</label>
  @ <br>
  @ <label for="tkt"><input type="checkbox" name="tkt" id="tkt"
  @   %s(tkt?" checked":"") value="1">
  @ Turn on ticket link information for logged in users.</label>
  @ <br>
  @ <label for="ck"><input type="checkbox" name="ck" id="ck"
  @   %s(ck?" checked":"") value="1">
  @ Turn on check-in/milestone link information for logged in users.</label>
  @ </p>
  @ <p>
  @ <input type="hidden" name="update" value="1">
  @ <input type="submit" value="Set">
  @ </p>
  @ <p>When browsing the repository there are two ways to list files and
  @ directories. The <em>Short</em> view is a compact listing combining
  @ all files and directories into just four columns. The <em>Long</em> view
  @ shows the most recent repository information for each file.</p>
  @ <p><label for="bu0"><input type="radio" name="bu" id="bu0"
  @    %s(strcmp("dirview",zBrowseUrl)==0?" checked":"") value="dirview">
  @ Long view</label><br>
  @ <label for="bu1"><input type="radio" name="bu" id="bu1"
  @   %s(strcmp("dir",zBrowseUrl)==0?" checked":"") value="dir">
  @ Short</label>
  @ <p>
  @ <input type="hidden" name="update" value="1">
  @ <input type="submit" value="Set">
  @ </p>
  @ <p>
  @ Enter number of days browse mode cookie should be kept by users browser.
  @ This cookie keeps track of user's preferred browse mode across user's
  @ multiple visits.<br>
  @ This applies to all users.<br>
  @ Set it to 0 to disable browse mode cookie.
  @ </p>
  @ <p>
  @ Cookie lifetime: 
  @ <input type="text" name="cl" value="%d(nCookieLife)" size=5> days
  @ <p>
  @ <input type="hidden" name="update" value="1">
  @ <input type="submit" value="Set">
  @ </form>
  @ </p>
  common_footer();
}

/*
** Generate a string suitable for inserting into a <TEXTAREA> that
** describes all allowed values for a particular enumeration.
*/
static char *enum_to_string(const char *zEnum){
  char **az;
  char *zResult;
  int i, j, nByte;
  int len1, len2, len3;
  int mx1, mx2, mx3;
  int rowCnt;
  az = db_query("SELECT name, value, color FROM enums "
                "WHERE type='%s' ORDER BY idx", zEnum);
  rowCnt = mx1 = mx2 = mx3 = 0;
  for(i=0; az[i]; i+=3){
    len1 = strlen(az[i]);
    len2 = strlen(az[i+1]);
    len3 = strlen(az[i+2]);
    if( len1>mx1 ) mx1 = len1;
    if( len2>mx2 ) mx2 = len2;
    if( len3>mx3 ) mx3 = len3;
    rowCnt++;
  }
  if( mx2<mx1 ) mx2 = mx1;
  nByte = (mx1 + mx2 + mx3 + 11)*rowCnt + 1;
  zResult = malloc( nByte );
  if( zResult==0 ) exit(1);
  for(i=j=0; az[i]; i+=3){
    const char *z1 = az[i];
    const char *z2 = az[i+1];
    const char *z3 = az[i+2];
    if( z1[0]==0 ){ z1 = "?"; }
    if( z2[0]==0 ){ z2 = z1; }
    if( z3[0] ){
      bprintf(&zResult[j], nByte-j, "%*s    %*s   (%s)\n",
              -mx1, z1, -mx2, z2, z3);
    }else{
      bprintf(&zResult[j], nByte-j, "%*s    %s\n", -mx1, z1, z2);
    }
    j += strlen(&zResult[j]);
  }
  db_query_free(az);
  zResult[j] = 0;
  return zResult;
}

/*
** Given text that describes an enumeration, fill the ENUMS table with
** coresponding entries.
**
** The text line oriented.  Each line represents a single value for
** the enum.  The first token on the line is the internal name.
** subsequent tokens are the human-readable description.  If the last
** token is in parentheses, then it is a color for the entry.
*/
static void string_to_enum(const char *zEnum, const char *z){
  int i, j, n;
  int cnt = 1;
  char *zColor;
  char zName[50];
  char zDesc[200];

  db_execute("DELETE FROM enums WHERE type='%s'", zEnum);
  while( isspace(*z) ){ z++; }
  while( *z ){
    assert( !isspace(*z) );
    for(i=1; z[i] && !isspace(z[i]); i++){}
    n = i>49 ? 49 : i;
    memcpy(zName, z, n);
    zName[n] = 0;
    z += i;
    while( *z!='\n' && isspace(z[1]) ){ z++; }
    if( *z=='\n' || *z==0 ){
      strcpy(zDesc, zName);
      zColor = "";
    }else{
      int lastP1 = -1;
      int lastP2 = -1;
      z++;
      for(j=i=0; *z && *z!='\n'; z++){
        if( j<199 && (j==0 || !isspace(*z) || !isspace(zDesc[j-1])) ){
          zDesc[j++] = *z;
        }
        if( *z=='(' ){ lastP1 = j-1; }
        else if( *z==')' ){ lastP2 = j-1; }
        else if( !isspace(*z) ){ lastP2 = -1; }
      }
      zDesc[j] = 0;
      if( lastP2>lastP1 && lastP1>1 ){
        zColor = &zDesc[lastP1+1];
        zDesc[lastP2] = 0;
        zDesc[lastP1] = 0;
        j = lastP1;
        while( j>0 && isspace(zDesc[j-1]) ){ j--; }
        zDesc[j] = 0;
      }else{
        j = strlen(zDesc);
        while( j>0 && isspace(zDesc[j-1]) ){ j--; }
        zDesc[j] = 0;
        zColor = "";
      }
    }
    db_execute(
       "INSERT INTO enums(type,idx,name,value,color) "
       "VALUES('%s',%d,'%q','%q','%q')",
       zEnum, cnt++, zName, zDesc, zColor
    );
    while( isspace(*z) ) z++;
  }

  /* If the enums were updated such that one of the defaults was removed,
  ** choose a new default.
  */
  if( !strcmp(zEnum,"status") ){
    const char* zDefault = db_config("initial_state","new");
    char* z = db_short_query("SELECT name FROM enums "
                             "WHERE type='status' AND name='%q'", zDefault);
    if( z==0 || z[0]==0 ) {
      /* gone missing, update */
      db_execute(
        "REPLACE INTO config(name,value) "
        "VALUES('initial_state',(SELECT name FROM enums WHERE type='status'));"
      );
    }
  }else if( !strcmp(zEnum,"type") ){
    const char* zDefault = db_config("dflt_tkt_type","code");
    char* z = db_short_query("SELECT name FROM enums "
                             "WHERE type='type' AND name='%q'", zDefault);
    if( z==0 || z[0]==0 ) {
      /* gone missing, update */
      db_execute(
        "REPLACE INTO config(name,value) "
        "VALUES('dflt_tkt_type',(SELECT name FROM enums WHERE type='type'));"
      );
    }
  }
}

/*
** WEBPAGE: /setup_enum
*/
void setup_enum_page(void){
  char *zText;
  const char *zEnum;
  int nRow;
  const char *zTitle;
  const char *zName;

  /* The user must be at least the administrator in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okAdmin ){
    login_needed();
    return;
  }

  /* What type of enumeration are we entering.
  */
  zEnum = P("e");
  if( zEnum==0 ){ zEnum = "subsys"; }
  if( strcmp(zEnum,"subsys")==0 ){
    zTitle = "Configure Subsystem Names";
    zName = "subsystem";
    nRow = 20;
    common_add_help_item("CvstracAdminSubsystem");
  }else
  if( strcmp(zEnum,"type")==0 ){
    zTitle = "Configure Ticket Types";
    zName = "type";
    nRow = 6;
    common_add_help_item("CvstracAdminTicketType");
  }else
  if( strcmp(zEnum,"status")==0 ){
    zTitle = "Configure Ticket States";
    zName = "status";
    nRow = 10;
    common_add_help_item("CvstracAdminTicketState");
  }else
  {
    common_add_nav_item("setup", "Main Setup Menu");
    common_header("Unknown Enumeration");
    @ <p>URL error:  The "e" query parameter specifies an unknown
    @ enumeration type: "%h(zEnum)".</p>
    @
    @ <p>Press the "Back" link above to return to the setup menu.</p>
    common_footer();
    return;
  }

  /*
  ** The "s" query parameter is a long text string that specifies
  ** the names of all subsystems.  If any subsystem names have been
  ** added or removed, then make appropriate changes to the subsyst
  ** table in the database.
  */
  if( P("x") ){
    db_execute("BEGIN");
    string_to_enum(zEnum, P("x"));
    db_execute("COMMIT");
  }

  /* Genenerate the page.
  */
  common_add_nav_item("setup", "Main Setup Menu");
  common_header(zTitle);
  zText = enum_to_string(zEnum);
  @ <p>
  @ The allowed values of the "%s(zName)" attribute of tickets
  @ are listed below.
  @ You may edit this text and press apply to change the set of
  @ allowed values.
  @ </p>
  @
  @ <p>
  @ The token on the left is the value as it is stored in the database.
  @ The text that follows is a human-readable description for the meaning
  @ of the token.  A color name for use in reports may optionally appear
  @ in parentheses after the description.
  @ </p>
  @
  @ <p>
  @ <form action="%s(g.zPath)" method="POST">
  @ <input type="hidden" name="e" value="%s(zEnum)">
  @ <textarea cols=60 rows=%d(nRow) name="x">%h(zText)</textarea>
  @ <p><input type="submit" value="Submit"></p>
  @ </form>
  common_footer();
}

/*
** WEBPAGE: /setup_udef
*/
void setup_udef_page(void){
  int idx, i;
  const char *zName;
  const char *zText;

  /* The user must be at least the administrator in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okAdmin ){
    login_needed();
    return;
  }

  /* Write back results if requested.
  */
  idx = atoi(PD("idx","0"));
  zName = P("n");
  zText = P("x");
  if( idx>=1 && idx<=5 && zName && zText ){
    char zEnum[20];
    char *zName2 = trim_string(zName);
    char *zDesc2 = trim_string(PD("d",""));
    bprintf(zEnum,sizeof(zEnum),"extra%d", idx);
    db_execute("BEGIN");
    if( zName2[0] ){
      string_to_enum(zEnum, zText);
      db_execute(
        "REPLACE INTO config(name,value) VALUES('%s_name','%q');",
        zEnum, zName2
      );
      db_execute(
        "REPLACE INTO config(name,value) VALUES('%s_desc','%q');",
        zEnum, zDesc2
      );
    }else{
      db_execute("DELETE FROM config WHERE name='%s_name'", zEnum);
      db_execute("DELETE FROM config WHERE name='%s_desc'", zEnum);
      db_execute("DELETE FROM enums WHERE type='%s'", zEnum);
    }
    db_execute("COMMIT");
    db_config(0,0);
  }

  /* Genenerate the page.
  */
  common_add_nav_item("setup", "Main Setup Menu");
  common_add_help_item("CvstracAdminUserField");
  common_header("Configure User-Defined Fields");
  @ <p>
  @ Five extra columns named "extra1" through "extra5" exist in the 
  @ TICKET table of the database.  These columns can be defined for
  @ application specific use using this configuration page.
  @ </p>
  @
  @ <p>
  @ Each column is controlled by a separate form below.  The column will
  @ be displayed on ticket reports if and only if its has a non-blank
  @ display name.  User's see the column as its display name, not as
  @ "extra1".
  @ </p>
  @
  @ <p>
  @ Allowed values for the column can be specified in the text box.
  @ The same format is used here
  @ as when specifying <a href="setup_enum?e=type">ticket types</a>,
  @ <a href="setup_enum?e=status">ticket states</a> and
  @ <a href="setup_enum?e=subsys">subsystem names</a>.
  @ There is one allowed value per line.  
  @ The token on the left is the value as it is stored in the database.
  @ The text that follows is a human-readable description for the meaning
  @ of the token.  A color name for use in reports may optionally appear
  @ in parentheses after the description.
  @ </p>
  @
  @ <p>
  @ The Allowed Values box may be left blank.
  @ If allowed values are defined for the column, then users will restricted
  @ to the values specified when changing the value of the column.
  @ If no allowed values are defined, then the column can be set to
  @ arbitrary text.
  @ </p>
  @
  @ <p>
  @ The Description box may be left blank. 
  @ If a description is provided, then this field may be entered on the
  @ new ticket page.  If no description is given, this field can be modified 
  @ on the edit ticket page but will not appear on the new ticket page.
  @ </p>
  for(i=0; i<5; i++){
    const char *zOld;
    char *zAllowed;
    const char *zDesc;
    char zEnumName[30];
    bprintf(zEnumName,sizeof(zEnumName),"extra%d_name",i+1);
    zOld = db_config(zEnumName,"");
    zEnumName[6] = 0;
    zAllowed = enum_to_string(zEnumName);
    bprintf(zEnumName,sizeof(zEnumName),"extra%d_desc",i+1);
    zDesc = db_config(zEnumName,"");
    @ <hr>
    @ <h3>Database column "extra%d(i+1)":</h3>
    @ <form action="%s(g.zPath)" method="POST">
    @ <input type="hidden" name="idx" value="%d(i+1)">
    @ Display Name:
    @ <input type="text" name="n" value="%h(zOld)"><br>
    @ Allowed Values: (<i>Name Desc Color</i> - omit for free text)<br>
    @ <textarea cols=60 rows=15 name="x">%h(zAllowed)</textarea><br>
    @ Description: (HTML - Leave blank to omit from new-ticket page)<br>
    @ <textarea cols=60 rows=5 name="d">%h(zDesc)</textarea><br>
    @ <input type="submit" value="Submit">
    @ </form>
  }
  common_footer();
}

/*
** WEBPAGE: /setup_chng
*/
void setup_chng_page(void){
  const char *zNotify, *zOldNotify;

  /* The user must be the setup user in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okSetup ){
    cgi_redirect("setup");
    return;
  }

  /*
  ** The "notify" query parameter is the name of a program or script that
  ** is run whenever a ticket is created or modified.  Modify the notify
  ** value if it has changed.  Only setup can do this.
  */
  zOldNotify = db_config("notify","");
  zNotify = P("notify");
  if( zNotify && strcmp(zOldNotify,zNotify)!=0 ){
    db_execute(
      "REPLACE INTO config(name,value) VALUES('notify','%q');",
      zNotify
    );
    zOldNotify = zNotify;
    db_config(0,0);
  }

  common_add_nav_item("setup", "Main Setup Menu");
  common_add_help_item("CvstracAdminNotification");
  common_header("Configure Ticket Change Notification");
  @ <p>
  @ <form action="%s(g.zPath)" method="POST">
  @ Enter a shell command to run whenever a ticket is
  @ created or modified.  The following substitutions are made
  @ on the string before it is passed to /bin/sh:
  @
  @ <table border=1 cellspacing=0 cellpadding=5 align="right" width="45%%">
  @ <tr><td bgcolor="#e0c0c0">
  @ <big><b>Important Security Note</b></big>
  @
  @ <p>Be sure to enclose all text substitutions in single-quotes.
  @ (ex <tt>'%%d'</tt>)  Otherwise, a user could cause arbitrary shell
  @ commands to be run on your system.</p>
  @  
  @ <p>Text is stripped of all single-quotes and backslashs before it is
  @ substituted, so if the substitution is itself enclosed in single-quotes,
  @ it will always be treated as a single token by the shell.</p>
  @
  @ <p>For best security, use only the <b>%%n</b> substitution and have
  @ a Tcl or Perl script extract other fields directly from the database.</p>
  @ </td></tr></table>
  @
  @ <blockquote>
  @ <table cellspacing="5" cellpadding="0">
  @ <tr><td width="40"><b>%%a</b></td>
  @     <td>UserID of the person the ticket is assigned to</td></tr>
  @ <tr><td><b>%%A</b></td><td>E-mail address of person assigned to</td></tr>
  @ <tr><td><b>%%c</b></td><td>Contact information for the originator</td></tr>
  @ <tr><td><b>%%d</b></td><td>The description, Wiki format</td></tr>
  @ <tr><td><b>%%D</b></td><td>The description, HTML format</td></tr>
  @ <tr><td><b>%%n</b></td><td>The ticket number</td></tr>
  @ <tr><td><b>%%p</b></td><td>The project name</td></tr>
  @ <tr><td><b>%%r</b></td><td>The remarks section, Wiki format</td></tr>
  @ <tr><td><b>%%R</b></td><td>The remarks section, HTML format</td></tr>
  @ <tr><td><b>%%s</b></td><td>The status of the ticket</td></tr>
  @ <tr><td><b>%%t</b></td><td>The title of the ticket</td></tr>
  @ <tr><td><b>%%u</b></td>
  @     <td>UserID of the person who made this change</td></tr>
  @ <tr><td><b>%%w</b></td><td>UserID of the originator of the ticket</td></tr>
  @ <tr><td><b>%%y</b></td><td>Type of ticket</td></tr>
  @ <tr><td><b>%%f</b></td><td>First TKTCHNG rowid of change set; zero if new record</td></tr>
  @ <tr><td><b>%%l</b></td><td>Last TKTCHNG rowid of change set; zero if new record</td></tr>
  @ <tr><td><b>%%h</b></td><td>attacHment number if change is a new attachment; zero otherwise</td></tr>
  @ <tr><td><b>%%1</b></td><td>First user-defined field</td></tr>
  @ <tr><td><b>%%2</b></td><td>Second user-defined field</td></tr>
  @ <tr><td><b>%%3</b></td><td>Third user-defined field</td></tr>
  @ <tr><td><b>%%4</b></td><td>Fourth user-defined field</td></tr>
  @ <tr><td><b>%%5</b></td><td>Fifth user-defined field</td></tr>
  @ <tr><td><b>%%%%</b></td><td>The literal character "<b>%%</b>"</td></tr>
  @ </table>
  @ </blockquote>
  @
  @ <input type="text" name="notify" size="70" value="%h(zOldNotify)">
  @ <input type="submit" value="Submit">
  @ </form>
  @ </p>
  common_footer();
}

/*
** WEBPAGE: /setup_diff
*/
void setup_diff_page(void){
  const char *zDiff, *zOldDiff;
  const char *zList, *zOldList;
  const char *zFilter, *zOldFilter;

  /* The user must be the setup user in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okSetup ){
    cgi_redirect("setup");
    return;
  }

  /*
  ** The "diff" query parameter is the name of a program or script that
  ** is run whenever a ticket is created or modified.  Modify the filediff
  ** value if it has changed.  Only setup can do this.
  */
  zOldDiff = db_config("filediff","");
  zDiff = P("diff");
  if( zDiff && strcmp(zOldDiff,zDiff)!=0 ){
    if( zDiff[0] ){
      db_execute(
        "REPLACE INTO config(name,value) VALUES('filediff','%q');",
        zDiff
      );
    }else{
      db_execute("DELETE FROM config WHERE name='filediff'");
    }
    zOldDiff = zDiff;
    db_config(0,0);
  }

  /*
  ** The "list" query parameter is the name of a program or script that
  ** is run whenever a ticket is created or modified.  Modify the filelist
  ** value if it has changed.  Only setup can do this.
  */
  zOldList = db_config("filelist","");
  zList = P("list");
  if( zList && strcmp(zOldList,zList)!=0 ){
    if( zList[0] ){
      db_execute(
        "REPLACE INTO config(name,value) VALUES('filelist','%q');",
        zList
      );
    }else{
      db_execute("DELETE FROM config WHERE name='filelist'");
    }
    zOldList = zList;
    db_config(0,0);
  }

  /*
  ** The "filter" query parameter is the name of a program or script that any
  ** files get filtered through for HTML markup.
  */
  zOldFilter = db_config("filefilter","");
  zFilter = P("filter");
  if( zFilter && strcmp(zOldFilter,zFilter)!=0 ){
    if( zFilter[0] ){
      db_execute(
        "REPLACE INTO config(name,value) VALUES('filefilter','%q');",
        zFilter
      );
    }else{
      db_execute("DELETE FROM config WHERE name='filefilter'");
    }
    zOldFilter = zFilter;
    db_config(0,0);
  }

  common_add_nav_item("setup", "Main Setup Menu");
  common_add_help_item("CvstracAdminFilter");
  common_header("Configure Source Code Diff Program");
  @ <form action="%s(g.zPath)" method="POST">
  @ <h2>File Diff</h2>
  @ <p>Enter a shell command to run in order to compute the difference between
  @ two versions of the same file.  The output can be either plain text
  @ or HTML.  If HTML, then the first non-whitespace character of output
  @ should be a "<".  Otherwise the output will be assumed to be plain text.</p>
  @
  @ <table border=1 cellspacing=0 cellpadding=5 align="right" width="33%%">
  @ <tr><td bgcolor="#e0c0c0">
  @ <big><b>Important Security Note</b></big>
  @
  @ <p>Be sure to enclose the substitutions in single-quotes.
  @ (examples: <tt>'%%F'</tt> or <tt>'%%V2'</tt>)
  @ Otherwise, a user who can check in new files
  @ (with unusual names) can cause arbitrary shell
  @ commands to be run on your system.</p>
  @  
  @ <p>CVSTrac will not attempt to diff a file whose name contains a
  @ single-quote or backslash
  @ so if the substitution is itself enclosed in single-quotes, it will always
  @ be treated as a single token by the shell.</p>
  @ </td></tr></table>
  @
  @ <p>The following substitutions are made prior to executing the program:</p>
  @
  @ <blockquote>
  @ <table cellspacing="5" cellpadding="0">
  @ <tr><td width="40" valign="top"><b>%%F</b></td>
  if( !strcmp(g.scm.zSCM,"cvs") ){
    @     <td>The name of the RCS file to be diffed.  This is a full
    @         pathname including the "<b>,v</b>" suffix.</td>
  }else{
    @     <td>The name of the file to be diffed.</td>
  }
  @ </tr>
  @ <tr><td><b>%%V1</b></td><td>The oldest version to be diffed</td></tr>
  @ <tr><td><b>%%V2</b></td><td>The newest version to be diffed</td></tr>
  @ <tr><td><b>%%RP</b></td><td>Path to repository</td></tr>
  @ <tr><td><b>%%%%</b></td><td>The literal character "<b>%%</b>"</td></tr>
  @ </table>
  @ </blockquote>
  @
  @ <input type="text" name="diff" size="70" value="%h(zOldDiff)">
  @ <input type="submit" value="Submit">
  @
  @ <p>If you leave the above entry blank, the following command is used:</p>
  @
  @ <blockquote><pre>
  if( !strcmp(g.scm.zSCM,"cvs") ){
    @ rcsdiff -q -r'%%V1' -r'%%V2' -u '%%F'
  }else{
    @ svnlook diff -r '%%V2' '%%RP'
  }
  @ </pre></blockquote>
  @ </form>
  @ </p>
  @ <hr>

  @ <form action="%s(g.zPath)" method="POST">
  @ <h2>File List</h2>
  @ <p>Enter below a shell command to run in order to list the content
  @ of a single version of a file <i>as a diff</i> (i.e. for the first
  @ revision of a file).  The output can be either plain text
  @ or HTML.  If HTML, then the first non-whitespace character of output
  @ should be a "<".  Otherwise the output will be assumed to be plain text.</p>
  @
  @ <p>This command is used to show the content 
  @ of files that are newly added to the repository.</p>
  @
  @ <p>The following substitutions are made prior to executing the program:</p>
  @
  @ <blockquote>
  @ <table cellspacing="5" cellpadding="0">
  @ <tr><td width="40" valign="top"><b>%%F</b></td>
  if( !strcmp(g.scm.zSCM,"cvs") ){
    @     <td>The name of the RCS file to be listed.  This is a full
    @         pathname including the "<b>,v</b>" suffix.</td>
  }else{
    @     <td>The name of the file to be listed.</td>
  }
  @ </tr>
  @ <tr><td><b>%%V</b></td><td>The version to be listed</td></tr>
  @ <tr><td><b>%%RP</b></td><td>Path to repository</td></tr>
  @ <tr><td><b>%%%%</b></td><td>The literal character "<b>%%</b>"</td></tr>
  @ </table>
  @ </blockquote>
  @
  @ <input type="text" name="list" size="70" value="%h(zOldList)">
  @ <input type="submit" value="Submit">
  @
  @ <p>If you leave the above entry blank, the following command is used:</p>
  @
  @ <blockquote><pre>
  if( !strcmp(g.scm.zSCM,"cvs") ){
    @ co -q -p'%%V' '%%F' | diff -c /dev/null -
  }else{
    @ svnlook cat -r '%%V' '%%RP' '%%F'
  }
  @ </pre></blockquote>
  @ </form>
  @ <hr>
  @ </p>

  @ <form action="%s(g.zPath)" method="POST">
  @ <h2>File Filter</h2>
  @ <p>Enter below a shell command to run in order to filter the contents
  @ of a single version of a file.  The filter should expect the file contents
  @ on standard input. The output can be either plain text
  @ or HTML.  If HTML, then the first non-whitespace character of output
  @ should be a "<".  Otherwise the output will be assumed to be plain text.</p>
  @
  @ <p>This command is used to show the content of files</p>
  @
  @ <p>The following substitutions are made prior to executing the program:</p>
  @
  @ <blockquote>
  @ <table cellspacing="5" cellpadding="0">
  @ <tr><td width="40" valign="top"><b>%%F</b></td>
  if( !strcmp(g.scm.zSCM,"cvs") ){
    @     <td>The name of the file to be diffed.  This is a relative
    @         pathname intended for display and content detection purposes.</td>
  }else{
    @     <td>The name of the file to be diffed.</td>
  }
  @ </tr>
  @ <tr><td><b>%%V</b></td><td>The version to be listed</td></tr>
  @ <tr><td><b>%%RP</b></td><td>Path to repository</td></tr>
  @ <tr><td><b>%%%%</b></td><td>The literal character "<b>%%</b>"</td></tr>
  @ </table>
  @ </blockquote>
  @
  @ <input type="text" name="filter" size="70" value="%h(zOldFilter)">
  @ <input type="submit" value="Submit">
  @
  @ <p>If you leave the above entry blank, output will simply be wrapped with
  @ HTML &lt;PRE&gt; tags and encoded as simple HTML.
  @ </form>
  @ </p>
  common_footer();
}

/*
** WEBPAGE: /setup_style
*/
void setup_style_page(void){
  const char *zHeader, *zFooter;

  /* The user must be the setup user in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okSetup ){
    cgi_redirect("setup");
    return;
  }

  /*
  ** If both "header" and "footer" query parameters are present, then
  ** change the header and footer to the values of those parameters.
  ** Only the setup user can do this.
  */
  if( P("ok") && P("header") && P("footer") ){
    db_execute(
      "REPLACE INTO config VALUES('header','%q');"
      "REPLACE INTO config VALUES('footer','%q');",
       trim_string(P("header")),
       trim_string(P("footer"))
    );
    db_config(0,0);
  }

  common_add_nav_item("setup", "Main Setup Menu");
  common_add_help_item("CvstracAdminStyle");
  if( attachment_max()>0 ){
    common_add_action_item("attach_add?tn=0", "Attach");
  }
  common_add_action_item("setup_style", "Cancel");
  common_header("Configure Style");
  @ <p>
  @ Enter HTML used for the header and footer of every page.
  @ If you leave these entries blank, default headers and/or footers
  @ are used.  If you enter a filename (beginning with a "/" character)
  @ instead of HTML text, then the
  @ file is read at runtime and used for the header or footer.</p>
  @
  @ <p>
  @ You may attach files to this page which can then be referenced from within
  @ your custom header/footer or from other pages. For example, stylesheets,
  @ JavaScript files, logos, icons, etc can all be attached. These attachments may
  @ be referenced directly by filename (i.e. <i>/filename.png</i>)
  @ rather than using <i>attach_get/89/filename.png</i> links.</p>
  @
  @ <p>Substitutions are made within the header and footer text.  These
  @ substitutions are made to the HTML regardless of whether the HTML
  @ is entered directly below or is read from a file.</p>
  @
  @ <blockquote>
  @ <table>
  @ <tr><td width="40"><b>%%N</b></td><td>The name of the project</td></tr>
  @ <tr><td><b>%%T</b></td><td>The title of the current page</td></tr>
  @ <tr><td><b>%%V</b></td><td>The version number of CVSTrac</td></tr>
  @ <tr><td><b>%%B</b></td><td>CVSTrac base URL</td></tr>
  @ <tr><td><b>%%%%</b></td><td>The literal character "<b>%%</b>"</td></tr>
  @ </table>
  @ </blockquote>
  @
  @ <p>
  @ <form action="%s(g.zPath)" method="POST">
  zHeader = db_config("header","");
  zFooter = db_config("footer","");

  /* user wants to restore the defaults */
  if( P("def") ){
    zHeader = HEADER;
    zFooter = FOOTER;
  }

  @ Header:<br>
  @ <textarea cols=80 rows=8 name="header">%h(zHeader)</textarea><br>
  @ Footer:<br>
  @ <textarea cols=80 rows=8 name="footer">%h(zFooter)</textarea><br>
  @ <input type="submit" name="ok" value="Submit">
  @ <input type="submit" name="def" value="Default">
  @ <input type="submit" name="can" value="Cancel">
  @ </form>
  @ </p>

  attachment_html("0","","");

  common_footer();
}

/*
** Make a copy of file zFrom into file zTo.  If any errors occur,
** return a pointer to a string describing the error.
*/
static const char *file_copy(const char *zFrom, const char *zTo){
  FILE *pIn, *pOut;
  int n;
  long long int total = 0;
  char zBuf[10240];
  pIn = fopen(zFrom, "r");
  if( pIn==0 ){
    return mprintf(
      "Unable to copy files - cannot open \"%h\" for reading.", zFrom
    );
  }
  unlink(zTo);
  pOut = fopen(zTo, "w");
  if( pOut==0 ){
    fclose(pIn);
    return mprintf(
      "Unable to copy files - cannot open \"%h\" for writing.", zTo
    );
  }
  while( (n = fread(zBuf, 1, sizeof(zBuf), pIn))>0 ){
    if( fwrite(zBuf, 1, n, pOut)<n ){
      fclose(pIn);
      fclose(pOut);
      return mprintf(
        "Copy operation failed after %lld bytes.  Is the disk full?", total
      );
    }
    total += n;
  }
  fclose(pIn);
  fclose(pOut);
  return 0;
}

/*
** WEBPAGE: /setup_attach
*/
void setup_attachment_page(void){
  /* The user must be the setup user in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okSetup ){
    cgi_redirect("setup");
    return;
  }

  if( P("sz") ){
    int sz = atoi(P("sz"))*1024;
    db_execute("REPLACE INTO config VALUES('max_attach_size',%d)", sz);
    db_config(0, 0);
    cgi_redirect("setup");
  }
 
  common_add_nav_item("setup", "Main Setup Menu");
  common_add_help_item("CvstracAdminAttachment");
  common_header("Set Maximum Attachment Size");
  @ <p>
  @ Enter the maximum attachment size below.  If you enter a size of
  @ zero, attachments are disallowed.
  @ </p>
  @
  @ <p>
  @ <form action="%s(g.zPath)" method="POST">
  @ Maximum attachment size in kilobytes: 
  @ <input type="text" name="sz" value="%d(attachment_max()/1024)" size=5>
  @ <input type="submit" value="Set">
  @ </form>
  @ </p>
  common_footer();
}

/*
** WEBPAGE: /setup_throttle
*/
void setup_throttle_page(void){
  int mxHit = atoi(db_config("throttle","0"));
  int nf = atoi(db_config("nofollow_link","0"));
  int cp = atoi(db_config("enable_captcha","0"));
  int lnk = atoi(db_config("max_links_per_edit","0"));
  int mscore = atoi(db_config("keywords_max_score","0"));
  const char *zKeys = db_config("keywords","");

  /* The user must be the setup user in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okSetup ){
    cgi_redirect("setup");
    return;
  }
  

  if( P("sz") && atoi(P("sz"))!=mxHit ){
    mxHit = atoi(P("sz"));
    db_execute("REPLACE INTO config VALUES('throttle',%d)", mxHit);
    db_config(0, 0);
  }

  if( P("nf") && atoi(P("nf"))!=nf ){
    nf = atoi(P("nf"));
    db_execute("REPLACE INTO config VALUES('nofollow_link',%d)", nf);
    db_config(0, 0);
  }
 
  if( P("cp") && atoi(P("cp"))!=cp ){
    cp = atoi(P("cp"));
    db_execute("REPLACE INTO config VALUES('enable_captcha',%d)", cp);
    db_config(0, 0);
  }
 
  if( P("lnk") && atoi(P("lnk"))!=lnk ){
    lnk = atoi(P("lnk"));
    db_execute("REPLACE INTO config VALUES('max_links_per_edit',%d)", lnk);
    db_config(0, 0);
  }

  if( P("mscore") && atoi(P("mscore"))!=mscore ){
    mscore = atoi(P("mscore"));
    db_execute("REPLACE INTO config VALUES('keywords_max_score',%d)", mscore);
    db_config(0, 0);
  }

  if( P("keys") && strcmp(zKeys,PD("keys","")) ){
    zKeys = P("keys");
    db_execute("REPLACE INTO config VALUES('keywords','%q')", zKeys);
    db_config(0, 0);
  }

  common_add_nav_item("setup", "Main Setup Menu");
  common_add_help_item("CvstracAdminAbuse");
  common_header("Abuse Controls");
  @ <h2>Set Maximum Anonymous Hits Per Hour</h2>
  @ <p>
  @ Enter the limit on the number of anonymous accesses from the same
  @ IP address that can occur within one hour.  Enter zero to disable
  @ the limiter.
  @ </p>
  @
  @ <p>
  @ <form action="%s(g.zPath)" method="POST">
  @ Maximum hits per hour: 
  @ <input type="text" name="sz" value="%d(mxHit)" size=5>
  @ <input type="submit" value="Set">
  @ </form>
  @ </p>
  @
  @ <p>
  @ The limiter works by maintain a table in the database (the ACCESS_LOAD
  @ table) that records the time of last access and a "load" for each
  @ IP address.  The load reduces exponentially with a half-life of
  @ one hour.  Each new access increases the load by 1.0.  When the
  @ load exceeds the threshold above, the load automatically doubles and
  @ the client is bounced to the <a href="captcha">captcha</a>
  @ page. After this redirection happens a
  @ few times, the user is denied access until the load decreases
  @ below the threshold. If the user passes the
  @ <a href="captcha">captcha</a> test, a cookie is set.
  @ </p>
  @
  @ <p>
  @ When the limiter is enabled, the <a href="captcha">captcha</a>
  @ page is also used to screen users before they try to do anything
  @ that might change the database (create a <a href="tktnew">new ticket</a>,
  @ <a href="wikiedit?p=WikiIndex">change a wiki page</a>, etc). This
  @ feature is intended to block automated wiki spam.
  @
  @ <p>
  @ Any attempt to access the page named "stopper" (reachable from
  @ <a href="honeypot">honeypot</a>) automatically increases
  @ the load to twice the threshold.  There are hyperlinks to the
  @ honeypot on every page.  The idea is to trick spiders into visiting
  @ this honeypot so that they can have their access terminated quickly.
  @ </p>
  @
  @ <p>
  @ The limiter and the honeypot only work for users that are not
  @ logged in - anonymous users.  Users who are logged in can visit
  @ any page (including the honeypot) as often as they want and will
  @ never be denied access. The limiter (but not the honeypot) is also
  @ disabled for any user with a valid <a href="captcha">captcha</a>
  @ cookie.
  @ </p>
  @
  @ <p>A summary of the <a href="info_throttle">Access Log</a> is available
  @ separately.</p>

  @ <hr>
  @ <h2>Captcha</h2>
  @ <form action="%s(g.zPath)" method="POST">
  @ <p>
  @ By turning on this option, anonymous users will be required to pass a
  @ simple <a href="http://en.wikipedia.org/wiki/Captcha">captcha</a>
  @ test before being allowed to change content (tickets, wiki, etc). Passing
  @ the test will set a cookie on the browser. Too many failures to pass
  @ the test will trigger the throttler and lock the users IP address out.
  @ Note that the rate limiter has to be enabled (non-zero) for this option
  @ to be available.
  @ </p>
  @ <p>
  @ <label for="cp"><input type="checkbox" name="cp" id="cp"
  @    %s(cp?" checked":"") %s(mxHit?"":"disabled") value="1">
  @ Turn on captcha for content changes.</label>
  @ </p>
  @ <input type="submit" value="Set">
  @ </form>
  @ <hr>

  @ <h2>External Links</h2>
  @ <form action="%s(g.zPath)" method="POST">
  @ <p>
  @ By turning on this option, all links to external sites are tagged as
  @ "nofollow". This provides a hint to search engines to ignore such links
  @ and reduces the value of wiki spam. However, this may be of limited use
  @ since wiki spammers aren't always smart enough to notice that they're
  @ wasting their time.
  @ </p>
  @ <p>
  @ <label for="nf"><input type="checkbox" name="nf" id="nf"
  @    %s(nf?" checked":"") value="1">
  @ Don't allow search engines to follow external links.</label>
  @ </p>
  @ <input type="submit" value="Set">
  @ </form>
  @ 
  @ <form action="%s(g.zPath)" method="POST">
  @ <p>
  @ Wiki spam generally works by inserting large numbers of links in a
  @ single page edit. A simple way to prevent this is to simply impose a
  @ maximum number of new external links in a single wiki edit.
  @ A value of zero will disable this option.
  @ </p>
  @ <p>
  @ Maximum external links per Wiki edit:
  @ <input type="text" name="lnk" value="%d(lnk)" size=5>
  @ </p>
  @ <input type="submit" value="Set">
  @ </form>
  @ <hr>
  @ <h2>Keyword Filtering</h2>
  @ <form action="%s(g.zPath)" method="POST">
  @ <p>
  @ Enter a space-separated list of keywords. All wiki edits will be
  @ checked against this list and, if the maximum score is exceeded, 
  @ the change will be denied. The scoring algorithm uses the standard
  @ CVSTrac text <strong>search()</strong> function (where each matched
  @ keyword scores from 6 to 10 points). Repeating a keyword in the
  @ list will cause it to score higher.
  @ </p>
  @ <p>
  cgi_text("mscore", 0, 0, 0, 0, 5, 8, 1, mprintf("%d",mscore),
           "Maximum keyword score");
  @ </p>
  @ <p>
  @ <h3>Forbidden Keywords</h3>
  @ <textarea name="keys" rows="8" cols="80" wrap="virtual">
  @ %h(zKeys)
  @ </textarea>
  @ </p>
  @ <input type="submit" value="Set">
  @ </form>

  common_footer();
}

/*
** WEBPAGE: /setup_markupedit
*/
void setup_markupedit_page(void){
  const char *zMarkup = PD("m","");
  const char *zType = PD("t","0");
  const char *zFormat = PD("f","");
  const char *zDescription = PD("d","");
  int delete = atoi(PD("del","0"));

  /* The user must be the setup user in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okSetup ){
    cgi_redirect("setup");
    return;
  }

  common_add_nav_item("setup", "Main Setup Menu");
  common_add_action_item("setup_markup", "Cancel");
  common_add_action_item(mprintf("setup_markupedit?m=%h&del=1",zMarkup),
                         "Delete");
  common_add_help_item("CvstracAdminMarkup");
  common_header("Custom Wiki Markup");

  if( P("can") ){
    cgi_redirect("setup_markup");
    return;
  }else if( P("ok") ){
    /* delete it */
    db_execute("DELETE FROM markup WHERE markup='%q';", zMarkup);
    cgi_redirect("setup_markup");
    return;
  }else if( delete && zMarkup[0] ){
    @ <p>Are you sure you want to delete markup <b>%h(zMarkup)</b>?</p>
    @
    @ <form method="POST" action="setup_markupedit">
    @ <input type="hidden" name="m" value="%h(zMarkup)">
    @ <input type="submit" name="ok" value="Yes, Delete">
    @ <input type="submit" name="can" value="No, Cancel">
    @ </form>
    common_footer();
    return;
  }

  if( P("u") ){
    if( zMarkup[0] && zType[0] && zFormat[0] ) {
      /* update database and bounce back to listing page. If the
      ** description is empty, we'll survive (and wing it).
      */
      db_execute("REPLACE INTO markup(markup,type,formatter,description) "
                 "VALUES('%q',%d,'%q','%q');",
                 zMarkup, atoi(zType), zFormat, zDescription);
    }

    cgi_redirect("setup_markup");
    return;
  }
  
  if( zMarkup[0] ){
    /* grab values from database, if available
    */
    char **az = db_query("SELECT type, formatter, description "
                         "FROM markup WHERE markup='%q';",
                         zMarkup);
    if( az && az[0] && az[1] && az[2] ){
      zType = az[0];
      zFormat = az[1];
      zDescription = az[2];
    }
  }

  @ <form action="%s(g.zPath)" method="POST">
  @ Markup Name: <input type="text" name="m" value="%h(zMarkup)" size=12>
  cgi_optionmenu(0,"t",zType, "Markup","0", "Block","2",
    "Program Markup","1", "Program Block","3", NULL);
  @ <br>Formatter:<br>
  @ <textarea name="f" rows="4" cols="60">%h(zFormat)</textarea><br>
  @ Description:<br>
  @ <textarea name="d" rows="4" cols="60">%h(zDescription)</textarea><br>
  @ <input type="hidden" name="u">
  @ <input type="submit" value="Set">
  @ </form>
  @
  @ <table border=1 cellspacing=0 cellpadding=5 align="right" width="45%%">
  @ <tr><td bgcolor="#e0c0c0">
  @ <big><b>Important Security Note</b></big>
  @
  @ <p>Program formatters execute external scripts and programs and
  @ improper configuration may result in a compromised server.</p>
  @
  @ <p>Be sure to enclose all text substitutions in single-quotes.
  @ (ex <tt>'%%k'</tt>)  Otherwise, a user could cause arbitrary shell
  @ commands to be run on your system.</p>
  @  
  @ <p>Text is stripped of all single-quotes and backslashs before it is
  @ substituted, so if the substitution is itself enclosed in single-quotes,
  @ it will always be treated as a single token by the shell.</p>
  @ </td></tr></table>
  @
  @ The following substitutions are made on the custom markup:
  @ <blockquote>
  @ <table cellspacing="5" cellpadding="0">
  @ <tr><td width="40"><b>%%m</b></td><td>markup name</td></tr>
  @ <tr><td><b>%%k</b></td><td>markup key</td></tr>
  @ <tr><td><b>%%a</b></td><td>markup arguments</td></tr>
  @ <tr><td><b>%%x</b></td><td>markup arguments or, if empty, key</td></tr>
  @ <tr><td><b>%%b</b></td><td>markup block</td></tr>
  @ <tr><td><b>%%r</b></td><td>%s(g.scm.zName) repository root</td></tr>
  @ <tr><td><b>%%n</b></td><td>CVSTrac project name</td></tr>
  @ <tr><td><b>%%u</b></td><td>Current user</td></tr>
  @ <tr><td><b>%%c</b></td><td>User capabilities</td></tr>
  @ </table>
  @ </blockquote>
  @
  @ Additionally, external programs have some or all the following
  @ environment variables defined:<br>
  @ REQUEST_METHOD, GATEWAY_INTERFACE, REQUEST_URI, PATH_INFO,
  @ QUERY_STRING, REMOTE_ADDR, HTTP_USER_AGENT, CONTENT_LENGTH,
  @ HTTP_REFERER, HTTP_HOST, CONTENT_TYPE, HTTP_COOKIE
  @ <br>
  @
  @ <h2>Notes</h2>
  @ <ul>
  @   <li>The markup name is the wiki formatting tag. i.e. a markup named
  @   <b>echo</b> would be invoked using <tt>{echo: key args}</tt></li>
  @   <li>Changing the name of an existing markup may break existing
  @   wiki pages</li>
  @   <li>"Markup" markups are simple string substitutions and are handled
  @   directly by CVSTrac</li>
  @   <li>"Block" markups are paired {markup} and {endmarkup} which get
  @   all the text in between as arguments (%a), with no key.</li>
  @   <li>"Program" markups are handled by running external scripts and
  @   programs. These are more flexible, but there are security risks and
  @   too many may slow down page creation. A Program Markup gets the
  @   arguments on the command line while a Program Block also gets the block
  @   from standard input. Both forms should write HTML to standard output</li>
  @   <li>The Description field is used when enumerating the list of available
  @   custom markups using the {markups} tag. This is included in pages
  @   such as <a href="wiki?p=WikiFormatting">WikiFormatting</a> in order to
  @   document server-specific markups.</li>
  @ </ul>

  common_footer();
}

/*
** WEBPAGE: /setup_markup
*/
void setup_markup_page(void){
  int j;
  char **az;

  /* The user must be the setup user in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okSetup ){
    cgi_redirect("setup");
    return;
  }

  common_add_nav_item("setup", "Main Setup Menu");
  common_add_action_item("setup_markupedit", "Add Markup");
  common_add_help_item("CvstracAdminMarkup");
  common_header("Custom Wiki Markup");

  az = db_query("SELECT markup, description FROM markup ORDER BY markup;");
  if( az && az[0] ){
    @ <p><big><b>Custom Markup Rules</b></big></p>
    @ <dl>
    for(j=0; az[j]; j+=2){
      @ <dt><a href="setup_markupedit?m=%h(az[j])">%h(az[j])</a></dt>
      if( az[j+1] && az[j+1][0] ){
        /* this markup has a description, output it.
        */
        @ <dd>
        output_formatted(az[j+1],NULL);
        @ </dd>
      }else{
        @ <dd>(no description)</dd>
      }
    }
    @ </dl>
  }

  common_footer();
}

/*
** WEBPAGE: /setup_toolsedit
*/
void setup_toolsedit_page(void){
  const char *zTool = PD("t","");
  const char *zObject = PD("o","");
  const char *zCommand = PD("c","");
  const char *zDescription = PD("d","");
  const char *zPerms = PD("p","as");
  int delete = atoi(PD("del","0"));

  /* The user must be the setup user in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okSetup ){
    cgi_redirect("setup");
    return;
  }

  common_add_nav_item("setup", "Main Setup Menu");
  common_add_action_item("setup_tools", "Cancel");
  common_add_action_item(mprintf("setup_toolsedit?t=%h&del=1",zTool),
                         "Delete");
  common_header("External Tools");

  if( P("can") ){
    cgi_redirect("setup_tools");
    return;
  }else if( P("ok") ){
    /* delete it */
    db_execute("DELETE FROM tool WHERE name='%q';", zTool);
    cgi_redirect("setup_tools");
    return;
  }else if( delete && zTool[0] ){
    @ <p>Are you sure you want to delete tool <b>%h(zTool)</b>?</p>
    @
    @ <form method="POST" action="setup_toolsedit">
    @ <input type="hidden" name="t" value="%h(zTool)">
    @ <input type="submit" name="ok" value="Yes, Delete">
    @ <input type="submit" name="can" value="No, Cancel">
    @ </form>
    common_footer();
    return;
  }

  if( P("u") ){
    if( zTool[0] && zPerms[0] && zObject[0] && zCommand[0] ) {
      /* update database and bounce back to listing page. If the
      ** description is empty, we'll survive (and wing it).
      */
      db_execute("REPLACE INTO tool(name,perms,object,command,description) "
                 "VALUES('%q','%q','%q','%q','%q');",
                 zTool, zPerms, zObject, zCommand, zDescription);
    }

    cgi_redirect("setup_tools");
  }
  
  if( zTool[0] ){
    /* grab values from database, if available
    */
    char **az = db_query("SELECT perms, object, command, description "
                         "FROM tool WHERE name='%q';",
                         zTool);
    if( az && az[0] && az[1] && az[2] && az[3] ){
      zPerms = az[0];
      zObject = az[1];
      zCommand = az[2];
      zDescription = az[3];
    }
  }

  @ <form action="%s(g.zPath)" method="POST">
  @ Tool Name: <input type="text" name="t" value="%h(zTool)" size=12>
  cgi_optionmenu(0,"o",zObject,
                 "File","file",
                 "Wiki","wiki",
                 "Ticket","tkt",
                 "Check-in","chng",
                 "Milestone","ms",
                 "Report", "rpt",
                 "Directory", "dir",
                 NULL);
  @ <br>Required Permissions:
  @ <input type="text" name="p" size=16 value="%h(zPerms)"><br>
  @ <br>Command-line:<br>
  @ <textarea name="c" rows="4" cols="60">%h(zCommand)</textarea><br>
  @ Description:<br>
  @ <textarea name="d" rows="4" cols="60">%h(zDescription)</textarea><br>
  @ <input type="hidden" name="u">
  @ <input type="submit" value="Set">
  @ </form>
  @
  @ <table border=1 cellspacing=0 cellpadding=5 align="right" width="45%%">
  @ <tr><td bgcolor="#e0c0c0">
  @ <big><b>Important Security Note</b></big>
  @
  @ <p>External scripts and programs and
  @ improper configuration may result in a compromised server.</p>
  @
  @ <p>Be sure to enclose all text substitutions in single-quotes.
  @ (ex <tt>'%%k'</tt>)  Otherwise, a user could cause arbitrary shell
  @ commands to be run on your system.</p>
  @
  @ <p>Text is stripped of all single-quotes and backslashs before it is
  @ substituted, so if the substitution is itself enclosed in single-quotes,
  @ it will always be treated as a single token by the shell.</p>
  @
  @ <p>Each tool can have a minimum permission set defined. See
  @ <a href="userlist">Users</a> for the full list.</p>
  @ </td></tr></table>
  @
  @ The following substitutions are available to all external tools:
  @ <blockquote>
  @ <table cellspacing="5" cellpadding="0">
  @ <tr><td><b>%%RP</b></td><td>%s(g.scm.zName) repository root</td></tr>
  @ <tr><td><b>%%P</b></td><td>CVSTrac project name</td></tr>
  @ <tr><td><b>%%B</b></td><td>Server base URL</td></tr>
  @ <tr><td><b>%%U</b></td><td>Current user</td></tr>
  @ <tr><td><b>%%UC</b></td><td>User capabilities</td></tr>
  @ <tr><td><b>%%N</b></td><td>Current epoch time</td></tr>
  @ <tr><td><b>%%T</b></td><td>Name of tool</td></tr>
  @ </table>
  @ </blockquote>
  @
  @ File tools have the following substitutions available:
  @ <blockquote>
  @ <table cellspacing="5" cellpadding="0">
  @ <tr><td><b>%%F</b></td><td>Filename</td></tr>
  @ <tr><td><b>%%V1</b></td><td>First version number</td></tr>
  @ <tr><td><b>%%V2</b></td><td>Second version number (i.e. diff)</td></tr>
  @ </table>
  @ </blockquote>
  @
  @ Directory tools have the following substitutions available:
  @ <blockquote>
  @ <table cellspacing="5" cellpadding="0">
  @ <tr><td><b>%%F</b></td><td>Directory pathname</td></tr>
  @ </table>
  @ </blockquote>
  @
  @ Ticket tools have the following substitutions available:
  @ <blockquote>
  @ <table cellspacing="5" cellpadding="0">
  @ <tr><td><b>%%TN</b></td><td>Ticket number</td></tr>
  @ </table>
  @ </blockquote>
  @
  @ Wiki tools have the following substitutions available:
  @ <blockquote>
  @ <table cellspacing="5" cellpadding="0">
  @ <tr><td><b>%%W</b></td><td>Wiki page name</td></tr>
  @ <tr><td><b>%%T1</b></td><td>First timestamp of wiki page</td></tr>
  @ <tr><td><b>%%T2</b></td><td>Second timestamp of wiki page (i.e. diff)
  @            </td></tr>
  @ <tr><td><b>%%C</b></td><td>Temporary file containing content</td></tr>
  @ </table>
  @ </blockquote>
  @
  @ Check-in tools have the following substitutions available:
  @ <blockquote>
  @ <table cellspacing="5" cellpadding="0">
  @ <tr><td><b>%%CN</b></td><td>Check-in number</td></tr>
  @ <tr><td><b>%%C</b></td><td>Temporary file containing message</td></tr>
  @ </table>
  @ </blockquote>
  @
  @ Milestone tools have the following substitutions available:
  @ <blockquote>
  @ <table cellspacing="5" cellpadding="0">
  @ <tr><td><b>%%MS</b></td><td>Milestone number</td></tr>
  @ <tr><td><b>%%C</b></td><td>Temporary file containing message</td></tr>
  @ </table>
  @ </blockquote>
  @
  @ Report tools have the following substitutions available:
  @ <blockquote>
  @ <table cellspacing="5" cellpadding="0">
  @ <tr><td><b>%%RN</b></td><td>Report number</td></tr>
  @ </table>
  @ </blockquote>
  @
  @ Additionally, external programs have some or all the following
  @ environment variables defined:<br>
  @ REQUEST_METHOD, GATEWAY_INTERFACE, REQUEST_URI, PATH_INFO,
  @ QUERY_STRING, REMOTE_ADDR, HTTP_USER_AGENT, CONTENT_LENGTH,
  @ HTTP_REFERER, HTTP_HOST, CONTENT_TYPE, HTTP_COOKIE
  @ <br>
  common_footer();
}

/*
** WEBPAGE: /setup_tools
*/
void setup_tools_page(void){
  int j;
  char **az;

  /* The user must be the setup user in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okSetup ){
    cgi_redirect("setup");
    return;
  }

  common_add_nav_item("setup", "Main Setup Menu");
  common_add_action_item("setup_toolsedit", "Add Tool");
  common_header("External Tools");

  az = db_query("SELECT name, description FROM tool ORDER BY name;");
  if( az && az[0] ){
    @ <p><big><b>External Tools</b></big></p>
    @ <dl>
    for(j=0; az[j]; j+=2){
      @ <dt><a href="setup_toolsedit?t=%h(az[j])">%h(az[j])</a></dt>
      if( az[j+1] && az[j+1][0] ){
        /* this tool has a description, output it.
        */
        @ <dd>
        output_formatted(az[j+1],NULL);
        @ </dd>
      }else{
        @ <dd>(no description)</dd>
      }
    }
    @ </dl>
  }

  common_footer();
}

/*
** WEBPAGE: /setup_backup
*/
void setup_backup_page(void){
  char *zDbName = mprintf("%s.db", g.zName);
  char *zBuName = mprintf("%s.db.bu", g.zName);
  const char *zMsg = 0;

  /* The user must be the setup user in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okSetup ){
    cgi_redirect("setup");
    return;
  }

  if( P("bkup") ){
    db_execute("BEGIN");
    zMsg = file_copy(zDbName, zBuName);
    db_execute("COMMIT");
  }else if( P("rstr") ){
    db_execute("BEGIN");
    zMsg = file_copy(zBuName, zDbName);
    db_execute("COMMIT");
  }
 
  common_add_nav_item("setup", "Main Setup Menu");
  common_add_help_item("CvstracAdminBackup");
  common_header("Backup The Database");
  if( zMsg ){
    @ <p><font color="#ff0000">%s(zMsg)</font></p>
  }
  @ <p>
  @ Use the buttons below to make a safe (atomic) backup or restore
  @ of the database file.   The original database is in the file
  @ named <b>%h(zDbName)</b> and the backup is in 
  @ <b>%h(zBuName)</b>.
  @ </p>
  @
  @ <p>
  @ It is always safe to do a backup.  The worst that can happen is that
  @ you can overwrite a prior backup.  But a restore can destroy your
  @ database if the backup copy you are restoring from is incorrect.
  @ Use caution when doing a restore.
  @ </p>
  @
  @ <form action="%s(g.zPath)" method="POST">
  @ <p><input type="submit" name="bkup" value="Backup"></p>
  @ <p><input type="submit" name="rstr" value="Restore"></p>
  @ </form>
  common_footer();
}

/*
** WEBPAGE: /setup_timeline
*/
void setup_timeline_page(void){
  int nCookieLife;
  int nTTL;
  int nRDL;
  
  /* The user must be the setup user in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okSetup ){
    cgi_redirect("setup");
    return;
  }
  
  if( P("cl") || P("ttl") || P("rdl") ){
    if( P("cl") ){
      int nCookieLife = atoi(P("cl"));
      db_execute("REPLACE INTO config VALUES('timeline_cookie_life',%d)", nCookieLife);
    }
    if( P("ttl") ){
      int nTTL = atoi(P("ttl"));
      db_execute("REPLACE INTO config VALUES('rss_ttl',%d)", nTTL);
    }
    if( P("rdl") ){
      int nRDL = atoi(P("rdl"));
      db_execute("REPLACE INTO config VALUES('rss_detail_level',%d)", nRDL);
    }
    db_config(0, 0);
  }
  
  nCookieLife = atoi(db_config("timeline_cookie_life", "90"));
  nTTL = atoi(db_config("rss_ttl", "60"));
  nRDL = atoi(db_config("rss_detail_level", "5"));
  
  common_add_nav_item("setup", "Main Setup Menu");
  common_header("Timeline & RSS Setup");
  @ <form action="%s(g.zPath)" method="POST">
  @ <p>
  @ Enter number of days timeline cookie should be kept by users browser.
  @ This cookie keeps timeline settings persistent across users multiple visits.<br>
  @ This applies to all users.<br>
  @ Set it to 0 to disable timeline cookie.
  @ </p>
  @ <p>
  @ Cookie lifetime: 
  @ <input type="text" name="cl" value="%d(nCookieLife)" size=5> days
  @ <input type="submit" value="Set">
  @ </p>
  @ <hr>
  @ <p>
  @ RSS feed's TTL (Time To Live) tells RSS readers how long a feed should
  @ be cached before refreshing from the source. Because a refresh
  @ downloads the entire page, in order to avoid excessive use of
  @ bandwidth this shouldn't be set too low. Anything lower then 15
  @ is probably not a very good idea, while 30-60 is most common.
  @ </p>
  @ <p>
  @ Time To Live:
  @ <input type="text" name="ttl" value="%d(nTTL)" size=5> minutes
  @ <input type="submit" value="Set">
  @ </p>
  @ <hr>
  @ <p>
  @ RSS feed's detail level determines how much details will be
  @ embedded in feed.<br>
  @ Higher the detail level, higher the bandwidth usage will be.
  @ </p>
  @ <p>
  @ RSS detail level:<br>
  @ <label for="rdl0"><input type="radio" name="rdl" value="0" id="rdl0"
  @ %s(nRDL==0?" checked":"")> Basic</label><br>
  @ <label for="rdl5"><input type="radio" name="rdl" value="5" id="rdl5"
  @ %s(nRDL==5?" checked":"")> Medium</label><br>
  @ <label for="rdl9"><input type="radio" name="rdl" value="9" id="rdl9"
  @ %s(nRDL==9?" checked":"")> High</label><br>
  @ <input type="submit" value="Set">
  @ </p>
  @ </form>
  common_footer();
}

#if 0  /* TO DO */
/*
** WEB-PAGE: /setup_repair
*/
void setup_repair_page(void){

  /* The user must be the setup user in order to see
  ** this screen.
  */
  login_check_credentials();
  if( !g.okSetup ){
    cgi_redirect("setup");
    return;
  }

  /* 
  ** Change a check-in number.
  */
  cnfrom = atoi(PD("cnfrom"),"0");
  cnto = atoi(PD("cnto"),"0");
  if( cnfrom>0 && cnto>0 && cnfrom!=cnto ){
    const char *zOld;
    zOld = db_short_query(
       "SELECT rowid FROM chng WHERE cn=%d", cnfrom
    );
    if( zOld || zOld[0] ){
      db_execute(
        "BEGIN;"
        "DELETE FROM chng WHERE cn=%d;"
        "UPDATE chng SET cn=%d WHERE cn=%d;"
        "UPDATE filechng SET cn=%d WHERE cn=%d;"
        "UPDATE xref SET cn=%d WHERE cn=%d;"
        "COMMIT;",
        cnto,
        cnto, cnfrom,
        cnto, cnfrom,
        cnto, cnfrom
      );
    }
  }

  /*
  ** Remove duplicate entries in the FILECHNG table.  Remove check-ins
  ** from the CHNG table that have no corresponding FILECHNG entries.
  */
  if( P("rmdup") ){
    db_execute(
      "BEGIN;"
      "DELETE FROM filechng WHERE rowid NOT IN ("
         "SELECT min(rowid) FROM filechng "
         "GROUP BY filename, vers||'x'"
      ");"
      "DELETE FROM chng WHERE milestone=0 AND cn NOT IN ("
         "SELECT cn FROM filechng"
      ");"
      "COMMIT;"
    );
  }

  common_add_nav_item("setup", "Main Setup Menu");
  common_header("Repair The Database");
  @ <p>
  @ You can use this page to repair damage to a database that was caused
  @ when the repository was read incorrectly.  The problem may
  @ have resulted from corruption in the %s(g.scm.zName) repository or a system 
  @ malfunction or from a bug in CVSTrac.  (All known bugs of this kind have
  @ been fixed but you never know when a new one might appear.)
  @ </p>
  @
  @ 
  @ </p>
  common_footer();
}
#endif

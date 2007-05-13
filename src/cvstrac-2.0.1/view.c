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
** Code to generate the bug report listings
*/
#include "config.h"
#include "view.h"

/* Forward references to static routines */
static void report_format_hints(void);

/*
** WEBPAGE: /reportlist
*/
void view_list(void){
  char **az;
  int i;

  login_check_credentials();
  if( !g.okRead ){ login_needed(); return; }
  throttle(1,0);
  common_standard_menu("reportlist", "search?t=1");
  if( g.okQuery ){
    common_add_action_item("rptnew", "New Report Format");
  }
  common_add_help_item("CvstracReport");
  common_header("Available Report Formats");
  az = db_query("SELECT rn, title, owner FROM reportfmt ORDER BY title");
  @ <p>Choose a report format from the following list:</p>
  @ <ol>
  for(i=0; az[i]; i+=3){
    @ <li><a href="rptview?rn=%t(az[i])"
    @        rel="nofollow">%h(az[i+1])</a>&nbsp;&nbsp;&nbsp;
    if( g.okWrite && az[i+2] && az[i+2][0] ){
      @ (by <i>%h(az[i+2])</i>)
    }
    if( g.okQuery ){
      @ [<a href="rptedit?rn=%t(az[i])&amp;copy=1" rel="nofollow">copy</a>]
    }
    if( g.okAdmin || (g.okQuery && strcmp(g.zUser,az[i+2])==0) ){
      @ [<a href="rptedit?rn=%t(az[i])" rel="nofollow">edit</a>]
    }
    @ [<a href="rptsql?rn=%t(az[i])" rel="nofollow">sql</a>]
    @ </li>
  }
  if( g.okQuery ){
    @ <p><li><a href="rptnew">Create a new report format</a></li></p>
  }
  @ </ol>
  common_footer();
}

/*
** Remove whitespace from both ends of a string.
*/
char *trim_string(const char *zOrig){
  int i;
  while( isspace(*zOrig) ){ zOrig++; }
  i = strlen(zOrig);
  while( i>0 && isspace(zOrig[i-1]) ){ i--; }
  return mprintf("%.*s", i, zOrig);
}

/*
** Extract a numeric (integer) value from a string.
*/
char *extract_integer(const char *zOrig){
  if( zOrig == NULL || zOrig[0] == 0 ) return "";
  while( *zOrig && !isdigit(*zOrig) ){ zOrig++; }
  if( *zOrig ){
    /* we have a digit. atoi() will get as much of the number as it
    ** can. We'll run it through mprintf() to get a string. Not
    ** an efficient way to do it, but effective.
    */
    return mprintf("%d", atoi(zOrig));
  }
  return "";
}

/*
** Remove blank lines from the beginning of a string and
** all whitespace from the end. Removes whitespace preceeding a NL,
** which also converts any CRNL sequence into a single NL.
*/
char *remove_blank_lines(const char *zOrig){
  int i, j, n;
  char *z;
  for(i=j=0; isspace(zOrig[i]); i++){ if( zOrig[i]=='\n' ) j = i+1; }
  n = strlen(&zOrig[j]);
  while( n>0 && isspace(zOrig[j+n-1]) ){ n--; }
  z = mprintf("%.*s", n, &zOrig[j]);
  for(i=j=0; z[i]; i++){
    if( z[i+1]=='\n' && z[i]!='\n' && isspace(z[i]) ){
      z[j] = z[i];
      while(isspace(z[j]) && z[j] != '\n' ){ j--; }
      j++;
      continue;
    }

    z[j++] = z[i];
  }
  z[j] = 0;
  return z;
}

/*********************************************************************/
/*
** wiki_key(), tkt_key() and chng_key() generate what should be unique
** wrappers around field values which indicate the desired formatting of
** the field. output_report_field() takes care of the formatting when
** it detects a specific wrapper. The wrapper should not be something
** which ever occurs in user-content. Right now, the wrapper is just some
** random junk with a keyword, but if there's collisions we can tweak it,
** maybe to something more dependent on the field contents?
*/

static const char* wiki_key(){
  static char key[64];
  if( key[0]==0 ){
    bprintf(key,sizeof(key),"wiki_%d:",rand());
  }
  return key;
}

static const char* tkt_key(){
  static char key[64];
  if( key[0]==0 ){
    bprintf(key,sizeof(key),"tkt_%d:",rand());
  }
  return key;
}

static const char* chng_key(){
  static char key[64];
  if( key[0]==0 ){
    bprintf(key,sizeof(key),"chng_%d:",rand());
  }
  return key;
}

static void f_wiki(sqlite3_context *context, int argc, sqlite3_value **argv){
  char *zText;
  if( argc!=1 ) return;
  zText = (char*)sqlite3_value_text(argv[0]);
  if(zText==0) return;
  zText = mprintf("%s%s",wiki_key(),zText);
  sqlite3_result_text(context, zText, -1, SQLITE_TRANSIENT);
  free(zText);
}

static void f_tkt(sqlite3_context *context, int argc, sqlite3_value **argv){
  char *zText;
  int tn;
  if( argc!=1 ) return;
  tn = sqlite3_value_int(argv[0]);
  zText = mprintf("%s%d",tkt_key(),tn);
  sqlite3_result_text(context, zText, -1, SQLITE_TRANSIENT);
  free(zText);
}

static void f_chng(sqlite3_context *context, int argc, sqlite3_value **argv){
  char *zText;
  int cn;
  if( argc!=1 ) return;
  cn = sqlite3_value_int(argv[0]);
  zText = mprintf("%s%d",chng_key(),cn);
  sqlite3_result_text(context, zText, -1, SQLITE_TRANSIENT);
  free(zText);
}

static void f_dummy(sqlite3_context *context, int argc, sqlite3_value **argv){
  char *zText;
  if( argc!=1 ) return;
  zText = (char*)sqlite3_value_text(argv[0]);
  if( zText!= 0 ){
    sqlite3_result_text(context, zText, -1, SQLITE_TRANSIENT);
  }
}

static void view_add_functions(int tabs){
  sqlite3 *db = db_open();
  if( tabs ){
    /* non-HTML output, just turn these functions into pass-throughs */
    sqlite3_create_function(db, "wiki", 1, SQLITE_ANY, 0, &f_dummy, 0, 0);
    sqlite3_create_function(db, "tkt", 1, SQLITE_ANY, 0, &f_dummy, 0, 0);
    sqlite3_create_function(db, "chng", 1, SQLITE_ANY, 0, &f_dummy, 0, 0);
  }else{
    sqlite3_create_function(db, "wiki", 1, SQLITE_ANY, 0, &f_wiki, 0, 0);
    sqlite3_create_function(db, "tkt", 1, SQLITE_ANY, 0, &f_tkt, 0, 0);
    sqlite3_create_function(db, "chng", 1, SQLITE_ANY, 0, &f_chng, 0, 0);
  }
}

/*********************************************************************/
/*
** Check the given SQL to see if is a valid query that does not
** attempt to do anything dangerous.  Return 0 on success and a
** pointer to an error message string (obtained from malloc) if
** there is a problem.
*/
extern int sqlite3StrNICmp(const char*,const char*,int);
extern int sqlite3StrICmp(const char*,const char*);
char *verify_sql_statement(char *zSql){
  int i;

  /* First make sure the SQL is a single query command by verifying that
  ** the first token is "SELECT" and that there are no unquoted semicolons.
  */
  for(i=0; isspace(zSql[i]); i++){}
  if( sqlite3StrNICmp(&zSql[i],"select",6)!=0 ){
    return mprintf("The SQL must be a SELECT statement");
  }
  for(i=0; zSql[i]; i++){
    if( zSql[i]==';' ){
      int bad;
      int c = zSql[i+1];
      zSql[i+1] = 0;
      bad = sqlite3_complete(zSql);
      zSql[i+1] = c;
      if( bad ){
        /* A complete statement basically means that an unquoted semi-colon
        ** was found. We don't actually check what's after that.
        */
        return mprintf("Semi-colon detected! "
                       "Only a single SQL statement is allowed");
      }
    }
  }
  return 0;  
}

/*
** WEBPAGE: /rptsql
*/
void view_see_sql(void){
  int rn;
  char *zTitle;
  char *zSQL;
  char *zOwner;
  char *zClrKey;
  char **az;

  login_check_credentials();
  if( !g.okRead ){
    login_needed();
    return;
  }
  throttle(1,0);
  rn = atoi(PD("rn","0"));
  az = db_query("SELECT title, sqlcode, owner, cols "
                "FROM reportfmt WHERE rn=%d",rn);
  common_standard_menu(0, 0);
  common_add_help_item("CvstracReport");
  common_add_action_item( mprintf("rptview?rn=%d",rn), "View");
  common_header("SQL For Report Format Number %d", rn);
  if( az[0]==0 ){
    @ <p>Unknown report number: %d(rn)</p>
    common_footer();
    return;
  }
  zTitle = az[0];
  zSQL = az[1];
  zOwner = az[2];
  zClrKey = az[3];
  @ <table cellpadding=0 cellspacing=0 border=0>
  @ <tr><td valign="top" align="right">Title:</td><td width=15></td>
  @ <td colspan=3>%h(zTitle)</td></tr>
  @ <tr><td valign="top" align="right">Owner:</td><td></td>
  @ <td colspan=3>%h(zOwner)</td></tr>
  @ <tr><td valign="top" align="right">SQL:</td><td></td>
  @ <td valign="top"><pre>
  @ %h(zSQL)
  @ </pre></td>
  @ <td width=15></td><td valign="top">
  output_color_key(zClrKey, 0, "border=0 cellspacing=0 cellpadding=3");
  @ </td>
  @ </tr></table>
  report_format_hints();
  common_footer();
}

/*
** WEBPAGE: /rptnew
** WEBPAGE: /rptedit
*/
void view_edit(void){
  int rn;
  const char *zTitle;
  const char *z;
  const char *zOwner;
  char *zClrKey;
  char *zSQL;
  char *zErr = 0;

  login_check_credentials();
  if( !g.okQuery ){
    login_needed();
    return;
  }
  throttle(1,1);
  db_add_functions();
  view_add_functions(0);
  rn = atoi(PD("rn","0"));
  zTitle = P("t");
  zOwner = PD("w",g.zUser);
  z = P("s");
  zSQL = z ? trim_string(z) : 0;
  zClrKey = trim_string(PD("k",""));
  if( rn>0 && P("del2") ){
    db_execute("DELETE FROM reportfmt WHERE rn=%d", rn);
    cgi_redirect("reportlist");
    return;
  }else if( rn>0 && P("del1") ){
    zTitle = db_short_query("SELECT title FROM reportfmt "
                            "WHERE rn=%d", rn);
    if( zTitle==0 ) cgi_redirect("reportlist");

    common_add_action_item(mprintf("rptview?rn=%d",rn), "Cancel");
    common_header("Are You Sure?");
    @ <form action="rptedit" method="POST">
    @ <p>You are about to delete all traces of the report
    @ <strong>%h(zTitle)</strong> from
    @ the database.  This is an irreversible operation.  All records
    @ related to this report will be removed and cannot be recovered.</p>
    @
    @ <input type="hidden" name="rn" value="%d(rn)">
    @ <input type="submit" name="del2" value="Delete The Report">
    @ <input type="submit" name="can" value="Cancel">
    @ </form>
    common_footer();
    return;
  }else if( P("can") ){
    /* user cancelled */
    cgi_redirect("reportlist");
    return;
  }
  if( zTitle && zSQL ){
    if( zSQL[0]==0 ){
      zErr = "Please supply an SQL query statement";
    }else if( (zTitle = trim_string(zTitle))[0]==0 ){
      zErr = "Please supply a title"; 
    }else if( (zErr = verify_sql_statement(zSQL))!=0 ){
      /* empty... zErr non-zero */
    }else{
      /* check query syntax by actually trying the query */
      db_restrict_access(1);
      zErr = db_query_check("%s", zSQL);
      if( zErr ) zErr = mprintf("%s",zErr);
      db_restrict_access(0);
    }
    if( zErr==0 ){
      if( rn>0 ){
        db_execute("UPDATE reportfmt SET title='%q', sqlcode='%q',"
                   " owner='%q', cols='%q' WHERE rn=%d",
           zTitle, zSQL, zOwner, zClrKey, rn);
      }else{
        db_execute("INSERT INTO reportfmt(title,sqlcode,owner,cols) "
           "VALUES('%q','%q','%q','%q')",
           zTitle, zSQL, zOwner, zClrKey);
        z = db_short_query("SELECT max(rn) FROM reportfmt");
        rn = atoi(z);
      }
      cgi_redirect(mprintf("rptview?rn=%d", rn));
      return;
    }
  }else if( rn==0 ){
    zTitle = "";
    zSQL = 
      @ SELECT
      @   CASE WHEN status IN ('new','active') THEN '#f2dcdc'
      @        WHEN status='review' THEN '#e8e8bd'
      @        WHEN status='fixed' THEN '#cfe8bd'
      @        WHEN status='tested' THEN '#bde5d6'
      @        WHEN status='defer' THEN '#cacae5'
      @        ELSE '#c8c8c8' END AS 'bgcolor',
      @   tn AS '#',
      @   type AS 'Type',
      @   status AS 'Status',
      @   sdate(origtime) AS 'Created',
      @   owner AS 'By',
      @   subsystem AS 'Subsys',
      @   sdate(changetime) AS 'Changed',
      @   assignedto AS 'Assigned',
      @   severity AS 'Svr',
      @   priority AS 'Pri',
      @   title AS 'Title'
      @ FROM ticket
    ;
    zClrKey = 
      @ #ffffff Key:
      @ #f2dcdc Active
      @ #e8e8e8 Review
      @ #cfe8bd Fixed
      @ #bde5d6 Tested
      @ #cacae5 Deferred
      @ #c8c8c8 Closed
    ;
  }else{
    char **az = db_query("SELECT title, sqlcode, owner, cols "
                         "FROM reportfmt WHERE rn=%d",rn);
    if( az[0] ){
      zTitle = az[0];
      zSQL = az[1];
      zOwner = az[2];
      zClrKey = az[3];
    }
    if( P("copy") ){
      rn = 0;
      zTitle = mprintf("Copy Of %s", zTitle);
      zOwner = g.zUser;
    }
  }
  if( zOwner==0 ) zOwner = g.zUser;
  common_add_action_item("reportlist", "Cancel");
  if( rn>0 ){
    common_add_action_item( mprintf("rptedit?rn=%d&del1=1",rn), "Delete");
  }
  common_add_help_item("CvstracReport");
  common_header(rn>0 ? "Edit Report Format":"Create New Report Format");
  if( zErr ){
    @ <blockquote><font color="#ff0000"><b>%h(zErr)</b></font></blockquote>
  }
  @ <form action="rptedit" method="POST">
  @ <input type="hidden" name="rn" value="%d(rn)">
  @ <p>Report Title:<br>
  @ <input type="text" name="t" value="%h(zTitle)" size="60"></p>
  @ <p>Enter a complete SQL query statement against the "TICKET" table:<br>
  @ <textarea name="s" cols="70" rows="20">%h(zSQL)</textarea></p>
  if( g.okAdmin ){
    char **azUsers;
    azUsers = db_query("SELECT id FROM user UNION SELECT '' ORDER BY id");
    @ <p>Report owner:
    cgi_v_optionmenu(0, "w", zOwner, (const char**)azUsers);
    @ </p>
  } else {
    @ <input type="hidden" name="w" value="%h(zOwner)">
  }
  @ <p>Enter an optional color key in the following box.  (If blank, no
  @ color key is displayed.)  Each line contains the text for a single
  @ entry in the key.  The first token of each line is the background
  @ color for that line.<br>
  @ <textarea name="k" cols="50" rows="6">%h(zClrKey)</textarea></p>
  if( !g.okAdmin && strcmp(zOwner,g.zUser)!=0 ){
    @ <p>This report format is owned by %h(zOwner).  You are not allowed
    @ to change it.</p>
    @ </form>
    report_format_hints();
    common_footer();
    return;
  }
  @ <input type="submit" value="Apply Changes">
  if( rn>0 ){
    @ <input type="submit" value="Delete This Report" name="del1">
  }
  @ </form>
  report_format_hints();
  common_footer();
}

/*
** Output a bunch of text that provides information about report
** formats
*/
static void report_format_hints(void){
  @ <hr><h3>TICKET Schema</h3>
  @ <blockquote><pre>
  @ CREATE TABLE ticket(
  @    tn integer primary key,  -- Unique tracking number for the ticket
  @    type text,               -- code, doc, todo, new, or event
  @    status text,             -- new, review, defer, active, fixed,
  @                             -- tested, or closed
  @    origtime int,            -- Time this ticket was first created
  @    changetime int,          -- Time of most recent change to this ticket
  @    derivedfrom int,         -- This ticket derived from another
  @    version text,            -- Version or build number
  @    assignedto text,         -- Whose job is it to deal with this ticket
  @    severity int,            -- How bad is the problem
  @    priority text,           -- When should the problem be fixed
  @    subsystem text,          -- What subsystem does this ticket refer to
  @    owner text,              -- Who originally wrote this ticket
  @    title text,              -- Title of this bug
  @    description text,        -- Description of the problem
  @    remarks text             -- How the problem was dealt with
  @ );
  @ </pre></blockquote>
  @ <h3>Notes</h3>
  @ <ul>
  @ <li><p>The SQL must consist of a single SELECT statement</p></li>
  @
  @ <li><p>If a column of the result set is named "#" then that column
  @ is assumed to hold a ticket number.  A hyperlink will be created from
  @ that column to a detailed view of the ticket.</p></li>
  @
  @ <li><p>If a column of the result set is named "bgcolor" then the content
  @ of that column determines the background color of the row.</p></li>
  @
  @ <li><p>Times in the TICKET table are expressed as seconds since 1970.
  @ Convert these values to human-friendly date formats using the
  @ <b>sdate()</b> and <b>ldate()</b> SQL functions.</p></li>
  @ 
  @ <li><p>The <b>now()</b> SQL function returns the current time and date
  @ in seconds since 1970.  The <b>user()</b> SQL function returns a string
  @ which is the login of the current user.</p></li>
  @
  @ <li><p>The first column whose name begins with underscore ("_") and all
  @ subsequent columns are shown on their own rows in the table.  This is
  @ useful for displaying the TICKET.DESCRIPTION and TICKET.REMARKS fields.
  @ </p></li>
  @
  @ <li><p>The <b>aux()</b> SQL function takes a parameter name as an argument
  @ and returns the value that the user enters in the resulting HTML form. A
  @ second optional parameter provides a default value for the field.</p></li>
  @
  @ <li><p>The <b>option()</b> SQL function takes a parameter name
  @ and a quoted SELECT statement as parameters. The query results are
  @ presented as an HTML dropdown menu and the function returns
  @ the currently selected value. Results may be a single value column or
  @ two <b>value,description</b> columns. The first row is the default.</p></li>
  @
  @ <li><p>The <b>cgi()</b> SQL function takes a parameter name as an argument
  @ and returns the value of a corresponding CGI query value. If the CGI
  @ parameter doesn't exist, an optional second argument will be returned
  @ instead.</p></li>
  @
  @ <li><p>If a column is wrapped by the <b>wiki()</b> SQL function, it will
  @ be rendered as wiki formatted content.</p></li>
  @
  @ <li><p>If a column is wrapped by the <b>tkt()</b> SQL function, it will
  @ be shown as a ticket number with a link to the appropriate page</p></li>
  @
  @ <li><p>If a column is wrapped by the <b>chng()</b> SQL function, it will
  @ be shown as a checkin number with a link to the appropriate page.</p></li>
  @
  @ <li><p>The <b>path()</b> SQL function can be used to extract complete filename
  @ from FILE table. For example:
  @ <pre>SELECT path(isdir, dir, base) AS 'filename' FROM file</pre>
  @ </p></li>
  @
  @ <li><p>The <b>dirname()</b> SQL function takes filename as only argument
  @ and extracts parent directory name from it.</p></li>
  @
  @ <li><p>The <b>basename()</b> SQL function takes filename as only argument
  @ and extracts basename from it.</p></li>
  @
  @ <li><p>The <b>search()</b> SQL function takes a keyword pattern and
  @ a search text. The function returns an integer score which is
  @ higher depending on how well the search went.</p></li>
  @
  @ <li><p>The query can join other tables in the database besides TICKET.
  @ </p></li>
  @ </ul>
  @
  @ <h3>Examples</h3>
  @ <p>In this example, the first column in the result set is named
  @ "bgcolor".  The value of this column is not displayed.  Instead, it
  @ selects the background color of each row based on the TICKET.STATUS
  @ field of the database.  The color key at the right shows the various
  @ color codes.</p>
  @ <table align="right" hspace=5 border=1 cellspacing=0 width=125>
  @ <tr bgcolor="#f2dcdc"><td align="center">new or active</td></tr>
  @ <tr bgcolor="#e8e8bd"><td align="center">review</td></tr>
  @ <tr bgcolor="#cfe8bd"><td align="center">fixed</td></tr>
  @ <tr bgcolor="#bde5d6"><td align="center">tested</td></tr>
  @ <tr bgcolor="#cacae5"><td align="center">defer</td></tr>
  @ <tr bgcolor="#c8c8c8"><td align="center">closed</td></tr>
  @ </table>
  @ <blockquote><pre>
  @ SELECT
  @   CASE WHEN status IN ('new','active') THEN '#f2dcdc'
  @        WHEN status='review' THEN '#e8e8bd'
  @        WHEN status='fixed' THEN '#cfe8bd'
  @        WHEN status='tested' THEN '#bde5d6'
  @        WHEN status='defer' THEN '#cacae5'
  @        ELSE '#c8c8c8' END as 'bgcolor',
  @   tn AS '#',
  @   type AS 'Type',
  @   status AS 'Status',
  @   sdate(origtime) AS 'Created',
  @   owner AS 'By',
  @   subsystem AS 'Subsys',
  @   sdate(changetime) AS 'Changed',
  @   assignedto AS 'Assigned',
  @   severity AS 'Svr',
  @   priority AS 'Pri',
  @   title AS 'Title'
  @ FROM ticket
  @ </pre></blockquote>
  @ <p>To base the background color on the TICKET.PRIORITY or
  @ TICKET.SEVERITY fields, substitute the following code for the
  @ first column of the query:</p>
  @ <table align="right" hspace=5 border=1 cellspacing=0 width=125>
  @ <tr bgcolor="#f2dcdc"><td align="center">1</td></tr>
  @ <tr bgcolor="#e8e8bd"><td align="center">2</td></tr>
  @ <tr bgcolor="#cfe8bd"><td align="center">3</td></tr>
  @ <tr bgcolor="#cacae5"><td align="center">4</td></tr>
  @ <tr bgcolor="#c8c8c8"><td align="center">5</td></tr>
  @ </table>
  @ <blockquote><pre>
  @ SELECT
  @   CASE priority WHEN 1 THEN '#f2dcdc'
  @        WHEN 2 THEN '#e8e8bd'
  @        WHEN 3 THEN '#cfe8bd'
  @        WHEN 4 THEN '#cacae5'
  @        ELSE '#c8c8c8' END as 'bgcolor',
  @ ...
  @ FROM ticket
  @ </pre></blockquote>
#if 0
  @ <p>You can, of course, substitute different colors if you choose.
  @ Here is a palette of suggested background colors:</p>
  @ <blockquote>
  @ <table border=1 cellspacing=0 width=300>
  @ <tr><td align="center" bgcolor="#ffbdbd">#ffbdbd</td>
  @     <td align="center" bgcolor="#f2dcdc">#f2dcdc</td></tr>
  @ <tr><td align="center" bgcolor="#ffffbd">#ffffbd</td>
  @     <td align="center" bgcolor="#e8e8bd">#e8e8bd</td></tr>
  @ <tr><td align="center" bgcolor="#c0ebc0">#c0ebc0</td>
  @     <td align="center" bgcolor="#cfe8bd">#cfe8bd</td></tr>
  @ <tr><td align="center" bgcolor="#c0c0f4">#c0c0f4</td>
  @     <td align="center" bgcolor="#d6d6e8">#d6d6e8</td></tr>
  @ <tr><td align="center" bgcolor="#d0b1ff">#d0b1ff</td>
  @     <td align="center" bgcolor="#d2c0db">#d2c0db</td></tr>
  @ <tr><td align="center" bgcolor="#bbbbbb">#bbbbbb</td>
  @     <td align="center" bgcolor="#d0d0d0">#d0d0d0</td></tr>
  @ </table>
  @ </blockquote>
#endif
  @ <p>To see the TICKET.DESCRIPTION and TICKET.REMARKS fields, include
  @ them as the last two columns of the result set and given them names
  @ that begin with an underscore.  Like this:</p>
  @ <blockquote><pre>
  @  SELECT
  @    tn AS '#',
  @    type AS 'Type',
  @    status AS 'Status',
  @    sdate(origtime) AS 'Created',
  @    owner AS 'By',
  @    subsystem AS 'Subsys',
  @    sdate(changetime) AS 'Changed',
  @    assignedto AS 'Assigned',
  @    severity AS 'Svr',
  @    priority AS 'Pri',
  @    title AS 'Title',
  @    description AS '_Description',   -- When the column name begins with '_'
  @    remarks AS '_Remarks'            -- the data is shown on a separate row.
  @  FROM ticket
  @ </pre></blockquote>
  @
  @ <p>Or, to see part of the description on the same row, use the
  @ <b>wiki()</b> function with some string manipulation. Using the
  @ <b>tkt()</b> function on the ticket number will also generate a linked
  @ field, but without the extra <i>edit</i> column:
  @ </p>
  @ <blockquote><pre>
  @  SELECT
  @    tkt(tn) AS '',
  @    title AS 'Title',
  @    wiki(substr(description,0,80)) AS 'Description'
  @  FROM ticket
  @ </pre></blockquote>
  @
}

/*********************************************************************/
static void output_report_field(const char *zData,int rn){
  const char *zWkey = wiki_key();
  const char *zTkey = tkt_key();
  const char *zCkey = chng_key();

  if( !strncmp(zData,zWkey,strlen(zWkey)) ){
    output_formatted(&zData[strlen(zWkey)],0);
  }else if( !strncmp(zData,zTkey,strlen(zTkey)) ){
    output_ticket(atoi(&zData[strlen(zTkey)]),rn);
  }else if( !strncmp(zData,zCkey,strlen(zCkey)) ){
    output_chng(atoi(&zData[strlen(zCkey)]));
  }else{
    @ %h(zData)
  }
}

static void column_header(int rn,const char *zCol, int nCol, int nSorted,
    const char *zDirection, const char *zExtra
){
  int set = (nCol==nSorted);
  int desc = !strcmp(zDirection,"DESC");

  /*
  ** Clicking same column header 3 times in a row resets any sorting.
  ** Note that we link to rptview, which means embedded reports will get
  ** sent to the actual report view page as soon as a user tries to do
  ** any sorting. I don't see that as a Bad Thing.
  */
  if(set && desc){
    @ <th bgcolor="%s(BG1)" class="bkgnd1">
    @   <a href="rptview?rn=%d(rn)%s(zExtra)">%h(zCol)</a></th>
  }else{
    if(set){
      @ <th bgcolor="%s(BG1)" class="bkgnd1"><a
    }else{
      @ <th><a
    }
    @ href="rptview?rn=%d(rn)&order_by=%d(nCol)&\
    @ order_dir=%s(desc?"ASC":"DESC")\
    @ %s(zExtra)">%h(zCol)</a></th>
  }
}

/*********************************************************************/
struct GenerateHTML {
  int rn;
  int nCount;
};

/*
** The callback function for db_query
*/
static int generate_html(
  void* pUser,     /* Pointer to output state */
  int nArg,        /* Number of columns in this result row */
  char **azArg,    /* Text of data in all columns */
  char **azName    /* Names of the columns */
){
  struct GenerateHTML* pState = (struct GenerateHTML*)pUser;
  int i;
  int tn;            /* Ticket number.  (value of column named '#') */
  int rn;            /* Report number */
  int ncol;          /* Number of columns in the table */
  int multirow;      /* True if multiple table rows per line of data */
  int newrowidx;     /* Index of first column that goes on a separate row */
  int iBg = -1;      /* Index of column that determines background color */
  char *zBg = 0;     /* Use this background color */
  char zPage[30];    /* Text version of the ticket number */

  /* Get the report number
  */
  rn = pState->rn;

  /* Figure out the number of columns, the column that determines background
  ** color, and whether or not this row of data is represented by multiple
  ** rows in the table.
  */
  ncol = 0;
  multirow = 0;
  newrowidx = -1;
  for(i=0; i<nArg; i++){
    if( azName[i][0]=='b' && strcmp(azName[i],"bgcolor")==0 ){
      zBg = azArg ? azArg[i] : 0;
      iBg = i;
      continue;
    }
    if( g.okWrite && azName[i][0]=='#' ){
      ncol++;
    }
    if( !multirow ){
      if( azName[i][0]=='_' ){
        multirow = 1;
        newrowidx = i;
      }else{
        ncol++;
      }
    }
  }

  /* The first time this routine is called, output a table header
  */
  if( pState->nCount==0 ){
    char zExtra[2000];
    int nField = atoi(PD("order_by","0"));
    const char* zDir = PD("order_dir","");
    zDir = !strcmp("ASC",zDir) ? "ASC" : "DESC";
    zExtra[0] = 0;

    if( g.nAux ){
      @ <tr>
      @ <td colspan=%d(ncol)><form action="rptview" method="GET">
      @ <input type="hidden" name="rn" value="%d(rn)">
      for(i=0; i<g.nAux; i++){
        const char *zN = g.azAuxName[i];
        const char *zP = g.azAuxParam[i];
        if( g.azAuxVal[i] && g.azAuxVal[i][0] ){
          appendf(zExtra,0,sizeof(zExtra),
                  "&%t=%t",g.azAuxParam[i],g.azAuxVal[i]);
        }
        if( g.azAuxOpt[i] ){
          @ %h(zN): 
          if( g.anAuxCols[i]==1 ) {
            cgi_v_optionmenu( 0, zP, g.azAuxVal[i], g.azAuxOpt[i] );
          }else if( g.anAuxCols[i]==2 ){
            cgi_v_optionmenu2( 0, zP, g.azAuxVal[i], g.azAuxOpt[i] );
          }
        }else{
          @ %h(zN): <input type="text" name="%h(zP)" value="%h(g.azAuxVal[i])">
        }
      }
      @ <input type="submit" value="Go">
      @ </form></td></tr> 
    }
    @ <tr>
    tn = -1;
    for(i=0; i<nArg; i++){
      char *zName = azName[i];
      if( i==iBg ) continue;
      if( newrowidx>=0 && i>=newrowidx ){
        if( g.okWrite && tn>=0 ){
          @ <th>&nbsp;</th>
          tn = -1;
        }
        if( zName[0]=='_' ) zName++;
        @ </tr><tr><th colspan=%d(ncol)>%h(zName)</th>
      }else{
        if( zName[0]=='#' ){
          tn = i;
        }
        /*
        ** This handles any sorting related stuff. Note that we don't
        ** bother trying to sort on the "wiki format" columns. I don't
        ** think it makes much sense, visually.
        */
        column_header(rn,azName[i],i+1,nField,zDir,zExtra);
      }
    }
    if( g.okWrite && tn>=0 ){
      @ <th>&nbsp;</th>
    }
    @ </tr>
  }
  if( azArg==0 ){
    @ <tr><td colspan="%d(ncol)">
    @ <i>No records match the report criteria</i>
    @ </td></tr>
    return 0;
  }
  ++pState->nCount;

  /* Output the separator above each entry in a table which has multiple lines
  ** per database entry.
  */
  if( newrowidx>=0 ){
    @ <tr><td colspan=%d(ncol)><font size=1>&nbsp;</font></td></tr>
  }

  /* Output the data for this entry from the database
  */
  if( zBg==0 ) zBg = "white";
  @ <tr bgcolor="%h(zBg)">
  tn = 0;
  zPage[0] = 0;
  for(i=0; i<nArg; i++){
    char *zData;
    if( i==iBg ) continue;
    zData = azArg[i];
    if( zData==0 ) zData = "";
    if( newrowidx>=0 && i>=newrowidx ){
      if( tn>0 && g.okWrite ){
        @ <td valign="top"><a href="tktedit?tn=%d(tn),%d(rn)">edit</a></td>
        tn = 0;
      }
      if( zData[0] ){
        @ </tr><tr bgcolor="%h(zBg)"><td colspan=%d(ncol)>
        output_formatted(zData, zPage[0] ? zPage : 0);
      }
    }else if( azName[i][0]=='#' ){
      tn = atoi(zData);
      if( tn>0 ) bprintf(zPage, sizeof(zPage), "%d", tn);
      @ <td valign="top"><a href="tktview?tn=%d(tn),%d(rn)">%h(zData)</a></td>
    }else if( zData[0]==0 ){
      @ <td valign="top">&nbsp;</td>
    }else{
      @ <td valign="top">
      output_report_field(zData,rn);
      @ </td>
    }
  }
  if( tn>0 && g.okWrite ){
    @ <td valign="top"><a href="tktedit?tn=%d(tn),%d(rn)">edit</a></td>
  }
  @ </tr>
  return 0;
}

/*
** Output the text given in the argument.  Convert tabs and newlines into
** spaces.
*/
static void output_no_tabs(const char *z){
  while( z && z[0] ){
    int i, j;
    for(i=0; z[i] && (!isspace(z[i]) || z[i]==' '); i++){}
    if( i>0 ){
      cgi_printf("%.*s", i, z);
    }
    for(j=i; isspace(z[j]); j++){}
    if( j>i ){
      cgi_printf("%*s", j-i, "");
    }
    z += j;
  }
}

/*
** Output a row as a tab-separated line of text.
*/
static int output_tab_separated(
  void *pUser,     /* Pointer to row-count integer */
  int nArg,        /* Number of columns in this result row */
  char **azArg,    /* Text of data in all columns */
  char **azName    /* Names of the columns */
){
  int *pCount = (int*)pUser;
  int i;

  if( *pCount==0 ){
    for(i=0; i<nArg; i++){
      output_no_tabs(azName[i]);
      cgi_printf("%c", i<nArg-1 ? '\t' : '\n');
    }
  }
  ++*pCount;
  for(i=0; i<nArg; i++){
    output_no_tabs(azArg[i]);
    cgi_printf("%c", i<nArg-1 ? '\t' : '\n');
  }
  return 0;
}

/*
** Generate HTML that describes a color key.
*/
void output_color_key(const char *zClrKey, int horiz, char *zTabArgs){
  int i, j, k;
  char *zSafeKey, *zToFree;
  while( isspace(*zClrKey) ) zClrKey++;
  if( zClrKey[0]==0 ) return;
  @ <table %s(zTabArgs)>
  if( horiz ){
    @ <tr>
  }
  zToFree = zSafeKey = mprintf("%h", zClrKey);
  while( zSafeKey[0] ){
    while( isspace(*zSafeKey) ) zSafeKey++;
    for(i=0; zSafeKey[i] && !isspace(zSafeKey[i]); i++){}
    for(j=i; isspace(zSafeKey[j]); j++){}
    for(k=j; zSafeKey[k] && zSafeKey[k]!='\n' && zSafeKey[k]!='\r'; k++){}
    if( !horiz ){
      cgi_printf("<tr bgcolor=\"%.*s\"><td>%.*s</td></tr>\n",
        i, zSafeKey, k-j, &zSafeKey[j]);
    }else{
      cgi_printf("<td bgcolor=\"%.*s\">%.*s</td>\n",
        i, zSafeKey, k-j, &zSafeKey[j]);
    }
    zSafeKey += k;
  }
  free(zToFree);
  if( horiz ){
    @ </tr>
  }
  @ </table>
}

/*
** Outputs a report, rn.
**
** zTableOpts may be used to control things like table alignment or width. It
** goes in the HTML <TABLE> tag.
*/
void embed_view(int rn, const char *zCaption, const char *zTableOpts){
  char **az;
  char *zSql;
  const char *zTitle;
  char *zClrKey;
  static int nDepth = 0;
  struct GenerateHTML sState;

  /* report fields can be wiki formatted. Let's not get into infinite
  ** recursions...
  */
  if(nDepth) return;

  db_add_functions();
  view_add_functions(0);

  az = db_query( "SELECT title, sqlcode, cols FROM reportfmt WHERE rn=%d", rn);
  if( az[0]==0 ) return;
  nDepth++;
  zTitle = az[0];
  if( zCaption==0 || zCaption[0]==0 ){
    zCaption = zTitle;
  }
  zSql = az[1];
  zClrKey = az[3];
  db_execute("PRAGMA empty_result_callbacks=ON");
  /* output_color_key(zClrKey, 1, "border=0 cellpadding=3 cellspacing=0"); */
  @ <div>
  @ <table border=1 cellpadding=2 cellspacing=0
  @        summary="%h(zTitle)" %s(zTableOpts?zTableOpts:"")>
  @ <caption>
  @   <a href="rptview?rn=%d(rn)" rel="nofollow"
  @      title="%h(zTitle)">%h(zCaption)</a>
  @ </caption>
  db_restrict_access(1);
  sState.rn = rn;
  sState.nCount = 0;
  db_callback_query(generate_html, &sState, "%s", zSql);
  db_restrict_access(0);
  @ </table></div>
  nDepth--;
}

/*
** Adds all appropriate action bar links for report tools
*/
static void add_rpt_tools( const char *zExcept, int rn ){
  int i;
  char *zLink;
  char **azTools;
  db_add_functions();
  azTools = db_query("SELECT tool.name FROM tool,user "
                     "WHERE tool.object='rpt' AND user.id='%q' "
                     "      AND cap_and(tool.perms,user.capabilities)!=''",
                     g.zUser);

  for(i=0; azTools[i]; i++){
    if( zExcept && 0==strcmp(zExcept,azTools[i]) ) continue;
    zLink = mprintf("rptrool?t=%T&rn=%d", azTools[i], rn);
    common_add_action_item(zLink, azTools[i]);
  }
}

/*
** WEBPAGE: /rpttool
**
** Execute an external tool on a given ticket
*/
void rpttool(void){
  int rn = atoi(PD("rn","0"));
  const char *zTool = P("t");
  char *zAction;
  const char *azSubst[32];
  int n = 0;

  if( rn==0 || zTool==0 ) cgi_redirect("index");

  login_check_credentials();
  if( !g.okRead ){ login_needed(); return; }
  throttle(1,0);
  history_update(0);

  zAction = db_short_query("SELECT command FROM tool "
                           "WHERE name='%q' AND object='rpt'", zTool);
  if( zAction==0 || zAction[0]==0 ){
    cgi_redirect(mprintf("rptview?rn=%d",rn));
  }

  common_standard_menu(0, 0);
  common_add_action_item(mprintf("rptview?rn=%d", rn), "View");
  common_add_action_item( mprintf("rptsql?rn=%d",rn), "SQL");
  add_rpt_tools(zTool,rn);

  common_header("%h (%d)", zTool, rn);

  azSubst[n++] = "RN";
  azSubst[n++] = mprintf("%d",rn);
  azSubst[n++] = 0;

  n = execute_tool(zTool,zAction,0,azSubst);
  free(zAction);
  if( n<=0 ){
    cgi_redirect(mprintf("rptview?rn=%d", rn));
  }
  common_footer();
}

/*
** WEBPAGE: /rptview
**
** Generate a report.  The rn query parameter is the report number
** corresponding to REPORTFMT.RN.  If the tablist query parameter exists,
** then the output consists of lines of tab-separated fields instead of
** an HTML table.
*/
void view_view(void){
  int count = 0;
  int rn;
  char **az;
  char *zSql;
  char *zTitle;
  char *zOwner;
  char *zClrKey;
  int tabs;

  login_check_credentials();
  if( !g.okRead ){ login_needed(); return; }
  throttle(1,0);
  rn = atoi(PD("rn","0"));
  if( rn==0 ){
    cgi_redirect("reportlist");
    return;
  }
  tabs = P("tablist")!=0;
  db_add_functions();
  view_add_functions(tabs);
  az = db_query(
    "SELECT title, sqlcode, owner, cols FROM reportfmt WHERE rn=%d", rn);
  if( az[0]==0 ){
    cgi_redirect("reportlist");
    return;
  }
  zTitle = az[0];
  zSql = az[1];

  if( P("order_by") ){
    /*
    ** If the user wants to do a column sort, wrap the query into a sub
    ** query and then sort the results. This is a whole lot easier than
    ** trying to insert an ORDER BY into the query itself, especially
    ** if the query is already ordered.
    */
    int nField = atoi(P("order_by"));
    if( nField > 0 ){
      const char* zDir = PD("order_dir","");
      zDir = !strcmp("ASC",zDir) ? "ASC" : "DESC";
      zSql = mprintf("SELECT * FROM (%s) ORDER BY %d %s", zSql, nField, zDir);
    }
  }

  zOwner = az[2];
  zClrKey = az[3];
  count = 0;
  if( !tabs ){
    struct GenerateHTML sState;

    db_execute("PRAGMA empty_result_callbacks=ON");
    common_standard_menu("rptview", 0);
    common_add_help_item("CvstracReport");
    common_add_action_item(
      mprintf("rptview?tablist=1&%s", getenv("QUERY_STRING")),
      "Raw Data"
    );
    if( g.okAdmin || (g.okQuery && strcmp(g.zUser,zOwner)==0) ){
      common_add_action_item( mprintf("rptedit?rn=%d",rn), "Edit");
    }
    common_add_action_item( mprintf("rptsql?rn=%d",rn), "SQL");
    common_header("%s", zTitle);
    output_color_key(zClrKey, 1, "border=0 cellpadding=3 cellspacing=0");
    @ <table border=1 cellpadding=2 cellspacing=0>
    db_restrict_access(1);
    sState.rn = rn;
    sState.nCount = 0;
    db_callback_query(generate_html, &sState, "%s", zSql);
    db_restrict_access(0);
    @ </table>
  }else{
    db_restrict_access(1);
    db_callback_query(output_tab_separated, &count, "%s", zSql);
    db_restrict_access(0);
    cgi_set_content_type("text/plain");
  }
  if( !tabs ){
    common_footer();
  }
}

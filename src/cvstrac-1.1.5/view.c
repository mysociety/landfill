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
static void report_format_hints();

/*
** WEBPAGE: /reportlist
*/
void view_list(void){
  char **az;
  int i;

  login_check_credentials();
  if( !g.okRead ){ login_needed(); return; }
  throttle(1);
  common_standard_menu("reportlist", "search?t=1");
  common_add_menu_item("rptnew", "New Report Format");
  common_header("Available Report Formats");
  az = db_query("SELECT rn, title, owner FROM reportfmt ORDER BY title");
  @ <p>Choose a report format from the following list:</p>
  @ <ol>
  for(i=0; az[i]; i+=3){
    @ <li><a href="rptview?rn=%s(az[i])">%h(az[i+1])</a>&nbsp;&nbsp;&nbsp;
    if( g.okWrite && az[i+2] && az[i+2][0] ){
      @ (by <i>%h(az[i+2])</i>)
    }
    if( g.okWrite ){
      @ [<a href="rptedit?rn=%s(az[i])&amp;copy=1">copy</a>]
    }
    if( g.okAdmin || (g.okWrite && strcmp(g.zUser,az[i+2])==0) ){
      @ [<a href="rptedit?rn=%s(az[i])">edit</a>]
    }
    @ [<a href="rptsql?rn=%s(az[i])">sql</a>]
    @ </li>
  }
  if( g.okWrite ){
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
** Remove blank lines from the beginning of a string and
** all whitespace from the end.  Convert any CRNL sequence into
** a single NL.
*/
char *remove_blank_lines(const char *zOrig){
  int i, j, n;
  char *z;
  for(i=j=0; isspace(zOrig[i]); i++){ if( zOrig[i]=='\n' ) j = i+1; }
  n = strlen(&zOrig[j]);
  while( n>0 && isspace(zOrig[j+n-1]) ){ n--; }
  z = mprintf("%.*s", n, &zOrig[j]);
  for(i=j=0; z[i]; i++){
    if( z[i]=='\r' && z[i+1]=='\n' ) continue;
    z[j++] = z[i];
  }
  z[j] = 0;
  return z;
}

/*
** Check the given SQL to see if is a valid query that does not
** attempt to do anything dangerous.  Return 0 on success and a
** pointer to an error message string (obtained from malloc) if
** there is a problem.
*/
static char *verify_sql_statement(char *zSql){
  int i;
  extern int sqliteStrNICmp(const char*,const char*,int);
  extern int sqliteStrICmp(const char*,const char*);

  /* First make sure the SQL is a single query command by verifying that
  ** the first token is "SELECT" and that there are no unquoted semicolons.
  */
  for(i=0; isspace(zSql[i]); i++){}
  if( sqliteStrNICmp(&zSql[i],"select",6)!=0 ){
    return mprintf("The SQL must be a SELECT statement");
  }
  for(i=0; zSql[i]; i++){
    if( zSql[i]==';' ){
      int bad;
      int c = zSql[i+1];
      zSql[i+1] = 0;
      bad = sqlite_complete(zSql);
      zSql[i+1] = c;
      if( bad ){
        return mprintf("Only a single SQL statement is allowed");
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
  throttle(1);
  rn = atoi(PD("rn","0"));
  az = db_query("SELECT title, sqlcode, owner, cols "
                "FROM reportfmt WHERE rn=%d",rn);
  common_standard_menu(0, 0);
  common_add_menu_item( mprintf("rptview?rn=%d",rn), "View");
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
  if( !g.okWrite ){
    login_needed();
    return;
  }
  throttle(1);
  rn = atoi(PD("rn","0"));
  zTitle = P("t");
  zOwner = PD("w",g.zUser);
  z = P("s");
  zSQL = z ? trim_string(z) : 0;
  zClrKey = trim_string(PD("k",""));
  if( rn>0 && P("del") ){
    db_execute("DELETE FROM reportfmt WHERE rn=%d", rn);
    cgi_redirect("reportlist");
    return;
  }
  if( zTitle && zSQL ){
    if( zSQL[0]==0 ){
      zErr = "Please supply an SQL query statement";
    }else if( (zTitle = trim_string(zTitle))[0]==0 ){
      zErr = "Please supply a title"; 
    }else{
      zErr = verify_sql_statement(zSQL);
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
      }
      cgi_redirect("reportlist");
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
  common_add_menu_item("reportlist", "Cancel");
  if( rn>0 ){
    common_add_menu_item( mprintf("rptedit?rn=%d",rn), "Delete");
  }
  common_header(rn>0 ? "Edit Report Format":"Create New Report Format");
  if( zErr ){
    @ <blockquote><font color="#ff0000"><b>%s(zErr)</b></font></blockquote>
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
    @ <input type="submit" value="Delete This Format" name="del">
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
  @ <b>sdata()</b> and <b>ldate()</b> SQL functions.</p></li>
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
  @    rn AS '#',
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
}

/*
** The callback function for db_query
*/
static int generate_html(
  void *pUser,     /* Pointer to row-count integer */
  int nArg,        /* Number of columns in this result row */
  char **azArg,    /* Text of data in all columns */
  char **azName    /* Names of the columns */
){
  int *pCount = (int*)pUser;
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
  rn = atoi(PD("rn","0"));

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
  if( *pCount==0 ){
    if( g.nAux ){
      @ <tr>
      @ <td colspan=%d(ncol)><form action="rptview" method="GET">
      @ <input type="hidden" name="rn" value="%d(rn)">
      for(i=0; i<g.nAux; i++){
        const char *zN = g.azAuxName[i];
        @ %h(zN): <input type="text" name="%h(zN)" value="%h(g.azAuxVal[i])">
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
        @ <th>%h(azName[i])</th>
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
  ++*pCount;

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
      if( tn>0 ) sprintf(zPage, "%d", tn);
      @ <td valign="top"><a href="tktview?tn=%d(tn),%d(rn)">%h(zData)</a></td>
    }else if( zData[0]==0 ){
      @ <td valign="top">&nbsp;</td>
    }else{
      @ <td valign="top">%h(zData)</td>
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
  while( isspace(*zClrKey) ) zClrKey++;
  if( zClrKey[0]==0 ) return;
  @ <table %s(zTabArgs)>
  if( horiz ){
    @ <tr>
  }
  while( zClrKey[0] ){
    while( isspace(*zClrKey) ) zClrKey++;
    for(i=0; zClrKey[i] && !isspace(zClrKey[i]); i++){}
    for(j=i; isspace(zClrKey[j]); j++){}
    for(k=j; zClrKey[k] && zClrKey[k]!='\n' && zClrKey[k]!='\r'; k++){}
    if( !horiz ){
      cgi_printf("<tr bgcolor=\"%.*s\"><td>%.*h</td></tr>\n",
        i, zClrKey, k-j, &zClrKey[j]);
    }else{
      cgi_printf("<td bgcolor=\"%.*s\">%.*h</td>\n",
        i, zClrKey, k-j, &zClrKey[j]);
    }
    zClrKey += k;
  }
  if( horiz ){
    @ </tr>
  }
  @ </table>
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
  int count;
  int rn;
  char **az;
  char *zSql;
  char *zTitle;
  char *zOwner;
  char *zClrKey;
  int tabs;

  login_check_credentials();
  if( !g.okRead ){ login_needed(); return; }
  throttle(1);
  rn = atoi(PD("rn","0"));
  if( rn==0 ){
    cgi_redirect("reportlist");
    return;
  }
  tabs = P("tablist")!=0;
  db_add_functions();
  az = db_query(
    "SELECT title, sqlcode, owner, cols FROM reportfmt WHERE rn=%d", rn);
  if( az[0]==0 ){
    cgi_redirect("reportlist");
    return;
  }
  zTitle = az[0];
  zSql = az[1];
  zOwner = az[2];
  zClrKey = az[3];
  count = 0;
  if( !tabs ){
    db_execute("PRAGMA empty_result_callbacks=ON");
    common_standard_menu("rptview", 0);
    common_add_menu_item(
      mprintf("rptview?tablist=1&%s", getenv("QUERY_STRING")),
      "Raw Data"
    );
    if( g.okAdmin || (g.okWrite && strcmp(g.zUser,zOwner)==0) ){
      common_add_menu_item( mprintf("rptedit?rn=%d",rn), "Edit");
    }
    common_add_menu_item( mprintf("rptsql?rn=%d",rn), "SQL");
    common_header("%s", zTitle);
    output_color_key(zClrKey, 1, "border=0 cellpadding=3 cellspacing=0");
    @ <table border=1 cellpadding=2 cellspacing=0>
    db_restrict_access(1);
    db_callback_query(generate_html, &count, "%s", zSql);
    db_restrict_access(0);
    @ </table>
  }else{
    db_callback_query(output_tab_separated, &count, "%s", zSql);
    cgi_set_content_type("text/plain");
  }
  if( !tabs ){
    common_footer();
  }
}

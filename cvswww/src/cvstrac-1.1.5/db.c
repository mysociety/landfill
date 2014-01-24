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
** This file contains routines used to interact with the database.
*/
#define _XOPEN_SOURCE
#include <unistd.h>
#include <time.h>
#include <sqlite.h>
#include "config.h"
#include "db.h"

/*
** This function is not an official part of the SQLite API and does
** not appear in <sqlite.h>.  So we have to supply our own prototype.
*/
extern char *sqlite_vmprintf(const char *, va_list);

/*
** The following is the handle to the open database.
*/
static sqlite *pDb = 0;

/*
** Open the database.
** Exit with an error message if the database will not open.
*/
sqlite *db_open(void){
  char *zName;
  char *zErrMsg;

  if( pDb ) return pDb;
  zName = mprintf("%s.db", g.zName);
  pDb = sqlite_open(zName, 0666, &zErrMsg);
  if( zErrMsg ){
    cgi_reset_content();
    cgi_set_status(200, "OK");
    cgi_set_content_type("text/html");
    @ <h1>Can't open database</h1>
    @ <p>Unable to open the database named "<b>%h(zName)</b>".
    @ Reason: %h(zErrMsg)</p>
    cgi_reply();
    exit(0);
  }
  sqlite_busy_timeout(pDb, 10000);
  free(zName);
  return pDb;
}

/*
** Close the database
*/
void db_close(void){
  if( pDb ){
    sqlite_close(pDb);
    pDb = 0;
  }
}

/*
** The SQL authorizer function for the user-supplies queries.  This
** routine NULLs-out fields of the database we do not want arbitrary
** users to see, such as the USER.PASSWD field.
*/
static int authorizer(
  void *NotUsed,
  int type,
  const char *zArg1,
  const char *zArg2,
  const char *zArg3,
  const char *zArg4
){
  extern int sqliteStrICmp(const char*, const char*);
  if( type==SQLITE_SELECT ){
    return SQLITE_OK;
  }else if( type==SQLITE_READ ){
    if( sqliteStrICmp(zArg1,"user")==0 ){
      if( sqliteStrICmp(zArg2,"passwd")==0 || sqliteStrICmp(zArg2,"email")==0 ){
        return SQLITE_IGNORE;
      }
    }else if( sqliteStrICmp(zArg1, "cookie")==0 ){
      return SQLITE_IGNORE;
    }
    return SQLITE_OK;
  }else{
    return SQLITE_DENY;
  }
}

/*
** Restrict access to sensitive information in the database.
*/
void db_restrict_access(int onoff){
  sqlite_set_authorizer(pDb, onoff ? authorizer : 0, 0);
}

/*
** Used to accumulate query results by db_query()
*/
struct QueryResult {
  int nElem;       /* Number of used entries in azElem[] */
  int nAlloc;      /* Number of slots allocated for azElem[] */
  char **azElem;   /* The result of the query */
};

/*
** The callback function for db_query
*/
static int db_query_callback(
  void *pUser,     /* Pointer to the QueryResult structure */
  int nArg,        /* Number of columns in this result row */
  char **azArg,    /* Text of data in all columns */
  char **NotUsed   /* Names of the columns */
){
  struct QueryResult *pResult = (struct QueryResult*)pUser;
  int i;
  if( pResult->nElem + nArg >= pResult->nAlloc ){
    if( pResult->nAlloc==0 ){
      pResult->nAlloc = nArg+1;
    }else{
      pResult->nAlloc = pResult->nAlloc*2 + nArg + 1;
    }
    pResult->azElem = realloc( pResult->azElem, pResult->nAlloc*sizeof(char*));
    if( pResult->azElem==0 ){
      exit(1);
    }
  }
  if( azArg==0 ) return 0;
  for(i=0; i<nArg; i++){
    pResult->azElem[pResult->nElem++] = mprintf("%s",azArg[i] ? azArg[i] : ""); 
  }
  return 0;
}

/*
** Execute a query against the database.  Return the
** results as a list of pointers to strings.  NULL values are returned
** as an empty string.  The list is terminated by a single NULL pointer.
**
** If anything goes wrong, an error page is generated and the program
** aborts.  If this routine will only return to its calling procedure
** if the query contained no errors.
*/
char **db_query(const char *zFormat, ...){
  int rc;
  char *zErrMsg = 0;
  va_list ap;
  struct QueryResult sResult;

  if( pDb==0 ) db_open();
  memset(&sResult, 0, sizeof(sResult));
  va_start(ap, zFormat);
  rc = sqlite_exec_vprintf(pDb, zFormat, db_query_callback,
                           &sResult, &zErrMsg, ap);
  va_end(ap);
  if( zErrMsg ){
    char *zSql = sqlite_vmprintf(zFormat, ap);
    cgi_reset_content();
    cgi_set_status(200, "OK");
    cgi_set_content_type("text/html");
    @ <h1>Query failed</h1>
    @ <p>Database query failed:
    @ <blockquote>%h(zSql)</blockquote>
    @ Reason: %h(zErrMsg)</p>
    cgi_reply();
    db_close();
    exit(0);
  }
  if( sResult.azElem==0 ){
    db_query_callback(&sResult, 0, 0, 0);
  }
  sResult.azElem[sResult.nElem] = 0;
  return sResult.azElem;
}

/*
** The callback function for db_short_query.
**
** Save the first argument azArg[0] in to memory obtained from malloc(),
** make *pUser point to that memory, then return 1 to abort the
** query.
*/
static int db_short_query_callback(
  void *pUser,     /* Pointer to the QueryResult structure */
  int nArg,        /* Number of columns in this result row */
  char **azArg,    /* Text of data in all columns */
  char **NotUsed   /* Names of the columns */
){
  char **pzResult = (char**)pUser;
  if( nArg>0 && azArg ){
    if( *pzResult ) free(*pzResult);
    *pzResult = mprintf("%s", azArg[0]);
  }
  return 1;
}

/*
** Execute a query against the database.  Return a pointer to a single
** string which is the first result of that query.  Errors in the query
** are ignored.
**
** This routine is designed for use on queries that only return a 
** single value.  For multi-valued results, use db_query().
*/
char *db_short_query(const char *zFormat, ...){
  va_list ap;
  char *zResult = 0;

  if( pDb==0 ) db_open();
  va_start(ap, zFormat);
  sqlite_exec_vprintf(pDb, zFormat, db_short_query_callback, &zResult, 0, ap);
  va_end(ap);
  return zResult;
}

/*
** Execute an SQL statement against either the database.
** Print an error and abort if something goes wrong.
*/
void db_execute(const char *zFormat, ...){
  int rc;
  char *zErrMsg = 0;
  va_list ap;

  if( pDb==0 ) db_open();
  va_start(ap, zFormat);
  rc = sqlite_exec_vprintf(pDb, zFormat, 0, 0, &zErrMsg, ap);
  va_end(ap);
  if( zErrMsg ){
    char *zSql = sqlite_vmprintf(zFormat, ap);
    cgi_reset_content();
    cgi_set_status(200, "OK");
    cgi_set_content_type("text/html");
    @ <h1>Query failed</h1>
    @ <p>Database query failed:
    @ <blockquote>%h(zSql)</blockquote>
    @ Reason: %h(zErrMsg)</p>
    cgi_reply();
    db_close();
    exit(0);
  }
}

/*
** Free the results of a db_query() call.
*/
void db_query_free(char **az){
  int i;
  for(i=0; az[i]; i++){
    free(az[i]);
  }
  free(az);
}

/*
** The operation of querying the CONFIG table for a CONFIG.VALUE that
** corresponds to a particular CONFIG.NAME is so common, that we give it
** its own subroutine.
**
** The entire contents of the CONFIG table are cached.  If you write
** to the CONFIG table to change a value, this routine will not know
** about it and will return the old value.  You can clear the cache
** by calling this routine will a NULL zName parameter.
*/
const char *db_config(const char *zName, const char *zDefault){
  static char **azCache = 0;
  int i;
  if( azCache==0 && zName!=0 ){
    azCache = db_query("SELECT name, value FROM config");
  }
  if( zName==0 ){
    azCache = 0;
    return zDefault;
  }
  for(i=0; azCache[i]; i+=2){
    if( azCache[i][0]==zName[0] && strcmp(azCache[i],zName)==0 ){
      return azCache[i+1];
    }
  }
  return zDefault;
}

/*
** Execute a query against the database and invoke the
** given callback for each row.
*/
void db_callback_query(
  int (*xCallback)(void*,int,char**,char**),  /* Callback for each row */
  void *pArg,                                 /* 1st argument to callback */
  const char *zFormat,                        /* SQL for the query */
  ...                                         /* Arguments for the SQL */
){
  int rc;
  char *zErrMsg = 0;
  va_list ap;

  if( pDb==0 ) db_open();
  va_start(ap, zFormat);
  rc = sqlite_exec_vprintf(pDb, zFormat, xCallback, pArg, &zErrMsg, ap);
  va_end(ap);
  if( zErrMsg ){
    char *zSql = sqlite_vmprintf(zFormat, ap);
    cgi_reset_content();
    cgi_set_status(200, "OK");
    cgi_set_content_type("text/html");
    @ <h1>Query failed</h1>
    @ <p>Database query failed:
    @ <blockquote>%h(zSql)</blockquote>
    @ Reason: %h(zErrMsg)</p>
    cgi_reply();
    db_close();
    exit(0);
  }
}

/*
** Implement the sdate() SQL function.  sdate() converts an integer
** which is the number of seconds since 1970 into a short date or
** time description.  For recent dates (within the past 24 hours)
** just the time is shown.  (ex: 14:23)  For dates within the past
** year, the month and day are shown.  (ex: Apr09).  For dates more
** than a year old, only the year is shown.
*/
static void sdate(sqlite_func *context, int argc, const char **argv){
  time_t now;
  time_t t;
  struct tm *pTm;
  char *zFormat;
  char zBuf[200];
  
  if( argc!=1 || argv[0]==0 ) return;
  time(&now);
  t = atoi(argv[0]);
  if( t+8*3600 > now && t-8*3600 <= now ){
    zFormat = "%H:%M";
  }else if( t+24*3600*120 > now && t-24*3600*120 < now ){
    zFormat = "%b %d";
  }else{
    zFormat = "%Y %b";
  }
  pTm = localtime(&t);
  strftime(zBuf, sizeof(zBuf), zFormat, pTm);
  sqlite_set_result_string(context, zBuf, -1);
}

/*
** Implement the ldate() SQL function.  ldate() converts an integer
** which is the number of seconds since 1970 into a full date and
** time description.
*/
static void ldate(sqlite_func *context, int argc, const char **argv){
  time_t t;
  struct tm *pTm;
  char zBuf[200];
  
  if( argc!=1 ) return;
  t = atoi(argv[0]);
  pTm = localtime(&t);
  strftime(zBuf, sizeof(zBuf), "%Y-%b-%d %H:%M", pTm);
  sqlite_set_result_string(context, zBuf, -1);
}

/*
** Implement the parsedate() SQL function.  parsedate() converts an
** ISO8601 date/time string into the number of seconds since 1970.
*/
static void pdate(sqlite_func *context, int argc, const char **argv){
  time_t t;
  
  if( argc!=1 ) return;
  t = argv[0] ? parse_time(argv[0]) : 0;
  sqlite_set_result_int(context, t);
}

/*
** Implement the now() SQL function.  now() takes no arguments and
** returns the current time in seconds since 1970.
*/
static void f_now(sqlite_func *context, int argc, const char **argv){
  time_t now;
  time(&now);
  sqlite_set_result_int(context, now);
}

/*
** Implement the user() SQL function.  user() takes no arguments and
** returns the user ID of the current user.
*/
static void f_user(sqlite_func *context, int argc, const char **argv){
  if( g.zUser!=0 ) sqlite_set_result_string(context, g.zUser, -1);
}

/*
** Implement the aux() SQL function.  aux() takes a single argument which
** is a parameter name.  It then returns the value of that parameter.  The
** user is able to enter the parameter on a form.
*/
static void f_aux(sqlite_func *context, int argc, const char **argv){
  int i;
  extern int sqliteStrICmp(const char*, const char*);
  extern char *sqlite_mprintf(const char*,...);
  if( argc<1 ) return;
  for(i=0; i<g.nAux; i++){
    if( sqliteStrICmp(argv[0],g.azAuxName[i])==0 ){
      sqlite_set_result_string(context, g.azAuxVal[i], -1);
      return;
    }
  }
  if( g.nAux<MX_AUX ){
    g.azAuxName[g.nAux] = sqlite_mprintf("%s",argv[0]);
    g.azAuxVal[g.nAux] = sqlite_mprintf("%s",PD(argv[0],argv[1]));
    sqlite_set_result_string(context, g.azAuxVal[g.nAux], -1);
    g.nAux++;
  }
}

/*
** The two arguments are capability strings: strings containing lower
** case letters.  Return a string that contains only those letters found
** in both arguments.
**
** Example:  cap_and("abcd","cdef") returns "cd".
*/
static void cap_and(sqlite_func *context, int argc, const char **argv){
  int i, j;
  const char *z;
  char set[26];
  char zResult[27];
  if( argc!=2 ) return;
  for(i=0; i<26; i++) set[i] = 0;
  z = argv[0];
  if( z ){
    for(i=0; z[i]; i++){
      if( z[i]>='a' && z[i]<='z' ) set[z[i]-'a'] = 1;
    }
  }
  z = argv[1];
  if( z ){
    for(i=0; z[i]; i++){
      if( z[i]>='a' && z[i]<='z' && set[z[i]-'a']==1 ) set[z[i]-'a'] = 2;
    }
  }
  for(i=j=0; i<26; i++){
    if( set[i]==2 ) zResult[j++] = i+'a';
  }
  zResult[j] = 0;
  sqlite_set_result_string(context, zResult, -1);
}

/*
** The two arguments are capability strings: strings containing lower
** case letters.  Return a string that contains those letters found
** in either arguments.
**
** Example:  cap_and("abcd","cdef") returns "abcdef".
*/
static void cap_or(sqlite_func *context, int argc, const char **argv){
  int i, j;
  const char *z;
  char set[26];
  char zResult[27];
  if( argc!=2 ) return;
  for(i=0; i<26; i++) set[i] = 0;
  z = argv[0];
  if( z ){
    for(i=0; z[i]; i++){
      if( z[i]>='a' && z[i]<='z' ) set[z[i]-'a'] = 1;
    }
  }
  z = argv[1];
  if( z ){
    for(i=0; z[i]; i++){
      if( z[i]>='a' && z[i]<='z' ) set[z[i]-'a'] = 1;
    }
  }
  for(i=j=0; i<26; i++){
    if( set[i] ) zResult[j++] = i+'a';
  }
  zResult[j] = 0;
  sqlite_set_result_string(context, zResult, -1);
}

/*
** This routine adds the extra SQL functions to the SQL engine.
*/
void db_add_functions(void){
  sqlite *pDb = db_open();
  sqlite_create_function(pDb, "sdate", 1, &sdate, 0);
  sqlite_create_function(pDb, "ldate", 1, &ldate, 0);
  sqlite_create_function(pDb, "parsedate", 1, &pdate, 0);
  sqlite_create_function(pDb, "now", 0, &f_now, 0);
  sqlite_create_function(pDb, "user", 0, &f_user, 0);
  sqlite_create_function(pDb, "aux", 1, &f_aux, 0);
  sqlite_create_function(pDb, "aux", 2, &f_aux, 0);
  sqlite_create_function(pDb, "cap_or", 2, &cap_or, 0);
  sqlite_create_function(pDb, "cap_and", 2, &cap_and, 0);
}

/*
** The (original version 1.0) database schema is defined by the following SQL.
** Changes are made for subsequent versions.  See the zSchemaChange...
** strings defined later on in this file for the details on the changes.
*/
static char zSchema[] =
@ BEGIN TRANSACTION;
@
@ -- Each "cvs commit" results in a single entry in the following table.
@ -- Even commits that involve multiple files generate just a single
@ -- table entry.
@ --
@ CREATE TABLE chng(
@    cn integer primary key,   -- A unique "change" number
@    date int,                 -- Time commit occured in seconds since 1970
@    branch text,              -- Name of branch or '' if main trunk
@    milestone int,            -- 0: not a milestone. 1: release, 2: event
@    user text,                -- User who did the commit
@    message text              -- Text of the log message
@ );
@ CREATE INDEX chng_idx ON chng(user,message);
@
@ -- Each file involved in a commit operation results in an entry in
@ -- this table.
@ --
@ CREATE TABLE filechng(
@    cn int,                  -- Corresponds to CHNG.CN field
@    filename text,           -- Name of the file
@    vers text,               -- New version number on this file
@    nins int, ndel int       -- Number of lines inserted and deleted
@ );
@ CREATE INDEX filechng_idx ON filechng(cn);
@
@ -- Every trouble ticket or change request is a single entry in this table
@ -- (The structure of this table is modified by zSchemaChange_1_4[] below.)
@ --
@ CREATE TABLE ticket(
@    tn integer primary key,  -- Unique tracking number for the ticket
@    type text,               -- code, doc, todo, new, or event
@    status text,             -- new, review, defer, active, fixed,
@                             -- tested, or closed
@    origtime int,            -- Time this ticket was first created
@    changetime int,          -- Time of most recent change to this ticket
@    derivedfrom int,         -- This ticket derived from another
@    version text,            -- Version or build number containing the problem
@    assignedto text,         -- Whose job is it to deal with this ticket
@    severity int,            -- How bad is the problem
@    priority text,           -- When should the problem be fixed
@    subsystem text,          -- What subsystem does this ticket refer to
@    owner text,              -- Who originally wrote this ticket
@    title text,              -- Title of this bug
@    description text,        -- Description of the problem
@    remarks text,            -- How the problem was dealt with
@    contact text             -- Contact information for the owner
@ );
@
@ -- Record each change to a bug report
@ --
@ CREATE TABLE tktchng(
@    tn int,                  -- Bug number
@    user text,               -- User that made the change
@    chngtime int,            -- Time of the change
@    fieldid text,            -- Name of the field that changed
@    oldval text,             -- Previous value of the field
@    newval text              -- New value of the field
@ );
@ CREATE INDEX tktchng_idx1 ON tktchng(tn, chngtime);
@
@ -- Miscellaneous configuration parameters
@ --
@ CREATE TABLE config(
@    name text primary key,   -- Name of the configuration parameter
@    value text               -- Value of the configuration parameter
@ );
@ INSERT INTO config(name,value) VALUES('cvsroot','');
@ INSERT INTO config(name,value) VALUES('historysize',0);
@ INSERT INTO config(name,value) VALUES('initial_state','new');
@ INSERT INTO config(name,value) VALUES('schema','1.0');
@
@ -- An entry in the following table describes everything we know
@ -- about a single user
@ --
@ CREATE TABLE user(
@    id text primary key,     -- The user ID
@    name text,               -- Complete name of the user
@    email text,              -- E-mail address for this user
@    passwd text,             -- User password
@    notify text,             -- Type of e-mail to receive
@    http text,               -- URL used by this user to access the site
@    capabilities text        -- What this user is allowed to do
@ );
@ INSERT INTO user(id,name,email,passwd,capabilities)
@   VALUES('setup','Setup Account',NULL,'aISQuNAAoY3qw','ainoprsw');
@
@ -- An entry in this table describes a database query that generates a
@ -- table of tickets.
@ --
@ CREATE TABLE reportfmt(
@    rn integer primary key,  -- Report number
@    owner text,              -- Owner of this report format (not used)
@    title text,              -- Title of this report
@    cols text,               -- A color-key specification
@    sqlcode text             -- An SQL SELECT statement for this report
@ );
@
@ -- Several default report formats:
@ --
@ INSERT INTO reportfmt VALUES(1,NULL,
@   'Recently changed and open tickets',
@   '#ffffff Key:
@ #f2dcdc Active
@ #e8e8bd Review
@ #cfe8bd Fixed
@ #bdefd6 Tested
@ #cacae5 Deferred
@ #c8c8c8 Closed',
@   "SELECT
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
@ WHERE changetime>now()-604800 OR status IN ('new','active')
@ ORDER BY changetime DESC");
@ ----------------------------------------------------------------------------
@ INSERT INTO reportfmt VALUES(2,NULL,
@   'Recently changed and open tickets w/description and remarks',
@   '#ffffff Key:
@ #f2dcdc Active
@ #e8e8bd Review
@ #cfe8bd Fixed
@ #bdefd6 Tested
@ #cacae5 Deferred
@ #c8c8c8 Closed',
@   "SELECT
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
@   title AS 'Title',
@   description AS '_Description',
@   remarks AS '_Remarks'
@ FROM ticket
@ WHERE changetime>now()-604800 OR status IN ('new','active')
@ ORDER BY changetime DESC");
@ ----------------------------------------------------------------------------
@ INSERT INTO reportfmt VALUES(3,NULL,
@   'Tickets associated with a particular user',
@   '#ffffff Priority:
@ #f2dcdc 1
@ #e8e8bd 2
@ #cfe8bd 3
@ #cacae5 4
@ #c8c8c8 5',
@   "SELECT
@   CASE priority WHEN 1 THEN '#f2dcdc'
@        WHEN 2 THEN '#e8e8bd'
@        WHEN 3 THEN '#cfe8bd'
@        WHEN 4 THEN '#cacae5'
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
@ WHERE owner=aux('User',user()) OR assignedto=aux('User',user())");
@ ----------------------------------------------------------------------------
@ INSERT INTO reportfmt VALUES(4,NULL,'All Tickets',
@   '#ffffff Key:
@ #f2dcdc Active
@ #e8e8bd Review
@ #cfe8bd Fixed
@ #bdefd6 Tested
@ #cacae5 Deferred
@ #c8c8c8 Closed',
@   "SELECT
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
@ FROM ticket");
@ ----------------------------------------------------------------------------
@ INSERT INTO reportfmt VALUES(5,NULL,'Tickets counts',NULL,"SELECT
@   status,
@   count(case when type='code' then 'x' end),
@   count(case when type='doc' then 'x' end),
@   count(case when type='new' then 'x' end),
@   count(case when type NOT IN ('code','doc','new') then 'x' end),
@   count(*)
@ FROM ticket GROUP BY status
@ UNION
@ SELECT
@   'TOTAL' AS 'Status',
@   count(case when type='code' then 'x' end) as 'Code Bugs',
@   count(case when type='doc' then 'x' end) as 'Doc Bugs',
@   count(case when type='new' then 'x' end) as 'Enhancements',
@   count(case when type NOT IN ('code','doc','new') then 'x' end)
@      as 'Other',
@   count(*) AS 'All Types'
@ FROM ticket
@ ORDER BY [All Types]");
@ ----------------------------------------------------------------------------
@ 
@ -- This table contains the names of all subsystems.
@ -- (This table is obsolete and is removed by zSchemaChange_1_4 below.)
@ --
@ CREATE TABLE subsyst(name text);
@ INSERT INTO subsyst VALUES('Unknown');
@
@ -- The next table is used for browsing the repository
@ --
@ CREATE TABLE file(
@    isdir boolean,           -- True if this is a directory
@    base text,               -- The basename of the file or directory
@    dir text,                -- Contained in this directory
@    unique(dir,base)
@ );
@ COMMIT;
;

/*
** The previous variable was the original schema version 1.0. The following SQL
** code converts the schema to version 1.1.
**
** The XREF table creates a mapping from check-ins to tickets.  The
** mapping is many-to-many.  This mapping is used to show which tickets
** are effected by a check-in and which check-ins are related to a 
** particular ticket.
*/
static char zSchemaChange_1_1[] =
@ BEGIN;
@ CREATE TABLE xref(
@   tn int,        -- Ticket number.  References TICKET.TN
@   cn int         -- Change number.  References CHNG.CN
@ );
@ CREATE INDEX xref_idx1 ON xref(tn);
@ CREATE INDEX xref_idx2 ON xref(cn);
@ UPDATE config SET value='1.1' WHERE name='schema';
@ COMMIT;
;

/*
** The following are additions to the schema for version 1.2.  The
** cookie table contains login cookies.
**
** Two big changes:  First, add the "cookie" table.  This table records
** the HTTP cookies used to login.  Second, the "wiki" table is added
** to store Wiki pages. 
*/
static char zSchemaChange_1_2[] =
@ BEGIN;
@ CREATE TABLE cookie(
@   cookie char(32) primary key,  -- The login cookie
@   user text,                    -- The user to log in as
@   expires int,                  -- When this cookie expires
@   ipaddr varchar(32),           -- IP address of browser with this cookie
@   agent text                    -- User agent of browser with this cookie
@ );
@ CREATE TABLE wiki(
@   name text,            -- Name of this page
@   invtime int,          -- Inverse timestamp: seconds *before* 1970
@   locked boolean,       -- True if editing is not allowed
@   who text,             -- Who generated this version of the page
@   ipaddr text,          -- IP Address of "who"
@   text clob,            -- Text of the page
@   UNIQUE(name,invtime)
@ );
@ UPDATE config SET value='1.2' WHERE name='schema';
@ COMMIT;
;

/*
** The following are additions to the schema for version 1.3.  A
** new table is added to hold attachments to tickets.
*/
static char zSchemaChange_1_3[] =
@ BEGIN;
@ CREATE TABLE attachment(
@   atn integer primary key, -- Unique key for this attachment
@   tn int,                  -- Ticket that this is attached to
@   size int,                -- Size of the attachment in bytes
@   date int,                -- Date that the attachment was uploaded
@   user text,               -- User who uploaded the attachment
@   mime text,               -- MIME type of the attachment
@   fname text,              -- Filename of the attachment
@   content blob             -- binary data for the attachment
@ );
@ CREATE INDEX attachment_idx1 ON attachment(tn);
@ UPDATE config SET value='1.3' WHERE name='schema';
@ COMMIT;
;

/*
** The following are additions to the schema for version 1.4.  There
** are three major changes.
**
**    1.  Indices are restructured for more efficient operation, especially
**        of the "timeline" function.
**
**    2.  The new ENUMS table is added.  This table contains allowed values
**        for various columns in the TICKET table.  The set of allowed values
**        used to be fixed.  But now users can edit them.  The old SUBSYST
**        table is deleted because it is now subsumed into the ENUMS table.
**
**    3.  The extra1 thru extra5 columns are added to the TICKET table and
**        the order of some of the columns is changed.  The new columns will
**        be used to implement user-defined ticket attributes.
*/
static char zSchemaChange_1_4[] =
@ BEGIN;
@
@ -- Redo the indexing of tables so that fewer full table scans are 
@ -- required to render most pages.
@ --
@ DROP INDEX chng_idx;
@ CREATE INDEX chng_idx1 ON chng(date,user);
@ DROP INDEX filechng_idx;
@ CREATE INDEX filechng_idx1 ON filechng(cn, filename);
@ DROP INDEX tktchng_idx1;
@ CREATE INDEX tktchng_idx1 ON tktchng(chngtime);
@ CREATE INDEX wiki_idx1 ON wiki(invtime);
@
@ -- The new ENUMS table is used to define allowed values of various
@ -- ticket columns.  
@ --
@ CREATE TABLE enums(
@   type text,   -- Which enumeration this entry is part of
@   idx int,     -- The order of this entry within its enumeration
@   name text,   -- The internal name of this enumeration entry
@   value text,  -- The user-visible name of this enumeration entry
@   color text   -- An optional color associated with this enum entry
@ );
@ CREATE INDEX enums_idx1 ON enums(type, idx);
@ CREATE INDEX enums_idx2 ON enums(name, type);
@ INSERT INTO enums VALUES('status',1,'new','New','#f2dcdc');
@ INSERT INTO enums VALUES('status',2,'review','Review','#e8e8bd');
@ INSERT INTO enums VALUES('status',3,'defer','Defer','#cacae5');
@ INSERT INTO enums VALUES('status',4,'active','Active','#f2dcdc');
@ INSERT INTO enums VALUES('status',5,'fixed','Fixed','#cfe8bd');
@ INSERT INTO enums VALUES('status',6,'tested','Tested','#bde5d6');
@ INSERT INTO enums VALUES('status',7,'closed','Closed','#c8c8c8');
@ INSERT INTO enums VALUES('type',1,'code','Code Defect','#f2dcdc');
@ INSERT INTO enums VALUES('type',2,'doc','Documentation','#e8e8bd');
@ INSERT INTO enums VALUES('type',3,'todo','Action Item','#cacae5');
@ INSERT INTO enums VALUES('type',4,'new','Enhancement','#cfe8bd');
@ INSERT INTO enums VALUES('type',5,'event','Incident','#c8c8c8');
@ INSERT INTO enums SELECT 'subsys', rowid, name, name, '' FROM subsyst;
@ DROP TABLE subsyst;
@
@ -- The TICKET table is modified by the addition of five new columns
@ -- named "extra1" throught "extra5".  These columns are used for user
@ -- defined ticket attributes.  The order of the tables is also modified
@ -- so that the big columns (description and remarks) are now at the end.
@ --
@ CREATE TEMP TABLE old_ticket AS SELECT * FROM ticket;
@ DROP TABLE ticket;
@ CREATE TABLE ticket(
@    tn integer primary key,  -- Unique tracking number for the ticket
@    type text,               -- code, doc, todo, new, or event
@    status text,             -- new, review, defer, active, fixed,
@                             -- tested, or closed
@    origtime int,            -- Time this ticket was first created
@    changetime int,          -- Time of most recent change to this ticket
@    derivedfrom int,         -- This ticket derived from another
@    version text,            -- Version or build number containing the problem
@    assignedto text,         -- Whose job is it to deal with this ticket
@    severity int,            -- How bad is the problem
@    priority text,           -- When should the problem be fixed
@    subsystem text,          -- What subsystem does this ticket refer to
@    owner text,              -- Who originally wrote this ticket
@    title text,              -- Title of this bug
@    contact text,            -- Contact information for the owner
@    extra1 numeric,          -- User defined column #1
@    extra2 numeric,          -- User defined column #2
@    extra3 numeric,          -- User defined column #3
@    extra4 numeric,          -- User defined column #4
@    extra5 numeric,          -- User defined column #5
@    description text,        -- Description of the problem
@    remarks text             -- How the problem was dealt with
@ );
@ CREATE INDEX ticket_idx1 ON ticket(origtime);
@ INSERT INTO ticket(tn,type,status,origtime,changetime,derivedfrom,
@                    version,assignedto,severity,priority,subsystem,
@                    owner,title,description,remarks,contact)
@       SELECT * FROM old_ticket;
@
@ -- After the above changes, we have schema version 1.4.
@ --
@ UPDATE config SET value='1.4' WHERE name='schema';
@ COMMIT;
;

/*
** The following changes occur to schema 1.5:
**
**   *  Add the INSPECT table used to record the occurance of inspections
**      of check-ins and the results of those inspections.
**
**   *  Create a new index on TKTCHNG that allows one to search by
**      ticket number
**
**   *  The ACCESS_LOAD table added to record the access history by IP
**      address which is then used to throttle unauthorized spiders.
*/
static char zSchemaChange_1_5[] =
@ BEGIN;
@
@ -- Create an INSPECT table to record inspections of check-ins and the
@ -- results of each inspection
@ --
@ CREATE TABLE inspect(
@   cn integer not null,      -- The check-in that was inspected
@   inspecttime int not null, -- Time that the inspection occurred
@   inspector text not null,  -- Developer who did the inspection
@   ticket int,               -- Ticket related to this inspection - or NULL
@   result text               -- "passed", "failed", or other comments, or NULL
@ );
@ CREATE INDEX inspect_idx1 ON inspect(inspecttime);
@ CREATE INDEX inspect_idx2 ON inspect(cn);
@
@ -- Allow efficient searching of TKTCHNG by TN.
@ --
@ CREATE INDEX tktchng_idx2 ON tktchng(tn);
@
@ -- Record the "load" of requests coming from each IP address.
@ --
@ CREATE TABLE access_load(
@   ipaddr text primary key,   -- IP address from which request originated
@   lastaccess time,           -- Time of last request
@   load real                  -- Cummulative load from this IP
@ );
@
@ -- After the above changes, we have schema version 1.4.
@ --
@ UPDATE config SET value='1.5' WHERE name='schema';
@ COMMIT;
;

/*
** Initialize the main database.
**
** This routine is called when the program is launched from the command-line
** with the "init" argument.  This routine is never called when this
** program is run as a CGI program.
*/
void db_init(void){
  db_execute(zSchema);
  db_execute(zSchemaChange_1_1);
  db_execute(zSchemaChange_1_2);
  db_execute(zSchemaChange_1_3);
  db_execute(zSchemaChange_1_4);
  db_execute(zSchemaChange_1_5);
  initialize_wiki_pages();
}

/*
** When upgrading the schema from version 1.0 to version 1.1, a query
** is executed on all entries of the CHNG table.  This routine is called
** once for each row.  The two parameters are the CHNG.CN value and
** the CHNG.MESSAGE value.  This callback scans the CHNG.MESSAGE looking
** for ticket numbers.  When it finds them, it creates entries in the
** XREF table.
*/
static int upgrade_schema_1_callback(
  void *pNotUsed,  /* Not used */
  int nArg,        /* Number of columns in this result row */
  char **azArg,    /* Text of data in all columns */
  char **azName    /* Names of the columns */
){
  assert( nArg==2 );
  xref_checkin_comment(atoi(azArg[0]), azArg[1]);
  return 0;
}
  
/*
** Update the database schema from version 1.0 to version 1.1.
*/
void db_upgrade_schema_1(void){
  db_execute(zSchemaChange_1_1);
  db_callback_query(upgrade_schema_1_callback, 0, 
    "BEGIN;"
    "DELETE FROM xref;"
    "SELECT cn, message FROM chng WHERE NOT milestone;"
    "COMMIT;");
}

/*
** Implement the crypt() SQL function.  crypt() converts a plain-text
** password into an encrypted password with random salt.
*/
static void f_crypt(sqlite_func *context, int argc, const char **argv){
  const char *zIn = argv[0];
  char *zOut;
  static char zSalt[3] = "aa";
  if( argc!=1 ) return;
  zOut = crypt(zIn, zSalt);
  if( zOut==0 ) return;
  zSalt[0] = zOut[2];
  zSalt[1] = zOut[3];
  zOut = crypt(zIn, zSalt);
  if( zOut==0 ) return;
  sqlite_set_result_string(context, zOut, -1);
}

/*
** Update the database schema from version 1.1 to version 1.2.
*/
void db_upgrade_schema_2(void){
  sqlite *pDb = db_open();
  db_execute(zSchemaChange_1_2);
  sqlite_create_function(pDb, "crypt", 1, &f_crypt, 0);
  db_execute(
    "REPLACE INTO user(id,name,email,passwd,notify,http,capabilities) "
    "  SELECT id, name, email, crypt(passwd), notify, http, capabilities "
    "  FROM user;"
  );
}

/*
** Update the database schema from version 1.2 to version 1.3.
*/
void db_upgrade_schema_3(void){
  db_execute(zSchemaChange_1_3);
}

/*
** Update the database schema from version 1.3 to version 1.4.
*/
void db_upgrade_schema_4(void){
  db_execute(zSchemaChange_1_4);
}

/*
** Update the database schema from version 1.4 to version 1.5.
*/
void db_upgrade_schema_5(void){
  db_execute(zSchemaChange_1_5);
}

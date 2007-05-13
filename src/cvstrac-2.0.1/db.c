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
#include <assert.h>
#include <errno.h>
#include <sqlite3.h>
#include <string.h>
#include <wchar.h>
#include "config.h"
#include "db.h"

/*
** The following is the handle to the open database.
*/
static sqlite3 *pDb = 0;

void db_err(const char *zReason, const char *zQuery, const char *zFormat, ...){
  char *zMsg;

  va_list ap;
  va_start(ap, zFormat);
  zMsg = vmprintf(zFormat, ap);
  va_end(ap);
  cgi_reset_content();
  cgi_set_status(200, "OK");
  cgi_set_content_type("text/html");
  @ <h1>Database Error</h1>
  @ %h(zMsg)
  if( zQuery ){
    @ <blockquote>%h(zQuery)</blockquote>
  }
  if( zReason ){
    @ Reason: %h(zReason)
  }
  cgi_append_header("Pragma: no-cache\r\n");
  cgi_reply();
  db_close();
  exit(0);
}

/*
** Open the database.
** Exit with an error message if the database will not open.
*/
sqlite3 *db_open(void){
  char *zName;

  if( pDb ) return pDb;
  zName = mprintf("%s.db", g.zName);
  if( SQLITE_OK!=sqlite3_open(zName, &pDb) ){
    db_err( sqlite3_errmsg(pDb), 0,
            "db_open: Unable to open the database named \"%h\"", zName );
    sqlite3_close(pDb);
  }
  sqlite3_busy_timeout(pDb, 10000);
  free(zName);
  return pDb;
}

/*
** Close the database
*/
void db_close(void){
  if( pDb ){
    sqlite3_close(pDb);
    pDb = 0;
  }
}

/*
** This SQL authorizer function ensures that only SELECT queries
** are allowed. Rather than rejecting the query entirely (SQLITE_DENY),
** we just ignore it to prevent a DoS via SQL injection.
*/
static int query_authorizer(
  void *NotUsed,
  int type,
  const char *zArg1,
  const char *zArg2,
  const char *zArg3,
  const char *zArg4
){
  if( type==SQLITE_SELECT || type==SQLITE_READ ){
    return SQLITE_OK;
#ifdef SQLITE_FUNCTION
  }else if( type==SQLITE_FUNCTION ){
    return SQLITE_OK;
#endif
  }
#ifdef USE_STRICT_AUTHORIZER
  return SQLITE_DENY;
#else
  return SQLITE_IGNORE;
#endif
}

/*
** The SQL authorizer function for the user-supplies queries.  This
** routine NULLs-out fields of the database we do not want arbitrary
** users to see, such as the USER.PASSWD field.
*/
extern int sqlite3StrICmp(const char*, const char*);
static int access_authorizer(
  void *NotUsed,
  int type,
  const char *zArg1,
  const char *zArg2,
  const char *zArg3,
  const char *zArg4
){
  if( type==SQLITE_SELECT ){
    return SQLITE_OK;
#ifdef SQLITE_FUNCTION
  }else if( type==SQLITE_FUNCTION ){
    return SQLITE_OK;
#endif
  }else if( type==SQLITE_READ ){
    if( sqlite3StrICmp(zArg1,"user")==0 ){
      if( sqlite3StrICmp(zArg2,"passwd")==0 || sqlite3StrICmp(zArg2,"email")==0 ){
        return SQLITE_IGNORE;
      }
    }else if( sqlite3StrICmp(zArg1, "cookie")==0 ){
      return SQLITE_IGNORE;
    }else if( sqlite3StrICmp(zArg1, "config")==0 ){
      return SQLITE_IGNORE;
    }else if( !g.okSetup && sqlite3StrICmp(zArg1, "access_load")==0 ){
      return SQLITE_IGNORE;
    }else if( (!g.okWrite || g.isAnon) && sqlite3StrICmp(zArg1,"ticket")==0
        && sqlite3StrICmp(zArg2,"contact")==0){
      return SQLITE_IGNORE;
    }else if( !g.okCheckout && sqlite3StrICmp(zArg1,"chng")==0 ){
      return SQLITE_IGNORE;
    }else if( !g.okCheckout && sqlite3StrICmp(zArg1,"filechng")==0 ){
      return SQLITE_IGNORE;
    }else if( !g.okCheckout && sqlite3StrICmp(zArg1,"file")==0 ){
      return SQLITE_IGNORE;
    }else if( !g.okCheckout && sqlite3StrICmp(zArg1,"inspect")==0 ){
      return SQLITE_IGNORE;
    }else if( !g.okRead && sqlite3StrICmp(zArg1,"ticket")==0 ){
      return SQLITE_IGNORE;
    }else if( !g.okRead && sqlite3StrICmp(zArg1,"tktchng")==0 ){
      return SQLITE_IGNORE;
    }else if( !g.okRdWiki && sqlite3StrICmp(zArg1,"attachment")==0 ){
      return SQLITE_IGNORE;
    }else if( !g.okRdWiki && sqlite3StrICmp(zArg1,"wiki")==0 ){
      return SQLITE_IGNORE;
    }
    return SQLITE_OK;
  }else{
    return SQLITE_DENY;
  }
}

#define MAX_AUTH_STACK 8
typedef int (*xAuthFunc)(void*,int, const char*,const char*,
                           const char*,const char*);
static xAuthFunc pAuthStack[MAX_AUTH_STACK];
static int nAuthStack = 0;

/*
** A SQLite authorizer function to call a "stack" of functions. The "worst"
** result from any of them is the result used.
*/
static int stack_authorizer(
  void *pTop,
  int type,
  const char *zArg1,
  const char *zArg2,
  const char *zArg3,
  const char *zArg4
){
  int code = SQLITE_OK;
  int nTop;
  for(nTop = (int)pTop; nTop>=0; nTop--){
    int rc = pAuthStack[nTop](NULL,type,zArg1,zArg2,zArg3,zArg4);
    /*
    ** We run through the authorizers one by one until we get the end or find
    ** one which returns an explicit denial or we hit the end. Then we
    ** return the "worst" code. Besides DENY and OK, only IGNORE is expected
    ** but future versions may allow other codes.
    */
    if( rc == SQLITE_DENY ){
      return rc;
    }else if( rc != SQLITE_OK ){
      code = rc;
    }
  }
  return code;
}

static void push_authorizer(xAuthFunc xAuth){
  assert(nAuthStack < MAX_AUTH_STACK);
  pAuthStack[nAuthStack] = xAuth;
  sqlite3_set_authorizer(pDb, stack_authorizer, (void *)nAuthStack);
  nAuthStack += 1;
}

static void pop_authorizer(xAuthFunc xAuth){
  assert(nAuthStack > 0);
  nAuthStack --;
  assert(pAuthStack[nAuthStack]==xAuth);
  sqlite3_set_authorizer(pDb, nAuthStack ? stack_authorizer : 0, (void *)nAuthStack);
}

/*
** Restrict access to sensitive information in the database.
*/
void db_restrict_access(int onoff){
  if( onoff ){
    push_authorizer(access_authorizer);
  }else{
    pop_authorizer(access_authorizer);
  }
}

/*
** Restrict queries to SELECT only.
*/
void db_restrict_query(int onoff){
  if( onoff ){
    push_authorizer(query_authorizer);
  }else{
    pop_authorizer(query_authorizer);
  }
}

/*
** Used to accumulate query results by db_query()
*/
struct QueryResult {
  int nElem;       /* Number of used entries in azElem[] */
  int nAlloc;      /* Number of slots allocated for azElem[] */
  int nCols;       /* Number of columns in the results */
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
  if( pResult->nCols==0 ){
    pResult->nCols = nArg;
  }else if( pResult->nCols!= nArg ){
    db_err("Mismatched number of columns in query results", 0,
           "db_query_callback: Database query failed");
  }
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
  char *zSql;

  assert(zFormat!=0);

  va_start(ap, zFormat);
  zSql = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);

  if( pDb==0 ) db_open();
  memset(&sResult, 0, sizeof(sResult));
  db_restrict_query(1);
  rc = sqlite3_exec(pDb, zSql, db_query_callback, &sResult, &zErrMsg);
  db_restrict_query(0);
  if( rc != SQLITE_OK ){
    db_err( zErrMsg ? zErrMsg : sqlite3_errmsg(pDb), zSql,
            "db_query: Database query failed" );
  }
  free(zSql);
  if( sResult.azElem==0 ){
    db_query_callback(&sResult, 0, 0, 0);
  }
  assert(sResult.azElem!=0);
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
  int rc;
  char *zResult = 0;
  char *zErrMsg = 0;
  char *zSql;

  assert(zFormat);
  if( pDb==0 ) db_open();
  va_start(ap, zFormat);
  zSql = sqlite3_vmprintf(zFormat,ap);
  va_end(ap);
  db_restrict_query(1);
  rc = sqlite3_exec(pDb, zSql, db_short_query_callback, &zResult, &zErrMsg);
  db_restrict_query(0);

  /* short query callback aborts when we get a real value */
  if( rc != SQLITE_OK && rc != SQLITE_ABORT ){
    db_err( zErrMsg ? zErrMsg : sqlite3_errmsg(pDb), zSql,
            "db_short_query: Database query failed" );
  }
  free(zSql);
  return zResult;
}

/*
** Execute an SQL statement against the database.
** Print an error and abort if something goes wrong.
*/
void db_execute(const char *zFormat, ...){
  int rc;
  char *zErrMsg = 0;
  char *zSql;
  va_list ap;

  assert(zFormat);

  if( pDb==0 ) db_open();
  va_start(ap, zFormat);
  zSql = sqlite3_vmprintf(zFormat,ap);
  va_end(ap);
  rc = sqlite3_exec(pDb, zSql, 0, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    db_err(zErrMsg, zSql, "db_execute: Database execute failed");
  }
  free(zSql);
}

/*
** The callback function for db_exists.
*/
static int db_exists_callback(
  void *pUser,     /* Pointer to the QueryResult structure */
  int nArg,        /* Number of columns in this result row */
  char **azArg,    /* Text of data in all columns */
  char **NotUsed   /* Names of the columns */
){
  *((int*)pUser) = 1;
  return 0;
}

/*
** Execute the SQL query statement.  Return TRUE if the query
** would return 1 or more rows.  Return FALSE if the query returns
** an empty set.
*/
int db_exists(const char *zFormat, ...){
  va_list ap;
  int iResult = 0;
  char *zErrMsg = 0;
  char *zSql;
  int rc;

  assert(zFormat);
  if( pDb==0 ) db_open();
  va_start(ap, zFormat);
  zSql = sqlite3_vmprintf(zFormat,ap);
  va_end(ap);
  db_restrict_query(1);
  rc = sqlite3_exec(pDb, zSql, db_exists_callback, &iResult, &zErrMsg);
  db_restrict_query(0);
  if( rc!=SQLITE_OK ){
    db_err(zErrMsg, zSql, "db_exists: Database exists query failed");
  }
  free(zSql);
  return iResult;
}

/*
** Check to see if the SQL query is valid. Return NULL if it's good, otherwise
** return an appropriate error message.
*/
char *db_query_check(const char *zFormat, ...){
  int rc;
  char *zErrMsg = 0;
  char *zSql;
  va_list ap;

  assert(zFormat);
  if( pDb==0 ) db_open();
  va_start(ap, zFormat);
  zSql = sqlite3_vmprintf(zFormat,ap);
  va_end(ap);
  db_restrict_query(1);
  rc = sqlite3_exec(pDb, zSql, 0, 0, &zErrMsg);
  db_restrict_query(0);
  return (rc!=SQLITE_OK) ? zErrMsg : 0;
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
  char *zSql;
  va_list ap;

  if( pDb==0 ) db_open();
  va_start(ap, zFormat);
  zSql = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  db_restrict_query(1);
  rc = sqlite3_exec(pDb, zSql, xCallback, pArg, &zErrMsg);
  db_restrict_query(0);
  if( rc != SQLITE_OK ){
    db_err(zErrMsg ? zErrMsg : sqlite3_errmsg(pDb), zSql,
           "db_callback_query: Database query failed");
  }
  free(zSql);
}

/*
** Execute a query against the database and invoke the
** given callback for each row.
*/
void db_callback_execute(
  int (*xCallback)(void*,int,char**,char**),  /* Callback for each row */
  void *pArg,                                 /* 1st argument to callback */
  const char *zFormat,                        /* SQL for the query */
  ...                                         /* Arguments for the SQL */
){
  int rc;
  char *zErrMsg = 0;
  char *zSql;
  va_list ap;

  if( pDb==0 ) db_open();
  va_start(ap, zFormat);
  zSql = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  rc = sqlite3_exec(pDb, zSql, xCallback, pArg, &zErrMsg);
  if( rc != SQLITE_OK ){
    db_err(zErrMsg ? zErrMsg : sqlite3_errmsg(pDb), zSql,
           "db_callback_execute: Database query failed");
  }
  free(zSql);
}

/*
** Implement the sdate() SQL function.  sdate() converts an integer
** which is the number of seconds since 1970 into a short date or
** time description.  For recent dates (within the past 24 hours)
** just the time is shown.  (ex: 14:23)  For dates within the past
** year, the month and day are shown.  (ex: Apr09).  For dates more
** than a year old, only the year is shown.
*/
static void sdate(sqlite3_context *context, int argc, sqlite3_value **argv){
  time_t now;
  time_t t;
  struct tm *pTm;
  char *zFormat;
  char zBuf[200];
  
  if( argc!=1 ) return;
  t = sqlite3_value_int(argv[0]);
  if( t==0 ) return;
  time(&now);
  if( t+8*3600 > now && t-8*3600 <= now ){
    zFormat = "%H:%M";
  }else if( t+24*3600*120 > now && t-24*3600*120 < now ){
    zFormat = "%b %d";
  }else{
    zFormat = "%Y %b";
  }
  pTm = localtime(&t);
  strftime(zBuf, sizeof(zBuf), zFormat, pTm);
  sqlite3_result_text(context, zBuf, -1, SQLITE_TRANSIENT);
}

/*
** Implement the ldate() SQL function.  ldate() converts an integer
** which is the number of seconds since 1970 into a full date and
** time description.
*/
static void ldate(sqlite3_context *context, int argc, sqlite3_value **argv){
  time_t t;
  struct tm *pTm;
  char zBuf[200];
  
  if( argc!=1 ) return;
  t = sqlite3_value_int(argv[0]);
  if( t==0 ) return;
  pTm = localtime(&t);
  strftime(zBuf, sizeof(zBuf), "%Y-%b-%d %H:%M", pTm);
  sqlite3_result_text(context, zBuf, -1, SQLITE_TRANSIENT);
}

/*
** Implement the parsedate() SQL function.  parsedate() converts an
** ISO8601 date/time string into the number of seconds since 1970.
*/
static void pdate(sqlite3_context *context, int argc, sqlite3_value **argv){
  time_t t;
  const char *z;
  
  if( argc!=1 ) return;
  z = (const char *)sqlite3_value_text(argv[0]);
  t = z ? parse_time(z) : 0;
  sqlite3_result_int(context, t);
}

/*
** Implement the now() SQL function.  now() takes no arguments and
** returns the current time in seconds since 1970.
*/
static void f_now(sqlite3_context *context, int argc, sqlite3_value **argv){
  time_t now;
  time(&now);
  sqlite3_result_int(context, now);
}

/*
** Implement the user() SQL function.  user() takes no arguments and
** returns the user ID of the current user.
*/
static void f_user(sqlite3_context *context, int argc, sqlite3_value **argv){
  if( g.zUser!=0 ) sqlite3_result_text(context, g.zUser, -1, SQLITE_STATIC);
}

/*
** Implement the cgi() SQL function.  cgi() takes a an argument which is
** a name of CGI query parameter. The value of that parameter is returned, 
** if available. optional second argument will be returned if the first
** doesn't exist as a CGI parameter.
*/
static void f_cgi(sqlite3_context *context, int argc, sqlite3_value **argv){
  const char* zP;
  if( argc!=1 && argc!=2 ) return;
  zP = P((const char*)sqlite3_value_text(argv[0]));
  if( zP ){
    sqlite3_result_text(context, zP, -1, SQLITE_STATIC);
  }else if( argc==2 ){
    zP = (const char*)sqlite3_value_text(argv[1]);
    if( zP ) sqlite3_result_text(context, zP, -1, SQLITE_TRANSIENT);
  }
}

/*
** Implement the aux() SQL function. The aux() SQL function takes a paramter
** name as an argument and returns the value that the user enters in the
** resulting HTML form. A second optional parameter provides a default value.
*/
extern int sqlite3StrICmp(const char*, const char*);
static void f_aux(sqlite3_context *context, int argc, sqlite3_value **argv){
  int i;
  const char *zParm;
  if( argc!=1 && argc!=2 ) return;

  zParm = (const char*)sqlite3_value_text(argv[0]);
  if( zParm==0 ) return;

  for(i=0; i<g.nAux && g.azAuxName[i]; i++){
    if( sqlite3StrICmp(zParm,g.azAuxName[i])==0 ){
      if( g.azAuxVal[i] ){
        sqlite3_result_text(context, g.azAuxVal[i], -1, SQLITE_STATIC);
      }
      return;
    }
  }
  if( g.nAux<MX_AUX ){
    const char *zDef = (argc==2) ? (const char*)sqlite3_value_text(argv[1])
                                 : 0;
    g.azAuxName[g.nAux] = mprintf("%s",zParm);
    g.azAuxParam[g.nAux] = mprintf("%s",zParm);
    for(i=0; g.azAuxParam[g.nAux][i]; i++){
      if(!isalnum(g.azAuxParam[g.nAux][i])) g.azAuxParam[g.nAux][i]='_';
    }
    g.azAuxVal[g.nAux] = mprintf("%s",PD(g.azAuxParam[g.nAux],zDef));
    if( g.azAuxVal[g.nAux] ){
      sqlite3_result_text(context, g.azAuxVal[g.nAux], -1, SQLITE_STATIC);
    }
    g.nAux++;
  }
}

/*
** Implement the option() SQL function.  option() takes a parameter name
** and a quoted SELECT statement as parameters. The query results are
** presented as an HTML dropdown menu and the function returns the
** currently selected value.  Results may be a single value column or
** two value,description columns.  The first result row is the default.
*/
extern int sqlite3StrICmp(const char*, const char*);
static void f_option(sqlite3_context *context, int argc, sqlite3_value **argv){
  const char *zParm;
  int i;
  if( argc!=1 && argc!=2 ) return;

  zParm = (const char*)sqlite3_value_text(argv[0]);
  if( zParm==0 ) return;

  for(i=0; i<g.nAux && g.azAuxName[i]; i++){
    if( sqlite3StrICmp(zParm,g.azAuxName[i])==0 ){
      if( g.azAuxVal[i] ){
        sqlite3_result_text(context, g.azAuxVal[i], -1, SQLITE_STATIC);
      }
      return;
    }
  }
  if( argc>1 && g.nAux<MX_AUX ){
    char *zErr;
    char *zQuery = (char *)sqlite3_value_text(argv[1]);
    if( zQuery==0 ) {
      db_err("Query cannot be NULL", "NULL", "Illegal option() query");
    }
    zErr = verify_sql_statement(zQuery);
    if( zErr ){
      db_err( zErr, zQuery, "Illegal option() query" );
    }else{
      int rc;
      struct QueryResult sResult;
      if( pDb==0 ) db_open();
      memset(&sResult, 0, sizeof(sResult));

      db_restrict_query(1);
      rc = sqlite3_exec(pDb, zQuery, db_query_callback, &sResult, &zErr);
      db_restrict_query(0);
      if( rc != SQLITE_OK ){
        db_err( zErr, zQuery, "option() query failed" );
      }
      
      if( sResult.azElem==0 ){
        db_query_callback(&sResult, 0, 0, 0);
      }
      sResult.azElem[sResult.nElem] = 0;

      g.azAuxOpt[g.nAux] = (const char**)sResult.azElem;
      g.anAuxCols[g.nAux] = sResult.nCols;
    }
    g.azAuxName[g.nAux] = mprintf("%s",zParm);
    g.azAuxParam[g.nAux] = mprintf("%s",zParm);
    for(i=0; g.azAuxParam[g.nAux][i]; i++){
      if(!isalnum(g.azAuxParam[g.nAux][i])) g.azAuxParam[g.nAux][i]='_';
    }
    g.azAuxVal[g.nAux] = PD(g.azAuxParam[g.nAux],
                            g.azAuxOpt[g.nAux][0]?g.azAuxOpt[g.nAux][0]:"");
    if( g.azAuxVal[g.nAux] ){
      sqlite3_result_text(context, g.azAuxVal[g.nAux], -1, SQLITE_STATIC);
    }
    g.nAux++;
  }
}
/*
** Implement the path() SQL function. path() takes 3 arguments in this order:
**   isdir, dir, base.
** This func should be used to extract complete filename from FILE table.
*/
static void f_path(sqlite3_context *context, int argc, sqlite3_value **argv){
  char *zPath;
  const char *zDir;
  if( argc!=3 ) return;
  if( SQLITE_NULL==sqlite3_value_type(argv[0])
                 || SQLITE_NULL==sqlite3_value_type(argv[1])
                 || SQLITE_NULL==sqlite3_value_type(argv[2])){
    return;
  }
  zDir = (const char*)sqlite3_value_text(argv[1]);
  if( 0==sqlite3_value_int(argv[0]) ){
    if( zDir && strlen(zDir)>0 ){
      zPath = mprintf("%s/%s", zDir, (const char*)sqlite3_value_text(argv[2]));
    }else{
      zPath = mprintf("%s", (const char*)sqlite3_value_text(argv[2]));
    }
  }else{
    if( zDir && strlen(zDir)>0 ){
      zPath = mprintf("%s/%s/", zDir, (const char*)sqlite3_value_text(argv[2]));
    }else{
      zPath = mprintf("%s/", (const char*)sqlite3_value_text(argv[2]));
    }
  }
  sqlite3_result_text(context, zPath, -1, SQLITE_TRANSIENT);
  free(zPath);
}

/*
** Implement the dirname() SQL function. dirname() returns dirname for given
** filename WITH trailing slash, except when dirname is empty string.
*/
static void f_dirname(sqlite3_context *context, int argc, sqlite3_value **argv){
  const char *z;
  char *zDir;
  int i;
  if( argc!=1 ) return;
  z = (const char*)sqlite3_value_text(argv[0]);
  if( z==0 || z[0]==0 ) return;
  i = strlen(z)-1;
  if( i<=0 ){
    sqlite3_result_text(context, "", -1, SQLITE_TRANSIENT);
    return;
  }
  zDir = mprintf("%s", z);
  if( z[i]=='/' ){ i--; } /* We need to handle dirs too*/
  while( i>=0 && zDir[i]!='/' ){ i--; }
  if( i==0 ){
    zDir = mprintf("");
  }else{
    zDir[i+1] = 0;
  }
  sqlite3_result_text(context, zDir, -1, SQLITE_TRANSIENT);
  free(zDir);
}

/*
** Implement the basename() SQL function. basename() extracts basename
** from given filename.
*/
static void f_basename(sqlite3_context *context, int argc, sqlite3_value **argv){
  const char *z;
  char *zBase;
  int i, nBaseEnd;
  if( argc!=1 ) return;
  z = (const char*)sqlite3_value_text(argv[0]);
  if( z==0 || z[0]==0 ) return;
  i = strlen(z)-1;
  if( i<=0 ){
    sqlite3_result_text(context, "", -1, SQLITE_TRANSIENT);
    return;
  }
  nBaseEnd = (z[i]=='/') ? --i : i;
  while( i>=0 && z[i]!='/' ){ i--; }
  zBase = mprintf("%s", &z[i+1]);
  /* Strip trailing slash in case of directories. NOOP for files. */
  zBase[nBaseEnd-i] = 0;
  sqlite3_result_text(context, zBase, -1, SQLITE_TRANSIENT);
  free(zBase);
}

/*
** The two arguments are capability strings: strings containing lower
** case letters.  Return a string that contains only those letters found
** in both arguments.
**
** Example:  cap_and("abcd","cdef") returns "cd".
*/
static void cap_and(sqlite3_context *context, int argc, sqlite3_value **argv){
  int i, j;
  const char *z;
  char set[26];
  char zResult[27];
  if( argc!=2 ) return;
  for(i=0; i<26; i++) set[i] = 0;
  z = (const char*)sqlite3_value_text(argv[0]);
  if( z ){
    for(i=0; z[i]; i++){
      if( z[i]>='a' && z[i]<='z' ) set[z[i]-'a'] = 1;
    }
  }
  z = (const char*)sqlite3_value_text(argv[1]);
  if( z ){
    for(i=0; z[i]; i++){
      if( z[i]>='a' && z[i]<='z' && set[z[i]-'a']==1 ) set[z[i]-'a'] = 2;
    }
  }
  for(i=j=0; i<26; i++){
    if( set[i]==2 ) zResult[j++] = i+'a';
  }
  zResult[j] = 0;
  sqlite3_result_text(context, zResult, -1, SQLITE_TRANSIENT);
}

/*
** The two arguments are capability strings: strings containing lower
** case letters.  Return a string that contains those letters found
** in either arguments.
**
** Example:  cap_and("abcd","cdef") returns "abcdef".
*/
static void cap_or(sqlite3_context *context, int argc, sqlite3_value **argv){
  int i, j;
  const char *z;
  char set[26];
  char zResult[27];
  if( argc!=2 ) return;
  for(i=0; i<26; i++) set[i] = 0;
  z = (const char*)sqlite3_value_text(argv[0]);
  if( z ){
    for(i=0; z[i]; i++){
      if( z[i]>='a' && z[i]<='z' ) set[z[i]-'a'] = 1;
    }
  }
  z = (const char*)sqlite3_value_text(argv[1]);
  if( z ){
    for(i=0; z[i]; i++){
      if( z[i]>='a' && z[i]<='z' ) set[z[i]-'a'] = 1;
    }
  }
  for(i=j=0; i<26; i++){
    if( set[i] ) zResult[j++] = i+'a';
  }
  zResult[j] = 0;
  sqlite3_result_text(context, zResult, -1, SQLITE_TRANSIENT);
}

/*
** Implementation of the length() function for local encoding
*/
static void lengthFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  int len;
  switch( sqlite3_value_type(argv[0]) ){
    case SQLITE_BLOB:
    case SQLITE_INTEGER:
    case SQLITE_FLOAT: {
      sqlite3_result_int(context, sqlite3_value_bytes(argv[0]));
      break;
    }
    case SQLITE_TEXT: {
      const char *z = (const char *)sqlite3_value_text(argv[0]);
      mbstate_t st;
      memset(&st, 0, sizeof(st));
      len = (int)mbsrtowcs(NULL, &z, 0, &st);
      if( len < 0 )
        len = 0;
      sqlite3_result_int(context, len);
      break;
    }
    default: {
      sqlite3_result_null(context);
      break;
    }
  }
}

/*
** Implementation of the substr() function for local encoding
*/
static void substrFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  const char *z;
  const char *z2;
  int i;
  int p1, p2, len;
  int pz1, pz2, zlen;
  mbstate_t st;
  z = (const char *)sqlite3_value_text(argv[0]);
  if( z==0 ) return;
  zlen = sqlite3_value_bytes(argv[0]);
  p1 = sqlite3_value_int(argv[1]);
  p2 = sqlite3_value_int(argv[2]);
  memset(&st, 0, sizeof(st));
  z2 = z;
  len = (int)mbsrtowcs(NULL, &z2, 0, &st);
  if( len <= 0 ) {
    sqlite3_result_text(context, "", 0, SQLITE_STATIC);
    return;
  }
  if( p1<0 ){
    p1 += len;
    if( p1<0 ){
      p2 += p1;
      p1 = 0;
    }
  }else if( p1>0 ){
    p1--;
  }
  if( p1+p2>len ){
    p2 = len-p1;
  }
  memset(&st, 0, sizeof(st));
  for( pz1=0; z[pz1]; pz1+=i ){
    if( p1--<=0 )
      break;
    i = (int)mbrtowc(NULL, z+pz1, zlen-pz1, &st);
    if( i<=0 )
      break;
  }
  for( pz2=0; z[pz1+pz2]; pz2+=i ){
    if( p2--<=0 )
      break;
    i = (int)mbrtowc(NULL, z+pz1+pz2, zlen-pz1-pz2, &st);
    if( i<=0 )
      break;
  }
  sqlite3_result_text(context, (char*)&z[pz1], pz2, SQLITE_TRANSIENT);
}

/*
** This routine adds the extra SQL functions to the SQL engine.
*/
void db_add_functions(void){
  if( pDb==0 ) db_open();
  sqlite3_create_function(pDb, "sdate", 1, SQLITE_ANY, 0, &sdate, 0, 0);
  sqlite3_create_function(pDb, "ldate", 1, SQLITE_ANY, 0, &ldate, 0, 0);
  sqlite3_create_function(pDb, "parsedate", 1, SQLITE_ANY, 0, &pdate, 0, 0);
  sqlite3_create_function(pDb, "now", 0, SQLITE_ANY, 0, &f_now, 0, 0);
  sqlite3_create_function(pDb, "user", 0, SQLITE_ANY, 0, &f_user, 0, 0);
  sqlite3_create_function(pDb, "aux", 1, SQLITE_ANY, 0, &f_aux, 0, 0);
  sqlite3_create_function(pDb, "aux", 2, SQLITE_ANY, 0, &f_aux, 0, 0);
  sqlite3_create_function(pDb, "cgi", 1, SQLITE_ANY, 0, &f_cgi, 0, 0);
  sqlite3_create_function(pDb, "cgi", 2, SQLITE_ANY, 0, &f_cgi, 0, 0);
  sqlite3_create_function(pDb, "option", 1, SQLITE_ANY, 0, &f_option, 0, 0);
  sqlite3_create_function(pDb, "option", 2, SQLITE_ANY, 0, &f_option, 0, 0);
  sqlite3_create_function(pDb, "path", 3, SQLITE_ANY, 0, &f_path, 0, 0);
  sqlite3_create_function(pDb, "dirname", 1, SQLITE_ANY, 0, &f_dirname, 0, 0);
  sqlite3_create_function(pDb, "basename", 1, SQLITE_ANY, 0, &f_basename, 0, 0);
  sqlite3_create_function(pDb, "cap_or", 2, SQLITE_ANY, 0, &cap_or, 0, 0);
  sqlite3_create_function(pDb, "cap_and", 2, SQLITE_ANY, 0, &cap_and, 0, 0);
  sqlite3_create_function(pDb, "search", -1, SQLITE_ANY, 0, srchFunc, 0, 0);
#ifdef CVSTRAC_I18N
  if( g.useUTF8==0 ){
    sqlite3_create_function(pDb, "length", 1, SQLITE_ANY, 0, lengthFunc, 0, 0);
    sqlite3_create_function(pDb, "substr", 3, SQLITE_ANY, 0, substrFunc, 0, 0);
  }
#endif
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
@   status AS 'Status',
@   count(case when type='code' then 'x' end) AS 'Code Bugs',
@   count(case when type='doc' then 'x' end) AS 'Doc Bugs',
@   count(case when type='new' then 'x' end) AS 'Enhancements',
@   count(case when type NOT IN ('code','doc','new') then 'x' end)
@         AS 'Other',
@   count(*) AS 'All Types'
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
@ -- After the above changes, we have schema version 1.5.
@ --
@ UPDATE config SET value='1.5' WHERE name='schema';
@ COMMIT;
;

/*
** The following changes occur to schema 1.6:
**
**   *  Add a captcha counter to the ACCESS_LOAD table.
**
**   *  Add a "description" field to the ATTACHMENT table.
**
**   *  Add a "directory" field to the CHNG table for milestones.
*/
static char zSchemaChange_1_6[] =
@ BEGIN;
@
@ -- Add a captch countdown to the ACCESS_LOAD table.
@ CREATE TEMP TABLE old_access_load AS SELECT * FROM access_load;
@ DROP TABLE access_load;
@ CREATE TABLE access_load(
@   ipaddr text primary key,   -- IP address from which request originated
@   lastaccess time,           -- Time of last request
@   load real,                 -- Cummulative load from this IP
@   captcha int                -- Number of captcha attempts left
@ );
@ INSERT INTO access_load(ipaddr,lastaccess,load)
@       SELECT * FROM old_access_load;
@
@ -- Add a description field to the ATTACHMENT table
@ CREATE TEMP TABLE old_attachment AS SELECT * FROM attachment;
@ DROP TABLE attachment;
@ CREATE TABLE attachment(
@   atn integer primary key, -- Unique key for this attachment
@   tn int,                  -- Ticket that this is attached to
@   size int,                -- Size of the attachment in bytes
@   date int,                -- Date that the attachment was uploaded
@   user text,               -- User who uploaded the attachment
@   mime text,               -- MIME type of the attachment
@   fname text,              -- Filename of the attachment
@   description text,        -- Description of the attachment
@   content blob             -- binary data for the attachment
@ );
@ INSERT INTO attachment(atn,tn,size,date,user,mime,fname,content)
@       SELECT * FROM old_attachment;
@
@ -- Add a directory field to the CHNG table
@ CREATE TEMP TABLE old_chng AS SELECT * FROM chng;
@ DROP TABLE chng;
@ CREATE TABLE chng(
@    cn integer primary key,   -- A unique "change" number
@    date int,                 -- Time commit occured in seconds since 1970
@    branch text,              -- Name of branch or '' if main trunk
@    milestone int,            -- 0: not a milestone. 1: release, 2: event
@    user text,                -- User who did the commit
@    directory text,           -- Directory/module of chng
@    message text              -- Text of the log message
@ );
@ INSERT INTO chng(cn,date,branch,milestone,user,message)
@       SELECT * FROM old_chng;
@
@ -- After the above changes, we have schema version 1.6.
@ --
@ UPDATE config SET value='1.6' WHERE name='schema';
@ COMMIT;
;

/*
** The following are additions to the schema for version 1.7.  A
** new table is added to hold custom Wiki markup styles
*/
static char zSchemaChange_1_7[] =
@ BEGIN;
@ CREATE TABLE markup(
@   markup text primary key,  -- Style name for this markup
@   type int,                 -- 0: markup, 1: program markup
@                             -- 2: block, 3: program block
@   formatter text,           -- pathname or format string
@   description text          -- description of what the markup does
@ );
@ CREATE INDEX markup_idx1 ON markup(markup);
@ UPDATE config SET value='1.7' WHERE name='schema';
@ COMMIT;
;

/*
** The following are additions to the schema for version 1.8.
**
**   *  add chngtype and prevvers fields to the FILECHNG table
** 
*/
static char zSchemaChange_1_8[] =
@ BEGIN;
@ CREATE TEMP TABLE old_filechng AS SELECT * FROM filechng;
@ DROP TABLE filechng;
@ CREATE TABLE filechng(
@    cn int,                  -- Corresponds to CHNG.CN field
@    filename text,           -- Name of the file
@    vers text,               -- New version number on this file
@    prevvers text,           -- Previous version number on this file
@    chngtype int,            -- 0: modify, 1: add, 2: remove
@    nins int, ndel int,      -- Number of lines inserted and deleted
@    UNIQUE(filename,vers)
@ );
@ INSERT INTO filechng(cn,filename,vers,nins,ndel)
@       SELECT cn,filename,vers,nins,ndel FROM old_filechng;
@ UPDATE config SET value='1.8' WHERE name='schema';
@ COMMIT;
;

/*
** The following are additions to the schema for version 1.9. This version
** introduces an index on FILECHNG(filenane,vers), but the main goal is to
** fix the prevvers,chngtype problems incompleteness in schema 1.8. This
** requires recopying FILECHNG. Most of the real action happens in
** db_upgrade_schema_9(). Because we're basically replicating the changes
** in the original 1.8, 1.8 is now a NOP and is replaced.
**
**   *  add chngtype and prevvers fields to the FILECHNG table
**
**   *  initialize svnlastupdate in CONFIG
** 
*/
static char zSchemaChange_1_9[] =
@ BEGIN;
@ CREATE TEMP TABLE old_filechng2 AS SELECT * FROM filechng;
@ DROP TABLE filechng;
@ CREATE TABLE filechng(
@    cn int,                  -- Corresponds to CHNG.CN field
@    filename text,           -- Name of the file
@    vers text,               -- New version number on this file
@    prevvers text,           -- Previous version number on this file
@    chngtype int,            -- 0: modify, 1: add, 2: remove
@    nins int, ndel int,      -- Number of lines inserted and deleted
@    UNIQUE(filename,vers)
@ );
@ COMMIT;
;

/*
** Schema 2.0 is the first of the SQLite 3 changes. Rather than a true
** schema change, it's actually a reencoding of the attachments into
** BLOBs.
*/
static char zSchemaChange_2_0[] =
@ BEGIN;
@ UPDATE config SET value='2.0' WHERE name='schema';
@ COMMIT;
;

/*
** Schema 2.1 attempts to solve problems with /dirview sorting by keeping
** last cn of file in FILE table.
*/
static char zSchemaChange_2_1[] =
@ BEGIN;
@ CREATE TEMP TABLE old_file AS SELECT * FROM file;
@ DROP TABLE file;
@ CREATE TABLE file(
@    isdir boolean,           -- True if this is a directory
@    base text,               -- The basename of the file or directory
@    dir text,                -- Contained in this directory
@    lastcn int,              -- Last CHNG.cn of the file or directory
@    unique(dir,base)
@ );
@ UPDATE config SET value='2.1' WHERE name='schema';
@ COMMIT;
;

/*
** The following are additions to the schema for version 2.2.  A
** new table is added to hold external tools which can be used against
** various CVSTrac objects (repository files, wiki pages, etc).
*/
static char zSchemaChange_2_2[] =
@ BEGIN;
@ CREATE TABLE tool(
@   name text primary key,  -- Name of tool
@   perms text,             -- Required permissions to see/use tool
@   object text,            -- target object. "wiki", "file", "tkt",
@                           -- "chng", "ms", "rpt", "dir"
@   command text,           -- tool command-line
@   description text        -- description of what the tool does
@ );
@ UPDATE config SET value='2.2' WHERE name='schema';
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
  db_execute(zSchemaChange_1_6);
  db_execute(zSchemaChange_1_7);
  db_execute(zSchemaChange_1_8);
  db_execute(zSchemaChange_1_9);
  db_execute(zSchemaChange_2_0);
  db_execute(zSchemaChange_2_1);
  db_execute(zSchemaChange_2_2);
  initialize_wiki_pages();
}

/*
** The zMsg parameter is a check-in comment for check-in number "cn".  Create
** entries in the XREF table from this check-in comment.
*/
void xref_checkin_comment(int cn, const char *zMsg, int isRemove){
  int i, j, tn;
  
  for(i=0; zMsg[i]; i++){
    if( zMsg[i]=='#' && (j = ndigit(&zMsg[i+1]))>0 
               && is_eow(&zMsg[i+1+j],0)
               && (tn = atoi(&zMsg[i+1]))>0 ){
      if( isRemove ){
        db_execute("DELETE FROM xref WHERE tn=%d AND cn=%d",tn,cn);
      }else{
        /* avoid duplicate xrefs... they're harmless, but we end up listing
        ** them multiple times in output.
        */
        if( !db_exists("SELECT * FROM xref WHERE tn=%d AND cn=%d", tn, cn) ){
          char zSql[200];
          bprintf(zSql, sizeof(zSql), "INSERT INTO xref(tn,cn) VALUES(%d,%d)",
                  tn, cn);
          db_execute(zSql);
        }
      }
    }
  }
}

void xref_add_checkin_comment(int cn, const char *zMsg){
  xref_checkin_comment(cn, zMsg, 0);
}

void xref_remove_checkin_comment(int cn, const char *zMsg){
  xref_checkin_comment(cn, zMsg, 1);
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
  xref_add_checkin_comment(atoi(azArg[0]), azArg[1]);
  return 0;
}
  
/*
** Update the database schema from version 1.0 to version 1.1.
*/
void db_upgrade_schema_1(void){
  db_execute(zSchemaChange_1_1);
  db_callback_execute(upgrade_schema_1_callback, 0, 
    "BEGIN;"
    "DELETE FROM xref;"
    "SELECT cn, message FROM chng WHERE NOT milestone;"
    "COMMIT;");
}

/*
** Implement the crypt() SQL function.  crypt() converts a plain-text
** password into an encrypted password with random salt.
*/
static void f_crypt(sqlite3_context *context, int argc, sqlite3_value **argv){
  char *zOut;
  static char zSalt[3] = "aa";
  if( argc==1 ) {
    const char *zIn = (const char*)sqlite3_value_text(argv[0]);
    zOut = crypt(zIn, zSalt);
    if( zOut==0 ) return;
    zSalt[0] = zOut[2];
    zSalt[1] = zOut[3];
    zOut = crypt(zIn, zSalt);
    if( zOut==0 ) return;
    sqlite3_result_text(context, zOut, -1, SQLITE_TRANSIENT);
  }
}

/*
** Update the database schema from version 1.1 to version 1.2.
*/
void db_upgrade_schema_2(void){
  if( pDb==0 ) db_open();
  db_execute(zSchemaChange_1_2);
  sqlite3_create_function(pDb, "crypt", 1, SQLITE_ANY, 0, &f_crypt, 0, 0);
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

/*
** Update the database schema from version 1.5 to version 1.6.
*/
void db_upgrade_schema_6(void){
  db_execute(zSchemaChange_1_6);
}

/*
** Update the database schema from version 1.6 to version 1.7.
*/
void db_upgrade_schema_7(void){
  db_execute(zSchemaChange_1_7);
}

/*
** Update the database schema from version 1.7 to version 1.8.
*/
void db_upgrade_schema_8(void){
  db_execute(zSchemaChange_1_8);
}

/*
** Update the database schema from version 1.7/1.8 to version 1.9.
*/
void db_upgrade_schema_9(void){
  /* zSchemaChange_1_9 copies FILECHNG to the temporary OLD_FILECHNG table and
   * creates an empty FILECHNG. Following code will fill FILECHNG.
   */
  db_execute(zSchemaChange_1_9);
  db_execute("BEGIN;");

  if( !strcmp(g.scm.zSCM,"cvs") ){
    char **azFile;
    int i, j, k;
    char *zFile;
    char *zVers;
    char zPrevVers[100];
    int lastchngtype, chngtype;

    /* This query _should_ give us a depth-first search through the CVS branch
    ** tree. If not, things will get wonky when the RCS file is in the Attic.
    */ 
    azFile = db_query("SELECT cn,filename,vers,nins,ndel FROM old_filechng2 "
                             "ORDER BY filename,cn DESC,vers DESC");

    for(i=0; azFile[i]; ){
      zFile = azFile[i+1];

      /* is_file_available() is a fairly expensive operation (calls access(2))
      ** so we want to check that as few times as possible. We only need
      ** to know if it's been moved to the Attic once per file, then we
      ** can fill in the other versions with zero until we get to the
      ** first add.  FIXME This doesn't work perfectly for branches where
      ** the file became "dead" in one branch but not another.
      */

      chngtype = lastchngtype = is_file_available(zFile) ? 0 : 2;

      for(j=0; azFile[i+j]; j+=5){
        zVers = azFile[i+j+2];

        /* run the inner loop until we get to a different filename */
        if( j!=0 && strcmp(azFile[i+j+1],zFile) ) break;

        strncpy( zPrevVers, zVers, sizeof(zPrevVers)-2 );

        /* by not passing the filename, we avoid a database query. We
        ** don't need a query because we have the entire list of known
        ** versions in azFile.
        */
        cvs_previous_version(zPrevVers,NULL);

        if( zPrevVers[0] ){
          /* We've calculate what the previous version should be. See if
          ** it's actually in the list. It _should_ be the next entry unless
          ** there's a branch. In some cases, the expected previous version
          ** won't exist because the repository admin manually changed
          ** version numbers. This isn't critical, but it does mean that
          ** the prevvers chain gets broken.
          */
          for(k=j+5; azFile[i+k]; k+=5){
            if( !strcmp(zPrevVers, azFile[i+k+2]) ) break;
            if( strcmp(azFile[i+k+1], zFile) ) break;
          }
        
          if( azFile[i+k]==0 || strcmp(azFile[i+k+1],zFile) ){
            zPrevVers[0]=0;
          }
        }

        if( zPrevVers[0] ){
          db_execute("INSERT INTO "
                     "filechng(cn,filename,vers,nins,ndel,prevvers,chngtype) "
                     "VALUES(%d,'%q','%q',%d,%d,'%q',%d)",
                     atoi(azFile[i+j]),zFile,zVers,atoi(azFile[i+j+3]),
                     atoi(azFile[i+j+4]), zPrevVers, chngtype);

          /* only the latest entry in a revision chain is a remove, the
          ** others have to be modify or adds. As long as we can calculate
          ** a previous revision, it must be a modify until we get to a
          ** new branch.
          */
          chngtype = 0;
        } else {
          /* no previous version for this file, which means it's
          ** an add. It may also just mean that the revision chain was
          ** broken because someone messed with the revision numbers in a
          ** non-chainable way.
          */
          db_execute("INSERT INTO "
                     "filechng(cn,filename,vers,nins,ndel,prevvers,chngtype) "
                     "VALUES(%d,'%q','%q',%d,%d,'',1)",
                     atoi(azFile[i+j]),zFile,zVers,
                     atoi(azFile[i+j+3]),atoi(azFile[i+j+4]));

          /* the next revision we see will be the latest revision in
          ** another branch. Reset chngtype.
          */
          chngtype = lastchngtype;
        }
      }
      assert( j != 0 );
      i += j;
    }

    db_query_free(azFile);
  }else{
    /* Otherwise, just make a copy of the fields. */
    db_execute("INSERT INTO filechng SELECT * FROM old_filechng2; "
               "INSERT INTO config(name,value) VALUES('svnlastupdate',0); ");
  }

  db_execute("CREATE INDEX filechng_idx1 ON filechng(filename,vers); "
             "UPDATE config SET value='1.9' WHERE name='schema'; "
             "COMMIT;");
}

/*
** Decode the string "in" into binary data and write it into "out".
** This routine reverses the encoded created by sqlite_encode_binary().
** The output will always be a few bytes less than the input.  The number
** of bytes of output is returned.  If the input is not a well-formed
** encoding, -1 is returned.
**
** The "in" and "out" parameters may point to the same buffer in order
** to decode a string in place.
*/
static int blob_decode(const unsigned char *in, unsigned char *out){
  int i, c, e;
  e = *(in++);
  i = 0;
  while( (c = *(in++))!=0 ){
    if( c==1 ){
      c = *(in++);
      if( c==1 ){
        c = 0;
      }else if( c==2 ){
        c = 1;
      }else if( c==3 ){
        c = '\'';
      }else{
        return -1;
      }
    }
    out[i++] = (c + e)&0xff;
  }
  return i;
}

/*
** Calls blob_decode() to decode the body of the attachment.
*/
static void f_decode(sqlite3_context *context, int argc, sqlite3_value **argv){
  if( argc==2 ) {
    const char *zIn = (const char*)sqlite3_value_text(argv[0]);
    int nBytes = sqlite3_value_int(argv[1]);
    if( zIn && zIn[0] && nBytes>0 ){
      char *zOut = calloc(nBytes,1);
      int nDecoded;
      if( zOut==0 ){
        db_err( strerror(errno), 0,
                "f_decode: Unable to allocate %d bytes", nBytes);
      }
      nDecoded = blob_decode((const unsigned char *)zIn,(unsigned char *)zOut);
      sqlite3_result_blob(context, zOut, nBytes, SQLITE_TRANSIENT);
      free(zOut);
    }
  }
}
/*
** Update the database schema from version 1.9 to version 2.0
*/
void db_upgrade_schema_20(void){
  if( pDb==0 ) db_open();
  sqlite3_create_function(pDb, "decode", 2, SQLITE_ANY, 0, &f_decode, 0, 0);
  db_execute(
    "REPLACE INTO attachment "
    "  SELECT atn, tn, size, date, user, mime, fname, description, "
    "         decode(content,size)"
    "  FROM attachment;"
  );
  db_execute(zSchemaChange_2_0);
}

/*
** Populate FILE with lastcn for each file/dir, updating schema from 2.0 to
** 2.1.
*/
void db_upgrade_schema_21(void){
  db_execute(zSchemaChange_2_1);
  db_execute("BEGIN");
  update_file_table_with_lastcn();
  db_execute("COMMIT");
}

/*
** Update the database schema from version 2.1 to version 2.2
*/
void db_upgrade_schema_22(void){
  db_execute(zSchemaChange_2_2);
}

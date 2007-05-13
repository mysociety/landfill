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
** This file includes the code required to generate a fairly large "test"
** CVSTrac database.
**
** Generated database is schema version 1.0. Make sure you run
**   cvstrac update `pwd` <DATABASE>
** before trying to use it.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sqlite3.h>

#define MAX_CHNG 20000
#define MAX_FILECHNG 10         /* per CHNG */
#define MAX_DEPTH 3             /* directories in a FILECHNG */
#define MAX_DIR 8              /* breadth of directory tree */
#define CODE_OVER 5*86400*365   /* approx 5 years of development */

static void generate_chng(sqlite3 *db, time_t when, int cn){
  int rc;
  char *zSql;
  char *zErrMsg;
  char zDir[4096];
  char zBase[4096];
  int depth = 1+(rand()%MAX_DEPTH);
  int num_filechng = 1+(rand()%MAX_FILECHNG);
  int j,k;

  zDir[0] = 0;

  sqlite3_exec(db,"BEGIN;",0,0,&zErrMsg);

  /* For simplicity, each CHNG record only hits a single directory. */
  for(j=k=0;j<depth;j++){
    sprintf(zBase,"dir_%d",rand()%MAX_DIR);
    zSql = sqlite3_mprintf("REPLACE INTO file VALUES(1,'%q','%q')",zBase,zDir);
    sqlite3_exec(db,zSql,0,0,&zErrMsg);
    free(zSql);
    if(zDir[0]) strcat(zDir,"/");
    strcat(zDir,zBase);
  }

  /* FILECHNG records need to be unique for any given cn, so we
  ** fudge things a touch here
  */
  for(j=0,k=rand()%10; j<num_filechng; j++,k+=1+(rand()%3)){
    sprintf(zBase,"file_%d.c",k);
    zSql = sqlite3_mprintf("REPLACE INTO file VALUES(0,'%q','%q')",zBase,zDir);
    sqlite3_exec(db,zSql,0,0,&zErrMsg);
    free(zSql);

    /* FIXME: this is definitely _not_ generating CVS version numbers */
    zSql = sqlite3_mprintf("INSERT INTO filechng "
                           "VALUES(%d,'%q/%q','%d',%d,%d)",
                           cn,zDir,zBase,cn,rand()%10,rand()%10);
    sqlite3_exec(db,zSql,0,0,&zErrMsg);
    free(zSql);
  }

  zSql = sqlite3_mprintf("INSERT INTO chng "
                       "VALUES(%d,%d,'',0,'setup','checkin %d of %d');"
                       "COMMIT",
                       cn, when, cn, MAX_CHNG);
  rc = sqlite3_exec(db,zSql,0,0,&zErrMsg);
  free(zSql);
}

/***********************************************************************/
/*
** The (original version 1.0) database schema is defined by the following SQL.
**
** Note that for test purposes, we remove the default reports. We also
** insert an anonymous user with setup capabilities just to make life
** simpler.
*/
static char zSchema[] =
" BEGIN TRANSACTION; \n"
" \n"
" -- Each 'cvs commit' results in a single entry in the following table. \n"
" -- Even commits that involve multiple files generate just a single \n"
" -- table entry. \n"
" -- \n"
" CREATE TABLE chng( \n"
"    cn integer primary key,   -- A unique 'change' number \n"
"    date int,                 -- Time commit occured in seconds since 1970 \n"
"    branch text,              -- Name of branch or '' if main trunk \n"
"    milestone int,            -- 0: not a milestone. 1: release, 2: event \n"
"    user text,                -- User who did the commit \n"
"    message text              -- Text of the log message \n"
" ); \n"
" CREATE INDEX chng_idx ON chng(user,message); \n"
" \n"
" -- Each file involved in a commit operation results in an entry in \n"
" -- this table. \n"
" -- \n"
" CREATE TABLE filechng( \n"
"    cn int,                  -- Corresponds to CHNG.CN field \n"
"    filename text,           -- Name of the file \n"
"    vers text,               -- New version number on this file \n"
"    nins int, ndel int       -- Number of lines inserted and deleted \n"
" ); \n"
" CREATE INDEX filechng_idx ON filechng(cn); \n"
" \n"
" -- Every trouble ticket or change request is a single entry in this table \n"
" -- (The structure of this table is modified by zSchemaChange_1_4[] below.) \n"
" -- \n"
" CREATE TABLE ticket( \n"
"    tn integer primary key,  -- Unique tracking number for the ticket \n"
"    type text,               -- code, doc, todo, new, or event \n"
"    status text,             -- new, review, defer, active, fixed, \n"
"                             -- tested, or closed \n"
"    origtime int,            -- Time this ticket was first created \n"
"    changetime int,          -- Time of most recent change to this ticket \n"
"    derivedfrom int,         -- This ticket derived from another \n"
"    version text,            -- Version or build number containing the problem \n"
"    assignedto text,         -- Whose job is it to deal with this ticket \n"
"    severity int,            -- How bad is the problem \n"
"    priority text,           -- When should the problem be fixed \n"
"    subsystem text,          -- What subsystem does this ticket refer to \n"
"    owner text,              -- Who originally wrote this ticket \n"
"    title text,              -- Title of this bug \n"
"    description text,        -- Description of the problem \n"
"    remarks text,            -- How the problem was dealt with \n"
"    contact text             -- Contact information for the owner \n"
" ); \n"
" \n"
" -- Record each change to a bug report \n"
" -- \n"
" CREATE TABLE tktchng( \n"
"    tn int,                  -- Bug number \n"
"    user text,               -- User that made the change \n"
"    chngtime int,            -- Time of the change \n"
"    fieldid text,            -- Name of the field that changed \n"
"    oldval text,             -- Previous value of the field \n"
"    newval text              -- New value of the field \n"
" ); \n"
" CREATE INDEX tktchng_idx1 ON tktchng(tn, chngtime); \n"
" \n"
" -- Miscellaneous configuration parameters \n"
" -- \n"
" CREATE TABLE config( \n"
"    name text primary key,   -- Name of the configuration parameter \n"
"    value text               -- Value of the configuration parameter \n"
" ); \n"
" INSERT INTO config(name,value) VALUES('cvsroot',''); \n"
" INSERT INTO config(name,value) VALUES('historysize',0); \n"
" INSERT INTO config(name,value) VALUES('initial_state','new'); \n"
" INSERT INTO config(name,value) VALUES('schema','1.0'); \n"
" \n"
" -- An entry in the following table describes everything we know \n"
" -- about a single user \n"
" -- \n"
" CREATE TABLE user( \n"
"    id text primary key,     -- The user ID \n"
"    name text,               -- Complete name of the user \n"
"    email text,              -- E-mail address for this user \n"
"    passwd text,             -- User password \n"
"    notify text,             -- Type of e-mail to receive \n"
"    http text,               -- URL used by this user to access the site \n"
"    capabilities text        -- What this user is allowed to do \n"
" ); \n"
" INSERT INTO user(id,name,email,passwd,capabilities) \n"
"   VALUES('setup','Setup Account',NULL,'setup','ainoprsw'); \n"
" \n"
" -- An entry in this table describes a database query that generates a \n"
" -- table of tickets. \n"
" -- \n"
" CREATE TABLE reportfmt( \n"
"    rn integer primary key,  -- Report number \n"
"    owner text,              -- Owner of this report format (not used) \n"
"    title text,              -- Title of this report \n"
"    cols text,               -- A color-key specification \n"
"    sqlcode text             -- An SQL SELECT statement for this report \n"
" ); \n"
" \n"
" -- This table contains the names of all subsystems. \n"
" -- (This table is obsolete and is removed by zSchemaChange_1_4 below.) \n"
" -- \n"
" CREATE TABLE subsyst(name text); \n"
" INSERT INTO subsyst VALUES('Unknown'); \n"
" \n"
" -- The next table is used for browsing the repository \n"
" -- \n"
" CREATE TABLE file( \n"
"    isdir boolean,           -- True if this is a directory \n"
"    base text,               -- The basename of the file or directory \n"
"    dir text,                -- Contained in this directory \n"
"    unique(dir,base) \n"
" ); \n"
" INSERT INTO user(id,name,email,passwd,capabilities) \n"
"   VALUES('anonymous','Anon',NULL,'','ainoprsw'); \n"
" COMMIT;";

int main(int argc, char **argv){
  sqlite3 *db;
  char *zErrMsg;
  char zDbName[4096];
  int rc;
  int i;
  time_t base = time(0);
  time_t now;
  int interval = CODE_OVER / MAX_CHNG;

  if( argc!=2 ){
    fprintf(stderr,"Usage: %s DATABASE\n", argv[0]);
    exit(1);
  }
  sprintf(zDbName, "%s.db", argv[1]);
  if( !access(zDbName, W_OK) ){
    fprintf(stderr,"%s exists and is writable\n", zDbName);
    exit(1);
  }
  if( SQLITE_OK != sqlite3_open(zDbName, &db) ){
    fprintf(stderr,"Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(1);
  }

  /* a certain amount of repeatability is nice */
  srand(0);

  /* Initialize the database */
  rc = sqlite3_exec(db,zSchema,0,0,&zErrMsg);

  /*
  ** Generate a whole bunch of checkins scattered over a few years
  ** of development time.
  */
  for( i=0, now=(base-CODE_OVER); i<MAX_CHNG; i++, now += interval ){
    generate_chng(db, now, i);
  }

  sqlite3_close(db);

  exit(0);
}

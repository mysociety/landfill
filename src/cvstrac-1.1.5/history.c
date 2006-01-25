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
** This file contains code used to read the CVSROOT/history file from
** the CVS archive and update the CHNG and FILECHNG tables according to
** the content of the history file.
*/
#include "config.h"
#include <time.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include "history.h"

/*
** This routine finds the name of an RCS repository file given its
** base filename and the root directory of the repository.  The
** algorithm first searches in the main directory, then in the Attic.
**
** Space to hold the returned name is obtained from malloc and must
** be freed by the calling function.
**
** NULL is return if the file cannot be found.
*/
char *find_repository_file(const char *zRoot, const char *zBase){
  char *zFile = mprintf("%s/%s,v", zRoot, zBase);
  if( access(zFile, 0) ){
    int n;
    free(zFile);
    n = strlen(zBase);
    while( n>-0 && zBase[n-1]!='/' ){ n--; }
    if( n>0 ){
      zFile = mprintf("%s/%.*s/Attic/%s,v", zRoot, n-1, zBase, &zBase[n]);
    }else{
      zFile = mprintf("%s/Attic/%s,v", zRoot, zBase);
    }
    if( access(zFile, 0) ){
      free(zFile);
      zFile = 0;
    }
  }
  return zFile;
}

/*
** Make sure the given file or directory is contained in the
** FILE table of the database.
*/
static void insert_file(const char *zName){
  int i;
  char *zBase, *zDir, *z;
  char *zToFree;
  int isDir = 0;

  zToFree = zDir = mprintf("%s", zName);
  while( zDir && zDir[0] ){
    for(i=strlen(zDir)-1; i>0 && zDir[i]!='/'; i--){}
    if( i==0 ){
      zBase = zDir;
      zDir = "";
    }else{
      zDir[i] = 0;
      zBase = &zDir[i+1];
    }
    z = db_short_query(
          "SELECT 1 FROM file WHERE dir='%q' AND base='%q'",
          zDir, zBase);
    if( z ){
      free(z);
      break;
    }
    free(z);
    db_execute(
      "INSERT INTO file(isdir, base, dir) "
      "VALUES(%d,'%q','%q')",
      isDir, zBase, zDir);
    isDir = 1;
    zName = zDir;
  }
  free(zToFree);
}

/*
** The zMsg parameter is a check-in comment for check-in number "cn".  Create
** entries in the XREF table from this check-in comment.
*/
void xref_checkin_comment(int cn, const char *zMsg){
  int i, tn;
  for(i=0; zMsg[i]; i++){
    if( zMsg[i]=='#' && isdigit(zMsg[i+1]) && (tn = atoi(&zMsg[i+1]))>0 ){
      char zSql[200];
      sprintf(zSql, "INSERT INTO xref(cn,tn) VALUES(%d,%d)", cn, tn);
      db_execute(zSql);
    }
  }
}

/*
** WEBPAGE: /update_file_table
**
** Make sure the FILE table contains every file mentioned in
** FILECHNG.
*/
void update_file_table(void){
  login_check_credentials();
  if( g.okSetup ){
    char **az;
    int i;
    az = db_query("SELECT DISTINCT filename FROM filechng");
    for(i=0; az[i]; i++){
      insert_file(az[i]);
    }
    db_query_free(az);
  }
  cgi_redirect("index");
}

/*
** Convert a struct tm* that represents a moment in UTC into the number
** of seconds in 1970, UTC.
*/
static time_t mkgmtime(struct tm *p){
  time_t t;
  int nDay;
  int isLeapYr;
  /* Days in each month:       31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 */
  static int priorDays[]   = {  0, 31, 59, 90,120,151,181,212,243,273,304,334 };
  if( p->tm_mon<0 ){
    int nYear = (11 - p->tm_mon)/12;
    p->tm_year -= nYear;
    p->tm_mon += nYear*12;
  }else if( p->tm_mon>11 ){
    p->tm_year += p->tm_mon/12;
    p->tm_mon %= 12;
  }
  isLeapYr = p->tm_year%4==0 && (p->tm_year%100!=0 || (p->tm_year+300)%400==0);
  p->tm_yday = priorDays[p->tm_mon] + p->tm_mday - 1;
  if( isLeapYr && p->tm_mon>1 ) p->tm_yday++;
  nDay = (p->tm_year-70)*365 + (p->tm_year-69)/4 -p->tm_year/100 + 
         (p->tm_year+300)/400 + p->tm_yday;
  t = ((nDay*24 + p->tm_hour)*60 + p->tm_min)*60 + p->tm_sec;
  return t;
}

/*
** An instance of the following structure maps branch version numbers
** into branch names
*/
typedef struct Br Br;
struct Br {
  char *zVers;    /* Version number */
  char *zName;    /* Symbolic name */
  Br *pNext;      /* Next mapping in a linked list */
};

/*
** This routine is called to complete the generation of an error
** message in the history_update module.
*/
static void error_finish(int nErr){
  if( nErr==0 ) return;
  @ </ul>
  common_footer();
  cgi_reply();
  exit(0);
}

/*
** This routine is called whenever an error situation is encountered.
** It makes sure an appropriate header has been issued.
*/
static void error_init(int *pnErr){
  if( *pnErr==0 ){
    common_standard_menu(0, 0);
    cgi_reset_content();
    cgi_set_status(200, "OK");
    cgi_set_content_type("text/html");
    common_header("Error Reading CVSROOT/history");
    @ <p>The following errors occurred while trying to read and
    @ interpret CVSROOT/history file from the CVS repository.  This
    @ indicates a problem in the installation of CVSTrac.  Please save
    @ this page and contact your system administrator.</p>
    @ <ul>
  }
  ++*pnErr;
}

/*
** Search the PATH environment variable for an executable named
** zProg.  If not found, issue an error message and exit.  If
** The program is found, return without doing anything.
*/
static void check_path(int nErr, const char *zProg){
  char *zPath;
  char *zBuf;
  char *z, *zEnd;
  int size;
  z = getenv("PATH");
  if( z==0 ){
    error_init(&nErr);
    @ <li><p>No PATH environment variable</p></li>
    error_finish(nErr);
  }
  size = strlen(zProg) + strlen(z) + 2;
  zPath = malloc( size*2 );
  if( zPath==0 ){
    error_init(&nErr);
    @ <li><p>Out of memory!</p></li>
    error_finish(nErr);
  }
  strcpy(zPath, z);
  zBuf = &zPath[size];
  for(z=zPath; z && *z; z=zEnd){
    zEnd = strchr(z,':');
    if( zEnd ){
      zEnd[0] = 0;
      zEnd++;
    }else{
      zEnd = &z[strlen(z)];
    }
    sprintf(zBuf, "%s/%s", z, zProg);
    if( access(zBuf,X_OK)==0 ){
      free(zPath);
      return;
    }
  }
  error_init(&nErr);
  @ <li><p>Unable to locate program "<b>%h(zProg)</b>".
  @ (uid=%d(getuid()), PATH=%h(getenv("PATH")))</p></li>
  error_finish(nErr);
}

/*
** Turn this on to help debug the history_update() procedure.  With this
** define turned on, diagnostic output is left in tables of the database.
*/
#define HISTORY_TRACE 0

#if HISTORY_TRACE
#  define HTRACE(L,V)  db_execute("INSERT INTO dmsg VALUES('%q','%q')",L,V)
#else
#  define HTRACE(L,V)
#endif

/*
** Check the CVSROOT/history file to see if it has been enlarged since the
** last time it was read.  If so, then read the part that we have not yet
** read and update the CHNG and FILECHNG tables to show the new information.
**
** If any errors occur, output an error page and exit.
**
** If the "isReread" flag is set, it means that the history file is being
** reread to pick up changes that we may have missed earlier.  
*/
void history_update(int isReread){
  int iOldSize;
  const char *zRoot;
  char *zFilename;
  const char *zModule;
  char **azResult;
  char **azFileList;
  FILE *in;
  int i, nField;
  time_t minTime, maxTime, tm;
  struct stat statbuf;
  int cnum = 0;     /* check-in number to use for this checkin */
  int next_cnum;    /* next unused check-in number */
  struct tm *pTm;
  char *zTRange;
  int nErr = 0;
  int path_ok = 0;
  char *azField[20];
  char zLine[2000];
 
  db_execute("BEGIN"); 
  iOldSize = atoi(db_config("historysize","0"));
  zRoot = db_config("cvsroot","");
  zFilename = mprintf("%s/CVSROOT/history", zRoot);
  if( stat(zFilename, &statbuf) || statbuf.st_size==iOldSize ){
    /* The size of the history file has not changed. 
    ** Exit without doing anything */
    db_execute("COMMIT");
    return;
  }
  in = fopen(zFilename,"r");
  if( in==0 ){
    error_init(&nErr);
    @ <li><p>Unable to open the history file %h(zFilename).</p></li>
    error_finish(nErr);
    return;
  }

  /* The "fc" table records changes to files.  Basically, each line of
  ** the CVSROOT/history file results in one entry in the "fc" table.
  **
  ** The "rev" table holds information about various versions of a particular
  ** file.  The output of the "rlog" command is used to fill in this table.
  */
#if HISTORY_TRACE
  db_execute(
    "CREATE TABLE fc(time,user,file,vers);"
    "CREATE TABLE rev(time,ins,del,user,branch,vers,file,comment);"
    "CREATE TABLE dmsg(label,value);"
  );
#else
  db_execute(
    "CREATE TEMP TABLE fc(time,user,file,vers text);"
    "CREATE TEMP TABLE rev(time,ins,del,user,branch,vers text,file,comment);"
  );
#endif

  /* Find the next available change number
  */
  azResult = db_query("SELECT max(cn) FROM chng");
  next_cnum = atoi(azResult[0])+1;
  db_query_free(azResult);

  /*
  ** Read the tail of the history file that has not yet been read.
  */
  fseek(in, iOldSize, SEEK_SET);
  minTime = 0x7fffffff;
  maxTime = 0;
  while( fgets(zLine,sizeof(zLine),in) ){
    int c;
    /* The first character of each line tells what the line means:
    **
    **      A    A new file is added to the repository
    **      M    A change is made to an existing file
    **      R    A file is removed
    **      T    Tagging operations
    */
    if( (c = zLine[0])!='A' && c!='M' && c!='T' && c!='R' ) continue;
    if( sscanf(&zLine[1],"%lx",&tm)!=1 ) continue;
    if( tm<minTime ) minTime = tm;
    if( tm>maxTime ) maxTime = tm;

    /* Break the line up into fields separated by the '|' character.
    */
    for(i=nField=0; zLine[i]; i++){
      if( zLine[i]=='|' && nField<sizeof(azField)/sizeof(azField[0])-1 ){
        azField[nField++] = &zLine[i+1];
        zLine[i] = 0;
      }
      if( zLine[i]=='\r' || zLine[i]=='\n' ) zLine[i] = 0;
    }

    /* Record all 'A' (add file), 'M' (modify file), and 'R' (removed file)
    ** lines in the "fc" temporary table.
    */
    if( (c=='A' || c=='M' || c=='R') && nField>=5 ){
      db_execute("INSERT INTO fc VALUES(%d,'%q','%q/%q','%q')",
                 tm, azField[0], azField[2], azField[4], azField[3]);
    }

    /* 'T' lines represent tag creating or deletion.  Construct or modify
    ** corresponding milestones in the database.
    */
    if( zLine[0]=='T' ){
      int date;
      int isDelete;
      struct tm sTm;
      isDelete = azField[2][0]=='D';
      if( azField[2][0]=='A' || azField[2][0]=='D' ){
        date = tm;
      }else if( sscanf(azField[2],"%d.%d.%d.%d.%d.%d",
                  &sTm.tm_year, &sTm.tm_mon, &sTm.tm_mday,
                  &sTm.tm_hour, &sTm.tm_min, &sTm.tm_sec)==6 ){
        sTm.tm_year -= 1900;
        sTm.tm_mon--;
        date = mkgmtime(&sTm);
      }else{
        continue;
      }
      db_execute("DELETE FROM chng WHERE milestone=2 AND message='%q'",
                 azField[3]);
      if( isDelete ) continue;
      cnum = next_cnum++;
      db_execute("INSERT INTO chng(cn,date,branch,milestone,user,message) "
         "VALUES(%d,%d,'',2,'%q','%q');",
         cnum, date, azField[0], azField[3]);
    }
  }

  /*
  ** Update the "historysize" entry so that we know how much of the history
  ** file has been read.  And close the CVSROOT/history file because we
  ** are finished with it - all the information we need is now in the
  ** "fc" temporary table.
  */
  db_execute("UPDATE config SET value=%d WHERE name='historysize'",
             ftell(in));
  db_config(0,0);
  fclose(in);

  /*
  ** Make sure we recorded at least one file change.  If there were no
  ** file changes in history file, we can stop here.
  */
  if( minTime>maxTime ){
    db_execute("COMMIT");
    return;
  }

  /*
  ** If the "module" configuration parameter exists and is not an empty string,
  ** then delete from the FC table all records dealing with files that are
  ** not a part of the specified module.
  */
  zModule = db_config("module", 0);
  if( zModule && zModule[0] ){
    db_execute(
      "DELETE FROM fc WHERE file NOT LIKE '%q%%'",
      zModule
    );
  }

  /*
  ** Extract delta comments from all files that have changed.
  **
  ** For each file that has changed, we run the "rlog" command see all
  ** check-ins that have occurred within an hour of the span of times
  ** that were read from the history file.  This makes sure wee see
  ** all of the check-ins, but it might also see some check-ins that have
  ** already been recorded in the database by a prior run of this procedure.
  ** Those duplicate check-ins will be removed in a subsequent step.
  */
  azFileList = db_query("SELECT DISTINCT file FROM fc");
  minTime -= 3600;
  pTm = gmtime(&minTime);
  strftime(zLine, sizeof(zLine)-1, "%Y-%m-%d %H:%M:%S", pTm);
  i = strlen(zLine);
  strcpy(&zLine[i],"<=");
  i += 2;
  maxTime += 3600;
  pTm = gmtime(&maxTime);
  strftime(&zLine[i], sizeof(zLine)-i-1, "%Y-%m-%d %H:%M:%S", pTm);
  zTRange = mprintf("%s",zLine);
  for(i=0; azFileList[i]; i++){
    char *zCmd;
    char *zFile;
    int nComment;
    int nIns, nDel;
    Br *pBr;
    char *zBr;
    time_t tm;
    int seen_sym = 0;
    int seen_rev = 0;
    char zVers[100];
    char zUser[100];
    char zComment[2000];

    zFile = find_repository_file(zRoot, azFileList[i]);
    if( zFile==0 ){
      error_init(&nErr);
      @ <li><p>Unable to locate the file %h(azFileList[i]) in the
      @ CVS repository</p></li>
      continue;
    }
    zCmd = mprintf("rlog '-d%s' '%s' 2>/dev/null", 
               quotable_string(zTRange), quotable_string(zFile));
    free(zFile);
    HTRACE("zCmd",zCmd);
    in = popen(zCmd, "r");
    if( in==0 ){
      error_init(&nErr);
      @ <li><p>Unable to execute the following command:
      @ <blockquote><pre>
      @ %h(zCmd)
      @ </pre></blockquote></p></li>
      free(zCmd);
      continue;
    }
    nComment = nIns = nDel = 0;
    pBr = 0;
    zBr = "";
    zUser[0] = 0;
    zVers[0] = 0;
    tm = 0;
    while( fgets(zLine, sizeof(zLine), in) ){
      if( strncmp(zLine,"symbolic names:", 14)==0 ){
        /* Lines in the "symbolic names:" section always begin with a tab.
        ** Each line consists of a tab, the name, and a version number.
        ** We are only interested in branch names.  Branch names always contain
        ** a ".0." in the version number.  example:
        **
        **          xyzzy: 1.2.0.3
        **
        ** For each branch, create an instance of a Br structure to record
        ** the version number prefix and name of that branch.
        */
        seen_sym = 1;
        while( fgets(zLine, sizeof(zLine), in) && zLine[0]=='\t' ){
          int i, j;
          char *zV;
          int nDot;
          int isBr;
          Br *pNew;

          /* Find the ':' that separates name from version number */
          for(i=1; zLine[i] && zLine[i]!=':'; i++){}
          if( zLine[i]!=':' ) continue;
          zLine[i] = 0;

          /* Make zV point to the version number */
          zV = &zLine[i+1];
          while( isspace(*zV) ){ zV++; }

          /* Check to see if zV contains ".0." */
          for(i=1, isBr=nDot=0; zV[i] && !isspace(zV[i]); i++){
            if( zV[i]=='.' ){
              nDot++;
              if( zV[i+1]=='0' && zV[i+2]=='.' ) isBr = i;
            }
          }
          if( !isBr || nDot<3 ) continue;  /* Skip the rest of no ".0." */

          /* Remove the ".0." from the version number.  For example,
          ** "1.2.0.3" becomes "1.2.3".
          */
          zV[i] = 0;
          for(i=isBr, j=isBr+2; zV[j]; j++, i++){
            zV[i] = zV[j];
          }
          zV[i++] = '.';
          zV[i] = 0;

          /* Create a Br structure to record this branch name.
          */
          pNew = malloc( sizeof(*pNew) + strlen(&zLine[1]) + i + 2 );
          pNew->pNext = pBr;
          pNew->zVers = (char*)&pNew[1];
          pNew->zName = &pNew->zVers[i+1];
          strcpy(pNew->zVers, zV);
          strcpy(pNew->zName, &zLine[1]);
          pBr = pNew;
        }
      }else if( strncmp(zLine,"revision ", 9)==0 ){
        int j;
        Br *p;
        for(j=9; isspace(zLine[j]); j++){}
        strncpy(zVers, &zLine[j], sizeof(zVers)-1);
        zVers[sizeof(zVers)-1] = 0;
        j = strlen(zVers);
        while( j>0 && isspace(zVers[j-1]) ){ j--; }
        zVers[j] = 0;
        nComment = nIns = nDel = 0;
        zUser[0] = 0;
        zBr = "";
        for(p=pBr; p; p=p->pNext){
          int n = strlen(p->zVers);
          if( strncmp(zVers, p->zVers, n)==0 && strchr(&zVers[n],'.')==0 ){
            zBr = p->zName;
            break;
          }
        }
        seen_rev++;
      }else if( strncmp(zLine,"date: ", 6)==0 ){
        char *z;
        struct tm sTm;
        if( sscanf(&zLine[6],"%d/%d/%d %d:%d:%d",
                  &sTm.tm_year, &sTm.tm_mon, &sTm.tm_mday,
                  &sTm.tm_hour, &sTm.tm_min, &sTm.tm_sec)==6 ){
          sTm.tm_year -= 1900;
          sTm.tm_mon--;
          tm = mkgmtime(&sTm);
        }
        z = strstr(zLine, "author: ");
        if( z ){
          strncpy(zUser, &z[8], sizeof(zUser)-1);
          zUser[sizeof(zUser)-1] = 0;
          z = strchr(zUser,';');
          if( z ) *z = 0;
        }
        z = strstr(zLine, "lines: ");
        if( z ){
          sscanf(&z[7], "%d %d", &nIns, &nDel);
        }
      }else if( strncmp(zLine,"branches: ", 10)==0 ){
        /* Ignore this line */
      }else if( (strncmp(zLine,"-----", 5)==0 || strncmp(zLine,"=====",5)==0)
             && zVers[0] && tm>0 ){
        while( nComment>0 && isspace(zComment[nComment-1]) ){ nComment--; }
        zComment[nComment] = 0;
        db_execute(
          "INSERT INTO rev VALUES(%d,%d,%d,'%q','%q','%s','%q','%q')",
          tm, nIns, -nDel, zUser, zBr, zVers, azFileList[i], zComment
        );
        zVers[0] = 0;
        tm = 0;
        zUser[0] = 0;
      }else if( zVers[0] && zUser[0] ){
        int len = strlen(zLine);
        if( len+nComment >= sizeof(zComment)-1 ){
          len = sizeof(zComment)-nComment-1;
          if( len<0 ) len = 0;
          zLine[len] = 0;
        }
        strcpy(&zComment[nComment], zLine);
        nComment += len;
      }
    }
    while( pBr ){
      Br *pNext = pBr->pNext;
      free(pBr);
      pBr = pNext;
    }
    pclose(in);
    if( seen_rev==0 ){
      error_init(&nErr);
      if( !path_ok ){
        check_path(nErr, "rlog");
        path_ok = 1;
      }
      @ <p><li>No revision information found in <b>rlog</b> output:
      @ <blockquote><pre>
      @ %h(zCmd);
      @ </pre></blockquote></p></li>
    }else if( seen_sym==0 ){
      error_init(&nErr);
      @ <p><li>No "<b>symbolic names:</b>" line seen in <b>rlog</b> output:
      @ <blockquote><pre>
      @ %h(zCmd);
      @ </pre></blockquote></p></li>
    }
    free(zCmd);
  }
  db_query_free(azFileList);

  /* Delete entries from the REV table that already exist in the database.
  */
  db_execute(
    "CREATE INDEX rev_idx1 ON rev(file,vers);"
    "DELETE FROM rev WHERE rowid IN ("
       "SELECT rev.rowid FROM filechng, rev "
       "WHERE filechng.filename=rev.file AND filechng.vers=rev.vers"
    ");"
  );

  /* Scan through the REV table to construct CHNG and FILECHNG entries
  */
  azResult = db_query(
     "SELECT time, user, branch, vers, ins, del, file, comment FROM rev "
     "ORDER BY time, comment, user, branch"
  );
  for(i=0; azResult[i]; i+=8){
    if(                      /* For each FILECHNG, create a new CHNG if... */
       i==0 ||                                      /* first entry */
       strcmp(azResult[i+7],azResult[i-1])!=0 ||    /* or comment changed */
       strcmp(azResult[i+1],azResult[i-7])!=0 ||    /* or user changed */
       strcmp(azResult[i+2],azResult[i-6])!=0 ||    /* or branch changed */
       atoi(azResult[i])>atoi(azResult[i-8])+30     /* or not with 30 seconds */
    ){
      int add_chng = 1;
      if( isReread ){
        const char *zPrior = db_short_query(
          "SELECT cn FROM chng WHERE date=%s AND user='%q'",
          azResult[i], azResult[i+1]
        );
        if( zPrior==0 || (cnum = atoi(zPrior))<=0 ){
          cnum = next_cnum++;
        }else{
          add_chng = 0;
        }
      }else{
        cnum = next_cnum++; 
      }
      if( add_chng ){
        db_execute(
          "INSERT INTO chng(cn, date, branch, milestone, user, message) "
          "VALUES(%d,%s,'%q',0,'%q','%q')",
          cnum, azResult[i], azResult[i+2], azResult[i+1], azResult[i+7]
        );
        xref_checkin_comment(cnum, azResult[i+7]);
      }
    }
    db_execute(
      "REPLACE INTO filechng(cn,filename,vers,nins,ndel) "
      "VALUES(%d,'%q','%s',%s,%s)",
      cnum, azResult[i+6], azResult[i+3], azResult[i+4], azResult[i+5]
    );
    insert_file(azResult[i+6]);
  }
  db_query_free(azResult);

  /* Commit all changes to the database
  */
  db_execute("COMMIT;");
  error_finish(nErr);
}

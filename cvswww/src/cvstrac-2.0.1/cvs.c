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
** the content of the history file. All the other CVS-specific stuff should also
** be found here.
*/
#include "config.h"
#include <time.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <assert.h>
#include <sys/types.h>
#include <pwd.h>  /* for getpwuid() */
#include "cvs.h"

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
static char *find_repository_file(const char *zRoot, const char *zBase){
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
** Given the name of a file relative to the repository root,
** return the complete pathname of the file.
*/
static char *real_path_name(const char *zPath){
  char *zName, *zBase, *zDir;
  char *zReal;
  const char *zRoot;
  int i, j;

  zRoot = db_config("cvsroot", 0);
  if( zRoot==0 ){ return 0; }
  zName = mprintf("%s", zPath);
  for(i=j=0; zName[i]; i++){
    if( zName[i]=='/' ){
      while( zName[i+1]=='/' ){ i++; }
      if( zName[i+1]==0 ) break;
    }
    zName[j++] = zName[i];
  }
  zName[j] = 0;
  zDir = mprintf("%s/%s", zRoot, zName);
  zBase = strrchr(zDir, '/');
  if( zBase==0 ){
    zBase = zDir;
    zDir = strdup(".");
  }else{
    *zBase = 0;
    zBase++;
  }
  zReal = find_repository_file(zDir, zBase);
  free(zName);
  free(zDir);
  return zReal;
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
    bprintf(zBuf, size, "%s/%s", z, zProg);
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
** previous version finder which doesn't use the prevvers
** field of filechng.  The new version number overwrites the old one.
**
** Examples:  "1.12" becomes "1.11".  "1.22.2.1" becomes "1.22".
**
** The special case of "1.1" becomes "" and the function returns zero.
**
** azVers should contain a list of "known good" version numbers.
*/
int cvs_previous_version(char *zVers,const char *zFile){
  size_t sz = strlen(zVers)+1;
  while( zVers[0] ) {
    int j, x;
    int n = strlen(zVers);
    for(j=n-2; j>=0 && zVers[j]!='.'; j--){}
    if(j<0) break;
    j++;
    x = atoi(&zVers[j]);
    if( x>1 ){
      bprintf(&zVers[j],sz-j,"%d",x-1);
    }else{
      for(j=j-2; j>0 && zVers[j]!='.'; j--){}
      if(j<0) break;
      zVers[j] = 0;
    }

    if(zVers[0]) {
      if( zFile ){
        if( db_exists("SELECT 1 FROM filechng "
                      "WHERE filename='%q' AND vers='%q'",
                      zFile,zVers) ){
          return 1;
        }
      }else{
        /* can't determine if it's good or bad, so assume good */
        break;
      }
    }
  }

  return zVers[0];
}

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
static int cvs_history_update(int isReread){
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
  time_t window = atoi(db_config("checkin_time_window","30"));
  unsigned long t;

  if( window < 30 ) window = 30;
 
  db_execute("BEGIN"); 
  iOldSize = atoi(db_config("historysize","0"));
  zRoot = db_config("cvsroot","");
  zFilename = mprintf("%s/CVSROOT/history", zRoot);
  if( stat(zFilename, &statbuf) || statbuf.st_size==iOldSize ){
    /* The size of the history file has not changed. 
    ** Exit without doing anything */
    db_execute("COMMIT");
    return 0;
  }
  in = fopen(zFilename,"r");
  if( in==0 ){
    error_init(&nErr);
    @ <li><p>Unable to open the history file %h(zFilename).</p></li>
    error_finish(nErr);
    return -1;
  }

  /* The "fc" table records changes to files.  Basically, each line of
  ** the CVSROOT/history file results in one entry in the "fc" table.
  **
  ** The "rev" table holds information about various versions of a particular
  ** file.  The output of the "rlog" command is used to fill in this table.
  */
#if HISTORY_TRACE
  db_execute(
    "CREATE TABLE fc(time,user,file,chngtype,vers);"
    "CREATE TABLE rev(time,ins,del,user,branch,vers,file,comment);"
    "CREATE TABLE dmsg(label,value);"
  );
#else
  db_execute(
    "CREATE TEMP TABLE fc(time,user,file,chngtype,vers text);"
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
    if( sscanf(&zLine[1],"%lx",&t)!=1 ) continue;
    tm = (time_t)t;
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
      db_execute("INSERT INTO fc VALUES(%d,'%q','%q/%q',%d,'%q')",
                 tm, azField[0], azField[2], azField[4],
                 ((c=='A') ? 1 : ((c=='R') ? 2 : 0)),
                 azField[3]);
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
      }else if(azField[2][0]) {
        /* most likely we're tagging a tag. This may have been done to
        ** turn a tag into a branch, for example. As long as nobody has
        ** editted the message we should be able to grab a date.
        */
        char *z = db_short_query("SELECT date FROM chng "
                                 "WHERE milestone AND message='%q'",
                                 azField[2]);
        if( z==0 ) continue;
        date = atoi(z);
        if( date==0 ) continue;
      }else{
        continue;
      }
      /*
      ** Older db schema's didn't have anywhere to put the directory
      ** information. This meant that an rtag effectively was repository
      ** wide rather than module/directory specific. By deleting these
      ** older tags (with NULL directory entries), we're basically
      ** maintaining the semantics those tags were created under.
      */
      db_execute("DELETE FROM chng WHERE "
                 "milestone=2 AND message='%q' AND "
                 "  (directory ISNULL OR directory='%q');",
                 azField[3],azField[4]);
      if( isDelete ) continue;
      cnum = next_cnum++;
      db_execute("INSERT INTO "
                 "chng(cn,date,branch,milestone,user,message,directory) "
                 "VALUES(%d,%d,'',2,'%q','%q','%q');",
                 cnum, date, azField[0], azField[3], azField[4]);
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
    return 0;
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
    const char *azSubst[10];

    zFile = find_repository_file(zRoot, azFileList[i]);
    if( zFile==0 ){
      error_init(&nErr);
      @ <li><p>Unable to locate the file %h(azFileList[i]) in the
      @ CVS repository</p></li>
      continue;
    }

    /*
    ** Some users might need to run rlog output through a filter for
    ** charset conversions.
    */
    azSubst[0] = "F";
    azSubst[1] = quotable_string(zFile);
    azSubst[2] = "TR";
    azSubst[3] = quotable_string(zTRange);
    azSubst[4] = "RP";
    azSubst[5] = db_config("cvsroot", "");
    azSubst[6] = 0;
    zCmd = subst(db_config("rlog", "rlog '-d%TR' '%F' 2>/dev/null"), azSubst);

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
          if( pNew==0 ){
            error_init(&nErr);
            @ <li><p>Out of memory at:
            @ <blockquote><pre>
            @ %h(zLine)
            @ </pre></blockquote></p></li>
            break;
          }
          pNew->pNext = pBr;
          pNew->zVers = (char*)&pNew[1];
          pNew->zName = &pNew->zVers[i+1];
          strcpy(pNew->zVers, zV);
          strcpy(pNew->zName, &zLine[1]);
          pBr = pNew;
        }
      }else if( strncmp(zLine,"revision ", 9)==0 && !zVers[0] ){
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
      }else if( strncmp(zLine,"date: ", 6)==0 && zVers[0] && tm==0 ){
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
  ** Note that we have to ensure the revision comparison is string-based and
  ** not numeric.
  */
  db_execute(
    "CREATE INDEX rev_idx1 ON rev(file,vers);"
    "DELETE FROM rev WHERE rowid IN ("
       "SELECT rev.rowid FROM filechng, rev "
       "WHERE filechng.filename=rev.file AND filechng.vers||''=rev.vers||''"
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
       atoi(azResult[i])>atoi(azResult[i-8])+window /* or not with n seconds */
    ){
      int add_chng = 1;
      if( isReread ){
        const char *zPrior = db_short_query(
          "SELECT cn FROM chng WHERE date=%d AND user='%q'",
          atoi(azResult[i]), azResult[i+1]
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
          "VALUES(%d,%d,'%q',0,'%q','%q')",
          cnum, atoi(azResult[i]), azResult[i+2], azResult[i+1], azResult[i+7]
        );
        xref_add_checkin_comment(cnum, azResult[i+7]);
      }
    }

    
    {
      char zVers[100];
      char *az = db_short_query("SELECT chngtype FROM fc WHERE "
                                "file='%q' AND vers='%q'",
                                azResult[i+6], azResult[i+3]);
      int chngtype = az ? atoi(az) : 2;
      if(az) free(az);

      if( chngtype!=1 ){
        /* go figure previous version. */
        /* FIXME: it'd be better to determine this during the rlog or something.
         * This is more brittle than I like. It should be fine for the most part if
         * repository admins don't directly mess with revision numbers, but the
         * prevvers chain gets broken if that happens. So far the effects are
         * benign.
         */
        strncpy(zVers,azResult[i+3],sizeof(zVers));
        cvs_previous_version(zVers,azResult[i+6]);
      }else{
        zVers[0] = 0;
      }

      db_execute(
        "REPLACE INTO filechng(cn,filename,vers,prevvers,chngtype,nins,ndel) "
        "VALUES(%d,'%q','%q','%q',%d,%d,%d)",
        cnum, azResult[i+6], azResult[i+3], zVers, chngtype,
        atoi(azResult[i+4]), atoi(azResult[i+5])
      );
    }
    
    if( iOldSize>0 ) insert_file(azResult[i+6], cnum);
  }
  db_query_free(azResult);
  
  /* We delayed populating FILE till now on initial scan */
  if( iOldSize==0 ){
    update_file_table_with_lastcn();
  }
  
  /* Commit all changes to the database
  */
  db_execute("COMMIT;");
  error_finish(nErr);

  return nErr ? -1 : 0;
}

/*
** Check a repository file for the presence of the -kb option.
*/
static int has_binary_keyword(const char* filename){
  FILE* in;
  char line[80];
  int has_binary=0;

  in = fopen(filename, "r");
  if( in==0 ) return 2;

  while( fgets(line, sizeof(line), in) ){
    /* End of header? */
    if( line[0]=='\n' || line[0]=='\r' ){
      break;
    }

    /* Is this the "expand" field? */
#define EXPAND "expand"
    if( strncmp(line, EXPAND, strlen(EXPAND))==0 ){
      /* Does its value contain 'b'? */
      if( strchr(line+strlen(EXPAND), 'b') ){
        has_binary=1;
      }
      break;
    }
  }

  fclose(in);
  return has_binary;
}

/*
** Diff two versions of a file, handling all exceptions.
**
** If oldVersion is NULL, then this function will output the
** text of version newVersion of the file instead of doing
** a diff.
*/
static int cvs_diff_versions(
  const char *oldVersion,
  const char *newVersion,
  const char *zRelFile
){
  const char *zTemplate;
  char *zCmd;
  FILE *in;
  const char *azSubst[10];
  char *zFile;

  if( zRelFile==0 ) return -1;

  zFile = real_path_name(zRelFile);
  if( zFile==0 ) return -1;

  /* Check file for binary keyword */
  if( has_binary_keyword(zFile) ){
    free(zFile);
    @ <p>
    @ %h(zRelFile) is a binary file
    @ </p>
    return 0; /* Don't attempt to compare binaries, but it's not a failure */
  }

  if( oldVersion[0]==0 ){
    @ %h(zRelFile)  -> %h(newVersion)
  }else{
    @ %h(zRelFile)  %h(oldVersion) -> %h(newVersion)
  }
  cgi_append_content("\n", 1);

  /* Find the command used to compute the file difference.
  */
  azSubst[0] = "F";
  azSubst[1] = zFile;
  if( oldVersion[0]==0 ){
    zTemplate = db_config("filelist",
      "co -q -p'%V' '%F' | diff -c /dev/null - 2>/dev/null");
    azSubst[2] = "V";
    azSubst[3] = newVersion;
    azSubst[4] = "RP";
    azSubst[5] = db_config("cvsroot", "");
    azSubst[6] = 0;
  }else{
    zTemplate = db_config("filediff","rcsdiff -q '-r%V1' '-r%V2' -u '%F' 2>/dev/null");
    azSubst[2] = "V1";
    azSubst[3] = oldVersion;
    azSubst[4] = "V2";
    azSubst[5] = newVersion;
    azSubst[6] = "RP";
    azSubst[7] = db_config("cvsroot", "");
    azSubst[8] = 0;
  }
  zCmd = subst(zTemplate, azSubst);
  free(zFile);
  in = popen(zCmd, "r");
  free(zCmd);
  if( in==0 ) return -1;

  output_pipe_as_html(in,0);
  pclose(in);

  return 0;
}

static int cvs_is_file_available(const char *file){
  const char *zRoot = db_config("cvsroot","");
  char *zFilename = NULL;

  int available = ((zRoot!=0)
    && (zFilename = mprintf("%s/%s,v", zRoot, file))!=0
    && access(zFilename,0)==0);

  if(zFilename) free(zFilename);
  return available;
}

static int cvs_dump_version(const char *zVersion, const char *zFile,int bRaw){
  int rc = 0;
  char *zReal = real_path_name(zFile);
  if( zReal==0 ) return -1;

  if( !bRaw && has_binary_keyword(zReal) ){

    /* FIXME: could do a hex dump, but yuck... */
    @ <tt>%h(zFile)</tt> is a binary file.
  }else{
    char *zCmd = mprintf("co -q '-p%s' '%s' 2>/dev/null", 
      quotable_string(zVersion), quotable_string(zReal));
    rc = common_dumpfile( zCmd, zVersion, zFile, bRaw );
    free(zCmd);
  }

  free(zReal);
  return rc;
}

/*
** Diff two versions of a file, handling all exceptions, and output
** a raw patch.
**
** If oldVersion is NULL, then this function will output the
** text of version newVersion of the file instead of doing
** a diff.
*/
static void raw_diff_versions(
  const char *oldVersion,
  const char *newVersion,
  const char *zRelFile
){
  const char *zTemplate;
  char *zCmd;
  FILE *in;
  const char *azSubst[16];
  const char *zRoot = db_config("cvsroot", 0);

  char *zFile = find_repository_file(zRoot, zRelFile);
  if( zFile==0 ) return;

  /* Check file for binary keyword. cvs diff doesn't handle binaries
  ** either, so no big loss.
  */
  if( has_binary_keyword(zFile) ){
    free(zFile);
    return;
  }

  /* Find the command used to compute the file difference.
  */
  azSubst[0] = "F";
  azSubst[1] = zFile;
  azSubst[2] = "R";
  azSubst[3] = zRelFile;
  if( is_dead_revision(zRelFile,oldVersion) ){
    zTemplate = "co -q -kk -p'%V' '%F' | diff -u /dev/null - -L'%R' 2>/dev/null";
    azSubst[4] = "V";
    azSubst[5] = newVersion;
    azSubst[6] = 0;
  }else if( is_dead_revision(zRelFile,newVersion) ){
    zTemplate = "co -q -kk -p'%V' '%F' | diff -u - /dev/null -L'%R' 2>/dev/null";
    azSubst[4] = "V";
    azSubst[5] = oldVersion;
    azSubst[6] = 0;
  }else{
    zTemplate = "rcsdiff -q -kk '-r%V1' '-r%V2' -u '%F' 2>/dev/null";
    azSubst[4] = "V1";
    azSubst[5] = oldVersion;
    azSubst[6] = "V2";
    azSubst[7] = newVersion;
    azSubst[8] = 0;
  }
  zCmd = subst(zTemplate, azSubst);

  /* patch doesn't need to guess filenames if we give it an index line.
  ** Some extra cvs-like information doesn't hurt, either.
  */
  cgi_printf("Index: %s\n", zRelFile);
  cgi_printf("RCS File: %s\n", zFile );
  cgi_printf("%s\n", zCmd );

  free(zFile);
  in = popen(zCmd, "r");
  free(zCmd);
  if( in==0 ) return;

  while( !feof(in) && !ferror(in) ){
    char zBuf[1024];
    size_t n = fread( zBuf,1,sizeof(zBuf),in );
    if( n > 0 ){
      cgi_append_content(zBuf,n);
    }
  }
  pclose(in);
}

static int cvs_diff_chng(int cn, int bRaw){
  int i;
  char **azFile;

  azFile = db_query("SELECT filename, vers, prevvers "
                    "FROM filechng WHERE cn=%d ORDER BY filename", cn);

  for(i=0; azFile[i]; i+=3){
    if( bRaw ){
      raw_diff_versions(azFile[i+2], azFile[i+1], azFile[i]);
    }else{
      @ <hr>
      cvs_diff_versions(azFile[i+2], azFile[i+1], azFile[i]);
    }
  }

  db_query_free(azFile);

  return 0;
}

/*
** Load the names of all users listed in CVSROOT/passwd into a temporary
** table "tuser".  Load the names of readonly users into a temporary table
** "treadonly".
*/
static void cvs_read_passwd_files(const char *zCvsRoot){
  FILE *f;
  char *zFile;
  char *zPswd, *zSysId;
  int i;
  char zLine[2000];
  

  db_execute(
    "CREATE TEMP TABLE tuser(id UNIQUE ON CONFLICT IGNORE,pswd,sysid,cap);"
    "CREATE TEMP TABLE treadonly(id UNIQUE ON CONFLICT IGNORE);"
  );
  if( zCvsRoot==0 ){
    zCvsRoot = db_config("cvsroot","");
  }
  zFile = mprintf("%s/CVSROOT/passwd", zCvsRoot);
  f = fopen(zFile, "r");
  free(zFile);
  if( f ){
    while( fgets(zLine, sizeof(zLine), f) ){
      remove_newline(zLine);
      for(i=0; zLine[i] && zLine[i]!=':'; i++){}
      if( zLine[i]==0 ) continue;
      zLine[i++] = 0;
      zPswd = &zLine[i];
      while( zLine[i] && zLine[i]!=':' ){ i++; }
      if( zLine[i]==0 ){
        zSysId = zLine;
      }else{
        zLine[i++] = 0;
        zSysId = &zLine[i];
      }
      db_execute("INSERT INTO tuser VALUES('%q','%q','%q','io');",
         zLine, zPswd, zSysId);
    }
    fclose(f);
  }
  zFile = mprintf("%s/CVSROOT/readers", zCvsRoot);
  f = fopen(zFile, "r");
  free(zFile);
  if( f ){
    while( fgets(zLine, sizeof(zLine), f) ){
      remove_newline(zLine);
      db_execute("INSERT INTO treadonly VALUES('%q');", zLine);
    }
    fclose(f);
  }
  zFile = mprintf("%s/CVSROOT/writers", zCvsRoot);
  f = fopen(zFile, "r");
  free(zFile);
  if( f ){
    db_execute("INSERT INTO treadonly SELECT id FROM tuser");
    while( fgets(zLine, sizeof(zLine), f) ){
      remove_newline(zLine);
      db_execute("DELETE FROM treadonly WHERE id='%q';", zLine);
    }
    fclose(f);
  }
  db_execute(
    "UPDATE tuser SET cap='o' WHERE id IN (SELECT id FROM treadonly);"
  );
}

/*
** Read the CVSROOT/passwd, CVSROOT/reader, and CVSROOT/write files.
** record infomation gleaned from those files in the local database.
*/
static int cvs_user_read(void){
  db_add_functions();
  db_execute("BEGIN");
  cvs_read_passwd_files(0);
  db_execute(
    "REPLACE INTO user(id,name,email,passwd,capabilities) "
    "  SELECT n.id, o.name, o.email, n.pswd, "
    "      cap_or(cap_and(o.capabilities, 'aknrsw'), n.cap) "
    "  FROM tuser as n, user as o WHERE n.id=o.id;"
    "INSERT OR IGNORE INTO user(id,name,email,passwd,capabilities) "
    "  SELECT id, id, '', pswd,"
    "      cap_or((SELECT capabilities FROM user WHERE id='anonymous'),cap) "
    "  FROM tuser;"
    "COMMIT;"
  );
  return 0;
}

/*
** Write the CVSROOT/passwd and CVSROOT/writer files based on the current
** state of the database.
*/
static int cvs_user_write(const char *zOmit){
  FILE *pswd;
  FILE *wrtr;
  FILE *rdr;
  char **az;
  int i;
  char *zFile;
  const char *zCvsRoot;
  const char *zUser = 0;
  const char *zWriteEnable;
  struct passwd *pw;

  /* If the "write_cvs_passwd" configuration option exists and is "no"
  ** (or at least begins with an 'n') then disallow writing to the
  ** CVSROOT/passwd file.
  */
  zWriteEnable = db_config("write_cvs_passwd","yes");
  if( zWriteEnable[0]=='n' ) return 1;

  /*
  ** Map the CVSTrac process owner to a real user id. This, presumably,
  ** will be someone with CVS read/write access.
  */
  pw = getpwuid(geteuid());
  if( pw==0 ) pw = getpwuid(getuid());
  if( pw ){
    zUser = mprintf("%s",pw->pw_name);
  }else{
    zUser = db_config("cvs_user_id","nobody");
  }

  zCvsRoot = db_config("cvsroot","");
  cvs_read_passwd_files(zCvsRoot);
  zFile = mprintf("%s/CVSROOT/passwd", zCvsRoot);
  pswd = fopen(zFile, "w");
  free(zFile);
  if( pswd==0 ) return 0;
  zFile = mprintf("%s/CVSROOT/writers", zCvsRoot);
  wrtr = fopen(zFile, "w");
  free(zFile);
  zFile = mprintf("%s/CVSROOT/readers", zCvsRoot);
  rdr = fopen(zFile, "w");
  free(zFile);
  az = db_query(
      "SELECT id, passwd, '%q', capabilities FROM user "
      "UNION ALL "
      "SELECT id, pswd, sysid, cap FROM tuser "
      "  WHERE id NOT IN (SELECT id FROM user) "
      "ORDER BY id", zUser);
  for(i=0; az[i]; i+=4){
    if( strchr(az[i+3],'o')==0 ) continue;
    if( zOmit && strcmp(az[i],zOmit)==0 ) continue;
    fprintf(pswd, "%s:%s:%s\n", az[i], az[i+1], az[i+2]);
    if( strchr(az[i+3],'i')!=0 ){
      if( wrtr!=0 ) fprintf(wrtr,"%s\n", az[i]);
    }else{
      if( rdr!=0 ) fprintf(rdr,"%s\n", az[i]);
    }
  }
  db_query_free(az);
  fclose(pswd);
  if( wrtr ) fclose(wrtr);
  if( rdr ) fclose(rdr);
  return 1;
}


void init_cvs(void){
  g.scm.zSCM = "cvs";
  g.scm.zName = "CVS";
  g.scm.canFilterModules = 1;
  g.scm.pxHistoryUpdate = cvs_history_update;
  g.scm.pxDiffVersions = cvs_diff_versions;
  g.scm.pxDiffChng = cvs_diff_chng;
  g.scm.pxIsFileAvailable = cvs_is_file_available;
  g.scm.pxDumpVersion = cvs_dump_version;
  g.scm.pxUserRead = cvs_user_read;
  g.scm.pxUserWrite = cvs_user_write;
}


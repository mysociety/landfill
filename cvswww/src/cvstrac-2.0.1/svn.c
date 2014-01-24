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
** This file contains code used to call svnlook to get information 
** about repository's history and update the CHNG and FILECHNG 
** tables according to the content of it's output. All the other 
** Subversion-specific stuff should also be found here.
*/
#include "config.h"
#include <time.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <errno.h>
#include "svn.h"

/*
** Instance of following structure holds information about revision
*/
typedef struct Revision Revision;
struct Revision {
  char zAuthor[100];   /* Author name */
  int  nDate;          /* Revision date */
  int  nMsgLength;     /* Number of characters in log message */
  char *zMessage;      /* Log message */
};


/*
** This func is supposed to deal with "svn copy" and update FILECHNG and 
** FILE for each file in copied directory.
*/
static void svn_insert_copied_files(
  int cn,
  int nRev,
  const char *zNewDir, 
  const char *zOldDir,
  int skipInsertFile
){
  int i;
  size_t nLen;
  char **azTree;
  char *zTmp;
  
  /* We need paths without leading '/' here since that is what we 
  ** already have in db.
  */
  while( zNewDir[0]=='/' ) zNewDir++;
  while( zOldDir[0]=='/' ) zOldDir++;
  
  /* zOldDir must end with '/' if it's not empty string.
  */
  zTmp = (strlen(zOldDir)==0) ? mprintf("") : mprintf("%s/", zOldDir);
  if( zTmp==0 ) common_err("Out of memory");
  nLen = strlen(zTmp);
  
  /* This query should return only files that are not deleted.
  */
  azTree = db_query(
    "SELECT fc.filename, fc.chngtype "
    "FROM filechng fc "
    "WHERE fc.filename LIKE '%q%%' AND fc.vers=( "
    "  SELECT MAX(vers) FROM filechng fc2 WHERE fc2.filename=fc.filename "
    ")  AND fc.chngtype<>2",
    zTmp
  );
  free(zTmp);
  
  if( azTree==0 ) return;

  for(i=0; azTree[i]; i+=2){
    char *zFile;
    int nFLen;
    if( strlen(zNewDir)>0 ){
      zFile = mprintf("%s/%s", zNewDir, &(azTree[i])[nLen]);
    }else{
      zFile = mprintf("%s", &(azTree[i])[nLen]);
    }
    if( zFile==0 ) common_err("Out of memory");
    nFLen = strlen(zFile);
    if( nFLen && zFile[nFLen-1]!='/' ){
      db_execute(
        "REPLACE INTO filechng(cn,filename,vers,prevvers,chngtype) "
        "VALUES(%d,'%q',%d,'%s',1);",
        cn, zFile, nRev, ( azTree[i+1][0]=='0' ) ? "" : azTree[i+1]
      );
    }
    if( !skipInsertFile ) insert_file(zFile, cn);
    free(zFile);
  }
  db_query_free(azTree);
}

/*
** This func is supposed to deal with "svn delete" on direcories and update 
** FILECHNG for each file and dir in deleted directory.
*/
static void svn_delete_dir(
  int cn,
  int nRev,
  const char *zDir,
  int skipInsertFile
){
  int i;
  char **azTree;
  
  /* We need paths without leading '/' here since that is what we 
  ** already have in db.
  */
  while( zDir[0]=='/' ) zDir++;
  
  azTree = db_query(
    "SELECT fc.filename, fc.chngtype "
    "FROM filechng fc "
    "WHERE fc.filename LIKE '%q%%' AND fc.vers=( "
    "  SELECT MAX(vers) FROM filechng fc2 WHERE fc2.filename=fc.filename "
    ")  AND fc.chngtype<>2",
    zDir
  );
  
  if( azTree==0 ) return;

  for(i=0; azTree[i]; i+=2){
    int nFLen = strlen(azTree[i]);
    if( nFLen && azTree[i][nFLen-1]!='/' ){
      db_execute(
        "REPLACE INTO filechng(cn,filename,vers,prevvers,chngtype) "
        "VALUES(%d,'%q',%d,'%s',2);",
        cn, azTree[i], nRev, ( azTree[i+1][0]=='0' ) ? "" : azTree[i+1]
      );
    }
    if( !skipInsertFile ) insert_file(azTree[i], cn);
  }

  db_query_free(azTree);
}

/*
** A little hackish way to determine if we even need to call "svnlook youngest".
*/
static int svn_did_repository_change( const char * zRoot, time_t since ){
  struct stat statbuf;
  char *zFilename = mprintf("%s/db/revisions",zRoot); /* bdb */
  if( stat(zFilename, &statbuf) ){
    free(zFilename);
    zFilename = mprintf("%s/db/current",zRoot); /* fsfs */
    if( stat(zFilename, &statbuf) ){
      free(zFilename);
      return 1; /* don't know, say yes */
    }
  }
  free(zFilename);
    
  /* Should run every hour, just in case there was some timing problems... */
  return statbuf.st_mtime > since || (time(NULL)-since)>3600;
}

/*
** Process recent activity in the Subversion repository.
**
** If any errors occur, output an error page and exit.
**
** If the "isReread" flag is set, it means the history file should be
** reread to pick up changes that we may have missed earlier.  
*/
static int svn_history_update(int isReread){
  const char *zRoot;
  char **azResult;
  FILE *in;
  int nRev;         /* Current revison number in loop */
  int cnum = 0;     /* check-in number to use for this checkin */
  int next_cnum;    /* next unused check-in number */
  int nErr = 0;
  char *zCmd;
  char zLine[2000];
  int nBaseRevision = 0;  /* Revision we last seen, and have stored in db */
  int nHeadRevision = 0;  /* Latest revision in repository */
  int nLine;
  int isInitialScan; /* If set delay updating FILE till after the last revision */
  Revision *pRev;

  db_execute("BEGIN");

  /* Get the path to local repository and last revision number we have in db
   * If there's no repository defined, bail and wait until the admin sets one.
  */
  zRoot = db_config("cvsroot","");
  if( zRoot[0]==0 ) return 1;
  nBaseRevision = atoi(db_config("historysize","0"));

  if( nBaseRevision
      && !svn_did_repository_change(zRoot,atoi(db_config("svnlastupdate","0"))) ){
    db_execute("COMMIT");
    return 1;
  }
  
  isInitialScan = (nBaseRevision==0);
  
  /* Get the number of latest revision in repository
  */
  zCmd = mprintf("svnlook youngest '%s' 2>/dev/null", quotable_string(zRoot));
  
  in = popen(zCmd, "r");
  if( in==0 ){
    error_init(&nErr);
    @ <li><p>Unable to execute the following command:
    @ <blockquote><pre>
    @ %h(zCmd)
    @ </pre></blockquote></p></li>
    error_finish(nErr);
    free(zCmd);
    return -1;
  }
  free(zCmd);
  
  if( fgets(zLine, sizeof(zLine), in) ){
    nHeadRevision = atoi(zLine);
  }
  pclose(in);
  
  /* This could mean that repository is empty, no revisions in it yet.
  ** But this could also mean that atoi() failed. And that would be bad since
  ** it means svnlook's output format changed.
  */
  if( nHeadRevision==0 ){
    error_init(&nErr);
    @ <li><p>Repository '%h(zRoot)' appears empty. Latest revision
    @ seems to be '%h(zLine)'</p></li>
    error_finish(nErr);
    return -1;
  }
  
  /* See if there are some new revisions in repository
  */
  if( nHeadRevision==nBaseRevision ) { 
    /* No changes to the repository since our last scan
    ** Exit without doing anything 
    */
    db_execute("UPDATE config SET value=%d WHERE name='svnlastupdate';", time(NULL)+1);
    db_execute("COMMIT");
    return 0;
  }
  
  /* Find the next available change number
  */
  azResult = db_query("SELECT max(cn) FROM chng");
  next_cnum = atoi(azResult[0])+1;
  db_query_free(azResult);
  
  pRev = (Revision *) malloc( sizeof(*pRev) );

  /*
  ** Parse output of svnlook changes and svnlook info to get the info we need
  ** We do this for each revision we miss from db
  */
  for(nRev=nBaseRevision+1; nRev<=nHeadRevision; nRev++){
    int nAddChng = 1;
    int isMsgEnd = 0;
    /* Example output of "svnlook info" is as follows:
    ** 
    ** chorlya                                      <-Author
    ** 2005-08-02 01:51:45 +0200 (Tue, 02 Aug 2005) <-Date
    ** 26                                           <- # of chars in message
    ** Changes to some make files                   <- multiline message
    **                                              <- followed by blank line
    */
    zCmd = mprintf("svnlook info -r %d '%s' 2>/dev/null", 
                   nRev, quotable_string(zRoot));
    in = popen(zCmd, "r");
    if( in==0 ){
      error_init(&nErr);
      @ <li><p>Unable to execute the following command:
      @ <blockquote><pre>
      @ %h(zCmd)
      @ </pre></blockquote></p></li>
      error_finish(nErr);
      free(zCmd);
      return -1;
    }
    free(zCmd);
    
    nLine = 0;
    while( fgets(zLine,sizeof(zLine),in) ){
      if( nLine==0 ){
        remove_newline(zLine);
        bprintf(pRev->zAuthor, sizeof(pRev->zAuthor), "%.90s", zLine);
      }else if( nLine==1 ){
        struct tm sTm;
        
        /* Since timestamp is localtime, we need to subtract timezone offset 
        ** to get timestamp in UTC
        */
        int nOffstHr, nOffstMn; /* Timezone offset hour and minute */

        zLine[25] = 0;
        if( sscanf(zLine,"%d-%d-%d %d:%d:%d %3d%2d",
                    &sTm.tm_year, &sTm.tm_mon, &sTm.tm_mday,
                    &sTm.tm_hour, &sTm.tm_min, &sTm.tm_sec,
                    &nOffstHr, &nOffstMn)==8 ){
          sTm.tm_year -= 1900;
          sTm.tm_mon--;
          /* We subtract our timezone offset from tm.tm_sec since tm_sec 
          ** is just added to rest of the timestamp without any calculations 
          ** being performed on it. Because of that tm_sec can be negative!
          */
          sTm.tm_sec -= (nOffstHr>0) ? (nOffstHr*60 + nOffstMn)*60
                                       : (nOffstHr*60 - nOffstMn)*60 ;
          pRev->nDate = mkgmtime(&sTm);
        }
      }else if( nLine==2 ){
        pRev->nMsgLength = atoi(zLine);
        if( pRev->nMsgLength==0 ){
          isMsgEnd = 1;
          pRev->zMessage = "";
          break; /* No comment here */
        }else{
          /* Allocate storage space for comment.
          */
          pRev->zMessage = (char *) malloc(pRev->nMsgLength+16);
          if( pRev->zMessage==NULL ){
            break; /* malloc() failed */
          }
        }
      }else{
        /* Before we begin concatenating lines of comment make sure we start
        ** with empty string.
        */
        if( nLine==3 ){
          pRev->zMessage[0] = 0;
        }
        
        /* Concat comment lines into one string */
        strcat(pRev->zMessage, zLine);
      }
      if( isMsgEnd ) break;
      nLine++;
    }
    pclose(in);
    
    if( isReread ){
      char *zPrior = db_short_query(
        "SELECT cn FROM chng WHERE date=%d AND user='%q'",
        pRev->nDate, pRev->zAuthor
      );
      if( zPrior==0 || (cnum = atoi(zPrior))<=0 ){
        cnum = next_cnum++;
      }else{
        nAddChng = 0;
      }
      if(zPrior) free(zPrior);
    }else{
      cnum = next_cnum++; 
    }
    
    /* We assume there can't ever be an empty commit, so we just INSERT
    ** TODO: check information read from "svnlook info" for assumed pattern.
    **       If it deviates from it too much, abort.
    */
    if( nAddChng ){
      if( pRev->zMessage==0 ) pRev->zMessage = "";
      db_execute(
        "INSERT INTO chng(cn, date, branch, milestone, user, message) "
        "VALUES(%d,%d,'',0,'%q','%q');",
        cnum, pRev->nDate, pRev->zAuthor, pRev->zMessage
      );
      xref_add_checkin_comment(cnum, pRev->zMessage);
    }
    
    /* Now that we've got the common revision info, that is same for every 
    ** file in this revision, we need to get the list of files that changed 
    ** in this revision and populate filechng table with it.
    ** 
    ** Example output of "svnlook changed" is as follows:
    ** A   trunk/vendors/deli/
    ** A   trunk/vendors/deli/chips.txt
    ** A   trunk/vendors/deli/sandwich.txt
    ** A   trunk/vendors/deli/pickle.txt
    ** 
    ** First column is svn update-style status letter.
    ** Then there are 3 columns that are of no intrest to us, 
    ** and then comes the file path.
    */
    zCmd = mprintf("svnlook changed -r %d '%s' 2>/dev/null", 
                   nRev, quotable_string(zRoot));
    in = popen(zCmd, "r");
    if( in==0 ){
      error_init(&nErr);
      @ <li><p>Unable to execute the following command:
      @ <blockquote><pre>
      @ %h(zCmd)
      @ </pre></blockquote></p></li>
      error_finish(nErr);
      free(zCmd);
      return -1;
    }
    free(zCmd);
    
    while( fgets(zLine,sizeof(zLine),in) && zLine[0]!='\n'  && zLine[0]!='\r' ){
      int nChngType = 0;
      const char *zFilename = &zLine[4];
      char *zPrevVersion;
      size_t nPos, nLen = strlen(zLine);
      if( nLen<4 ) continue;

      /*
      ** First char indicates what happend to file/dir in this revision.
      ** Following table shows the meaning of some expected values for first 
      ** char and how we map that to filechng field in database:
      ** 
      ** Char  Meaning                    chngtype
      ** =========================================
      **  U    File updated (modified)       0
      **  A    File added to repository      1
      **  D    File deleted (removed)        2
      **
      ** First strip CR and LF chars from right end of filename
      */
      remove_newline(&zLine[4]);

      if( zLine[0]=='U' ){
        nChngType = 0;
      }else if( zLine[0]=='D' ){
        nChngType = 2;
        /* If dir was deleted we need to delete all files and dirs in it.
        */
        nPos = strlen(zFilename)-1;
        if( zFilename[nPos]=='/' ){
          svn_delete_dir(cnum, nRev, zFilename, isInitialScan);
        }
      }else if( zLine[0]=='A' ){
        /* If this is the first revision there can't be any copies in
        ** repository yet.
        */
        if( nRev>1 ){
          /* TODO: check to make sure dir is actually a copy before calling
          ** svn_insert_copied_files() since this func is pretty slow.
          */
          nPos = strlen(zFilename)-1;
          if( zFilename[nPos]=='/' ){
            
            FILE *history;
            char zHistLine[2000];
            char *zCurrPath = NULL;
            int i=0, nHistErr=0;
            zCmd = mprintf("svnlook history -r %d '%s' '%s' 2>/dev/null", nRev, 
                           quotable_string(zRoot), quotable_string(zFilename));
            history = popen(zCmd, "r");
            if( history==0 ){
              error_init(&nErr);
              @ <li><p>Unable to execute the following command:
              @ <blockquote><pre>
              @ %h(zCmd)
              @ </pre></blockquote></p></li>
              error_finish(nErr);
              free(zCmd);
              return -1;
            }
            free(zCmd);
            
            while( fgets(zHistLine,sizeof(zHistLine),history) && i<4 && !nHistErr ){
              remove_newline(zHistLine);
              switch( i ){
                case 0:
                  if( strncmp(zHistLine, "REVISION   PATH", 15)!=0 ) nHistErr++;
                  break;
                case 1:
                  if( strncmp(zHistLine, "--------   ----", 15)!=0 ) nHistErr++;
                  break;
                case 2:
                  /* We need to store this just so we can compare it
                  ** with next line.
                  */
                  zCurrPath = mprintf("%s", &zHistLine[11]);
                  break;
                case 3:
                  /* If paths changed, this dir was copied.
                  */
                  if( strcmp(zCurrPath, &zHistLine[11])!=0 ){
                    svn_insert_copied_files(cnum, nRev, zCurrPath, 
                      &zHistLine[11], isInitialScan);
                  }
                  break;
              }
              i++;
            }
            pclose(history);
            if( zCurrPath ) free(zCurrPath);
          }
        }
        nChngType = 1;
      }else{
        nChngType = -1; /* TODO: do something smart(tm) here :D */
        continue;
      }
      
      zPrevVersion = db_short_query(
        "SELECT vers FROM filechng WHERE filename='%q' AND cn<%d "
        "ORDER BY cn DESC;",
        zFilename, cnum
      );
      
      db_execute(
        "REPLACE INTO filechng(cn,filename,vers,prevvers,chngtype) "
        "VALUES(%d,'%q',%d,'%q',%d);",
        cnum, zFilename, nRev, zPrevVersion ? zPrevVersion : "", nChngType
      );

      if(zPrevVersion) free(zPrevVersion);
      if( !isInitialScan ) insert_file(zFilename, cnum);
    }
    pclose(in);
  }
  
  free(pRev);
  
  /* We delayed populating FILE till now on initial scan */
  if( isInitialScan ){
    update_file_table_with_lastcn();
  }
  
  /* Update the "historysize" entry so that we know last revision number that 
  ** we have in db, and "svnlastupdate" to keep those calls to svnlook youngest
  ** to minimum.
  */
  db_execute("UPDATE config SET value=%d WHERE name='historysize';", nHeadRevision);
  db_execute("UPDATE config SET value=%d WHERE name='svnlastupdate';", time(NULL)+1);
  db_config(0,0);
  
  /* Commit all changes to the database
  */
  db_execute("COMMIT;");
  error_finish(nErr);
  return nErr ? -1 : 0;
}

/*
** Diff two versions of a file, handling all exceptions.
**
** If oldVersion is NULL or "0", then this function will output the
** text of version newVersion of the file instead of doing a diff.
*/
static int svn_diff_versions(
  const char *oldVersion,
  const char *newVersion,
  const char *zFile
){
  const char *zTemplate;
  char *zCmd;
  FILE *in;
  int i;
  const char *azSubst[16];
  char zLine[2][2000];
  int nBuf, inFile;
  char *z;
  long long nLine;
  int file_cat; /* set when we want "svnlook cat" instead of "svnlook diff" */
  
  if( zFile==0 ) return -1;
    
  /* If this is the first time file is checked into repository (Added), 
  ** don't diff it just display it's contents
  */
  z = mprintf("%s", newVersion);
  previous_version(z, zFile);
  if( z==0 || z[0]==0 )
    file_cat = 1; /* svnlook cat */
  else
    file_cat = 0; /* svnlook diff */
  
  free(z);
  
  /* Find the command used to compute the file difference.
  */
  azSubst[0] = "F";
  azSubst[1] = zFile;
  azSubst[2] = "V1";
  azSubst[3] = oldVersion;
  azSubst[4] = "V2";
  azSubst[5] = newVersion;
  azSubst[6] = "RP";
  azSubst[7] = db_config("cvsroot", "");
  azSubst[8] = "V";
  azSubst[9] = newVersion;
  azSubst[10] = 0;
  if( file_cat==1 ){
    zTemplate = db_config("filelist","svnlook cat -r '%V' '%RP' '%F' 2>/dev/null");
  }else{
    zTemplate = db_config("filediff","svnlook diff -r '%V2' '%RP' 2>/dev/null");
  }
  zCmd = subst(zTemplate, azSubst);
  in = popen(zCmd, "r");
  free(zCmd);
  if( in==0 ) return -1;
  if( file_cat==0 ){
    /* We're looking for something like this:
    **
    ** Modified: trunk/vendors/deli/sandwich.txt
    ** ==============================================================================
    **
    ** So first we look for a delimiter line since it should be more uniqe 
    ** then one before it. When we find it, we go one line back and check
    ** if that is the file we're after. That line can begin with any element
    ** of zMarker.
    */
    const char zMarker[3][11] = { "Modified: ", "Added: ", "Deleted: " };
    int nMarkerLen[3] = { 10, 7, 9 };
    nBuf = nLine = inFile = 0;
    @ <pre>
    while( fgets(zLine[nBuf], sizeof(zLine[nBuf]), in) ){
      if( strncmp(zLine[nBuf], "===================================================================", 67)==0
          && (zLine[nBuf][67]=='\n' || zLine[nBuf][67]=='\r')
      ){
        if( inFile ){
          /* This is the begging of some other file, and end of ours */
          break;
        } else {
          /* If previous line begins with one of our markers, 
          ** it could be what we're looking for
          */
          for(i=0; i<sizeof(nMarkerLen)/sizeof(nMarkerLen[0]); i++)
            if( strncmp(zLine[(nBuf+1)%2], zMarker[i], nMarkerLen[i])==0 )
              break;
          
          if( i<sizeof(nMarkerLen)/sizeof(nMarkerLen[0]) ){
            /* We found one of the markers on previous line! Now we need to 
            ** check if our filename is present, which would mean we found 
            ** our file and can strat sending it to browser
            */
            int iLen = strlen(zFile);
            if( strncmp(&zLine[(nBuf+1)%2][nMarkerLen[i]], zFile, iLen)==0 
                && (zLine[(nBuf+1)%2][nMarkerLen[i]+iLen]=='\n' 
                    || zLine[(nBuf+1)%2][nMarkerLen[i]+iLen]=='\r')
            ){
              inFile = 1;
            }
          }
        }
      }
      
      if( inFile ){
        /* We can't print current line yet since it may be end of our 
        ** file in diff output, so we print previous line here
        */
        cgi_printf("%h", zLine[(nBuf+1)%2]);
      }
      nBuf = ++nLine % 2;
    }
    @ </pre>
  } else {
    output_pipe_as_html(in,1);
    pclose(in);
  }
  
  return 0;
}

static int svn_dump_version(const char *zVersion, const char *zFile,int bRaw){
  char *zCmd;
  int rc = -1;
  
  if( !zVersion || !zVersion[0] ){
    zCmd = mprintf("svnlook cat '%s' '%s' 2>/dev/null", 
      db_config("cvsroot", ""), quotable_string(zFile) );
  }else{
    zCmd = mprintf("svnlook cat -r '%s' '%s' '%s' 2>/dev/null", 
      quotable_string(zVersion), db_config("cvsroot", ""), quotable_string(zFile) );
  }
  
  rc = common_dumpfile( zCmd, zVersion, zFile, bRaw );
  free(zCmd);

  return rc;
}

static int svn_diff_chng(int cn, int bRaw){
  const char *zRoot;
  char *zRev;
  char *zCmd;
  char zLine[2000];
  FILE *in;
  
  zRev = db_short_query("SELECT vers FROM filechng WHERE cn=%d", cn);
  if( !zRev || !zRev[0] ) return -1; /* Invalid check-in number */
  
  zRoot = db_config("cvsroot", "");
  zCmd = mprintf("svnlook diff -r '%s' '%s' 2>/dev/null",
    quotable_string(zRev), quotable_string(zRoot));
  free(zRev);
  
  in = popen(zCmd, "r");
  free(zCmd);
  if( in==0 ) return -1;
  
  if( bRaw ){
    while( !feof(in) ){
      int amt = fread(zLine, 1, sizeof(zLine), in);
      if( amt<=0 ) break;
      cgi_append_content(zLine, amt);
    }
  }else{
    output_pipe_as_html(in,1);
  }
  pclose(in);
  
  return 0;
}

/*
** svntrac can import users only from svnserver's users file. If you use some 
** other authentication method you'll have to enter users manually or import 
** them in some other way.
*/

/*
** Load the names of all users listed in users file into a temporary
** table "tuser". First locate the users file and figureout what permisions
** have authenticated users.
*/
static void svn_read_users_file(const char *zSvnRoot){
  FILE *f;
  int i, bInSection=0;
  char *zFile, *zKey, *zValue;
  char zLine[2000];
  char *zAnonAccess, *zAuthAccess;
  /* We set these to Subversion defaults in case they are not defined 
  ** explicitly in .conf file.
  */
  zAnonAccess = "o"; /* TODO: use this to set anon user's perms */
  zAuthAccess = "io";
  
  db_execute(
    "CREATE TEMP TABLE tuser(id UNIQUE ON CONFLICT IGNORE,pswd,sysid,cap);"
  );
  if( zSvnRoot==0 ){
    zSvnRoot = db_config("cvsroot","");
  }
  zFile = mprintf("%s/conf/svnserve.conf", zSvnRoot);
  f = fopen(zFile, "r");
  free(zFile);
  /*
  ** This is how Subversion .conf file should look like:
  ** 
  ** # This is commnet
  ** [general] # Section name
  ** anon-access = read # What can anon users do
  ** auth-access = write # What can auth users do
  ** password-db = passwd # Absolute or relative location of users file
  ** realm = My First Repository # This is of no intrest to us
  ** [some_other_section]
  */
  if( f ){
    while( fgets(zLine, sizeof(zLine), f) ){
      remove_newline(zLine);
      for(i=0; zLine[i] && isspace(zLine[i]); i++){}
      if( zLine[i]==0 || zLine[i]=='#' ) continue;
      if( zLine[i]=='[' ){
        /* If we are already in section, then this marks the end of our section.
        ** If not, this might be the start of our section.
        */
        if( bInSection ){
          bInSection = 0;
          break;
        } else if( strncmp(&zLine[i], "[general]", 9)==0 ){
          bInSection = 1;
          continue;
        }
      }
      
      /* If we're not in our section there is nothing to look for in this line.
      ** TODO: I made an assumption here that all keys for key-value pairs in 
      ** .conf files have to start with alnum char. Check if this really 
      ** is the case.
      */
      if( !bInSection || !isalnum(zLine[i]) ) continue;
      
      /* Extract key-value pair */
      zKey = &zLine[i];
      for(; zLine[i] && !isspace(zLine[i]); i++){}
      zLine[i++] = 0;
      for(; zLine[i] && isspace(zLine[i]); i++){}
      if( zLine[i++]!='=' ) continue; /* Not valid key-value pair */
      for(; zLine[i] && isspace(zLine[i]); i++){}
      zValue = &zLine[i];
      for(; zLine[i] && !isspace(zLine[i]); i++){}
      zLine[i] = 0; /* Make sure we rtrim our value */
      
      /* We handel each key differently */
      if( strcmp(zKey, "anon-access")==0 ){
        if( strcmp(zValue, "none")==0 ){ zAnonAccess=""; continue; }
        if( strcmp(zValue, "read")==0 ){ zAnonAccess="o"; continue; }
        if( strcmp(zValue, "write")==0 ){ zAnonAccess="io"; continue; }
      }
      
      if( strcmp(zKey, "auth-access")==0 ){
        if( strcmp(zValue, "write")==0 ){ zAuthAccess="io"; continue; }
        if( strcmp(zValue, "read")==0 ){ zAuthAccess="o"; continue; }
        if( strcmp(zValue, "none")==0 ){ zAuthAccess=""; continue; }
      }
      
      if( strcmp(zKey, "password-db")==0 ){
        if( zValue[0]=='/' ){
          /* We've got absolute path, no problem here */
          zFile = mprintf("%s", zValue);
          continue;
        } else {
          /* We've got relative path */
          zFile = mprintf("%s/conf/%s", zSvnRoot, zValue);
          continue;
        }
      }
    }
    fclose(f);
  }
  
  /* If we didn't find path to users file, exit */
  if( !zFile || !zFile[0] ) return;
  
  f = fopen(zFile, "r");
  free(zFile);
  
  /* Read username->password pairs from file and store them in db
  ** Passwords are stored in clear text.
  */
  if( f ){
    bInSection = 0;
    while( fgets(zLine, sizeof(zLine), f) ){
      char zBuf[3];
      char zSeed[100];
      const char *z;
      remove_newline(zLine);
      for(i=0; zLine[i] && isspace(zLine[i]); i++){}
      if( zLine[i]==0 || zLine[i]=='#' ) continue;
      if( zLine[i]=='[' ){
        /* If we are already in section, then this marks the end of our section.
        ** If not, this might be the start of our section.
        */
        if( bInSection ){
          bInSection = 0;
          break;
        } else if( strncmp(&zLine[i], "[users]", 7)==0 ){
          bInSection = 1;
          continue;
        }
      }
      
      /* If we're not in our section there is nothing to look for in this line.
      ** TODO: I made an assumption here that all usernames have to start 
      ** with alnum char. Check if this really is the case.
      */
      if( !bInSection || !isalnum(zLine[i]) ) continue;
      
      /* Each line here is key-value pair, or in our case uname-passwd pair */
      zKey = &zLine[i];
      for(; zLine[i] && !isspace(zLine[i]); i++){}
      zLine[i++] = 0;
      for(; zLine[i] && isspace(zLine[i]); i++){}
      if( zLine[i++]!='=' ) continue; /* Not valid key-value pair */
      for(; zLine[i] && isspace(zLine[i]); i++){}
      zValue = &zLine[i];
      for(; zLine[i] && !isspace(zLine[i]); i++){}
      zLine[i] = 0; /* Make sure we rtrim our value */
      
      /* We need to encrypt passwords */
      bprintf(zSeed,sizeof(zSeed),"%d%.20s",getpid(),zKey);
      z = crypt(zSeed, "aa");
      zBuf[0] = z[2];
      zBuf[1] = z[3];
      zBuf[2] = 0;
          
      db_execute("INSERT INTO tuser VALUES('%q','%q','','%s');",
         zKey, crypt(zValue, zBuf), zAuthAccess);
    }
    fclose(f);
  }
}

/*
** Read svnserve users file and record infomation gleaned from that file
** in the local database.
*/
static int svn_user_read(void){
  db_add_functions();
  db_execute("BEGIN");
  svn_read_users_file(0);

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

void init_svn(void){
  g.scm.zSCM = "svn";
  g.scm.zName = "Subversion";
  g.scm.pxHistoryUpdate = svn_history_update;
  g.scm.pxDiffVersions = svn_diff_versions;
  g.scm.pxDiffChng = svn_diff_chng;
  g.scm.pxIsFileAvailable = 0;  /* use the database */
  g.scm.pxDumpVersion = svn_dump_version;
  g.scm.pxUserRead = svn_user_read;
}


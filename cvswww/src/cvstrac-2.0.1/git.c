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
#include <dirent.h>
#include <errno.h>
#include <limits.h>  /* for PATH_MAX */
#include "git.h"

static void err_pipe(const char* zMsg,const char* zCmd){
  int nErr = 0;
  error_init(&nErr);
  @ <b>%h(zMsg)</b>
  @ <li><p>Unable to execute the following command:
  @ <blockquote><pre>
  @ %h(zCmd)
  @ </pre></blockquote></p></li>
  @ <b>%h(strerror(errno))</b>
  error_finish(nErr);
}

static int next_cnum(){
  char *zResult = db_short_query("SELECT max(cn) FROM chng");
  int next_cnum = zResult ? atoi(zResult)+1 : 0;
  if(zResult) free(zResult);
  return next_cnum;
}

/*
** If *nDate==0, it's usually because the commit wasn't correctly read. A NULL
** return code just means that the commit could be the root object.
*/
static char **git_read_commit(
  const char *zGitDir,
  const char *zObject, /* git sha1 object */
  char *zAuthor,       /* at least 100 bytes */
  int *nDate,
  char *zComment,
  int nMaxComment
){
  char *zCmd;
  FILE *in;
  char zLine[PATH_MAX*2];
  int bInMsg = 0;
  char **azParents = 0;
  int nParent = 0;
  int nMaxParent = 0;
  int nComment = 0;
  char zCommitter[100];

  assert(nDate);

  zCmd = mprintf("GIT_DIR='%s' git-cat-file commit '%s' 2>&1",
                 zGitDir, zObject);
  if( zCmd==0 ) return 0;

  in = popen(zCmd, "r");
  if( in==0 ){
    err_pipe("Reading commit",zCmd);
    free(zCmd);
    return 0;
  }
  free(zCmd);

  if( zAuthor ) zAuthor[0] = 0;
  if( zComment ) zComment[0] = 0;
  zCommitter[0] = 0;
  *nDate = 0;

  while( !feof(in) && !ferror(in) ){
    if( 0==fgets(zLine,sizeof(zLine),in) ) break;

    /* you'll get this if it was some other kind of object */
    if( !strncmp(zLine,"error:",6) ) break;

    if( bInMsg==0 ){
      if( zLine[0]=='\n' ){
        bInMsg = 1;
        if( zComment==0 ) break;
      }else if( 0==strncmp(zLine,"parent ",7) ){
        char zParent[100];
        if( nParent+2 >= nMaxParent ){
          nMaxParent = (nParent+2) * 2;
          azParents = realloc(azParents, sizeof(char*)*nMaxParent);
          if( azParents==0 ) common_err("%s",strerror(errno));
        }

        sscanf(&zLine[7],"%50[0-9a-fA-F]",zParent);
        azParents[nParent++] = strdup(zParent);
        azParents[nParent] = 0;

      }else if( zAuthor!= 0 && 0==strncmp(zLine,"author ",7) ){
        sscanf(&zLine[7],"%90[^<]%*[^>]>",zAuthor);
      }else if( 0==strncmp(zLine,"committer ",10) ){
        sscanf(&zLine[10],"%90[^<]%*[^>]> %d",zCommitter,nDate);
      }
    }else{
      int len = strlen(zLine);
      if( len+nComment >= nMaxComment ) break;
      strcpy(&zComment[nComment], zLine);
      nComment += len;
    }
  }
  pclose(in);

  if( *nDate==0 ){
    if( azParents ) db_query_free(azParents);
    return NULL;
  }

  if( zComment && zComment[0]==0 && bInMsg ){
    strncpy(zComment,"Empty log message",nMaxComment);
    nComment = strlen(zComment);
  }

  if( zCommitter[0] ){
    char *zMsg = mprintf( "\n\nCommitter: %s", zCommitter);
    int len = strlen(zLine);
    if( len+nComment < nMaxComment ){
      strcpy(&zComment[nComment], zMsg);
      nComment += len;
    }
    if( zAuthor!=0 && zAuthor[0]==0 ){
      /* apparently GIT commits don't always have an author */
      strcpy(zAuthor, zCommitter);
    }
  }

  return azParents;
}

static void git_ingest_commit_chng(
  const char *zGitDir,
  int cn,
  const char *zCommit,
  time_t nDate,
  const char *zAuthor,
  const char *zComment,
  const char *zPrevVers,
  int skipInsertFile
){
  FILE *in = 0;
  char zLine[PATH_MAX*3];

  if( zPrevVers[0]==0 ){
    /* Initial commit, hence no parent(s) to compare against. That means just a
    ** straight tree list
    */

    char *zCmd = mprintf("GIT_DIR='%s' git-ls-tree -r '%s'", zGitDir, zCommit);
    in = popen(zCmd,"r");
    if( in==0 ){
      err_pipe("Reading tree",zCmd);
      return;
    }
    free(zCmd);

    while( !feof(in) && !ferror(in) ){
      char zMode[100], zType[100], zObject[100], zPath[PATH_MAX];

      if( 0==fgets(zLine,sizeof(zLine),in) ) break;
      remove_newline(zLine);

      sscanf(zLine, "%8[0-9] %90s %50[0-9a-fA-F] %[^\t]",
             zMode, zType, zObject, zPath);

      if( !strcmp(zType,"blob") ){
        int nIns = 0;
        int nDel = 0;

        db_execute(
          "INSERT INTO filechng(cn,filename,vers,prevvers,chngtype,nins,ndel) "
          "VALUES(%d,'%q','%s','',1,%d,%d)",
          cn, zPath, zCommit, nIns, nDel);
        if( !skipInsertFile ) insert_file(zPath, cn);
      }
    }
  }else{
    /* Now get the list of changed files and turn them into FILE
    ** and FILECHNG records.  git-diff-tree is disgustingly PERFECT for
    ** this. Compared to the hassles one has to go through with CVS or
    ** Subversion to find out what's in a change tree, it's just mind
    ** blowing how ideal this is.  FIXME: we're not handling renames or
    ** copies right now. When/if we do, add in the "-C -M" flags.
    */

    char *zCmd = mprintf("GIT_DIR='%s' git-diff-tree -r -t '%s' '%s'",
                         zGitDir, zPrevVers, zCommit);
    in = popen(zCmd,"r");
    if( in==0 ){
      err_pipe("Reading tree",zCmd);
      return;
    }
    free(zCmd);

    while( !feof(in) && !ferror(in) ){
      char zSrcMode[100], zDstMode[100], zSrcObject[100], zDstObject[100];
      char cStatus, zPath[PATH_MAX];

      if( 0==fgets(zLine,sizeof(zLine),in) ) break;
      remove_newline(zLine);

      sscanf(zLine, "%*c%8s %8s %50[0-9a-fA-F] %50[0-9a-fA-F] %c %[^\t]",
             zSrcMode, zDstMode, zSrcObject, zDstObject, &cStatus, zPath);

      if( zSrcMode[1]=='0' || zDstMode[1]=='0' ){
        int nIns = 0;
        int nDel = 0;

        if( cStatus=='N' || cStatus=='A' ){
          if( !skipInsertFile ) insert_file(zPath, cn);
          db_execute(
            "INSERT INTO "
            "       filechng(cn,filename,vers,prevvers,chngtype,nins,ndel) "
            "VALUES(%d,'%q','%s','',1,%d,%d)",
            cn, zPath, zCommit, nIns, nDel);
        }else if( cStatus=='D' ){
          db_execute(
            "INSERT INTO "
            "       filechng(cn,filename,vers,prevvers,chngtype,nins,ndel) "
            "VALUES(%d,'%q','%s','%s',2,%d,%d)",
            cn, zPath, zCommit, zPrevVers, nIns, nDel);
        }else{
          db_execute(
            "INSERT INTO "
            "       filechng(cn,filename,vers,prevvers,chngtype,nins,ndel) "
            "VALUES(%d,'%q','%s','%s',0,%d,%d)",
            cn, zPath, zCommit, zPrevVers, nIns, nDel);
        }
      }
    }
  }
  assert(in);
  pclose(in);

  db_execute(
    "INSERT INTO chng(cn, date, branch, milestone, user, message) "
    "VALUES(%d,%d,'',0,'%q','%q')",
    cn, nDate, zAuthor, zComment
  );
  xref_add_checkin_comment(cn, zComment);
}

/*
** Read in any commits in the tree into the ci table. To sanely deal with
** multi-parent merges, this may be a recursive function. Returns the
** number of _new_ commits.
*/
static int git_ingest_commit_tree(const char *zGitDir, const char *zCommit){
  int i;
  char **azParents = 0;
  char zCur[50];
  int nNew = 0;

  strncpy(zCur,zCommit,sizeof(zCur));
  
  while( zCur[0]!=0 ){
    if( db_exists("SELECT 1 FROM filechng WHERE vers='%s' "
                  "UNION ALL "
                  "SELECT 1 FROM ci WHERE vers='%s'", zCur, zCur)){
      /* Seen this already, or it's already one of the commits we're going
      ** to ingest.
      */
      break;
    }
    
    {
      /* Read the commit in a different scope so all the large static
      ** buffers aren't holding stack space when we recurse.
      */
      char zComment[10000];
      char zAuthor[100];
      int nDate = 0;

      azParents = git_read_commit(zGitDir,zCur,zAuthor,&nDate,
                                  zComment,sizeof(zComment));
      if( nDate==0 ) break;

      db_execute("INSERT INTO ci(vers,date,author,message,prevvers) "
                 "VALUES('%s',%d,'%q','%q','%s');",
                 zCur, nDate, zAuthor, zComment, azParents ? azParents[0] : "");

      nNew ++;
    }

    /* we'll want to break out if we're at a root object */
    zCur[0] = 0;
    if( azParents && azParents[0] ){
      /* If there's more than one parent, recurse on the extras. Otherwise,
      ** just update our "current" counter. This minimizes actual recursions
      ** so multi-parent commits don't end up blowing our stack.
      */
      for(i=1; azParents[i]; i++){
        nNew += git_ingest_commit_tree(zGitDir,azParents[i]);
      }

      strncpy(zCur,azParents[0],sizeof(zCur));

      db_query_free(azParents);
    }
  }

  return nNew;
}

/*
** Read in the git references of zType (either "heads" or "tags") and turn them
** into new CHNG records. If bTags is non-zero, also generate/update milestones
** for the references (i.e. for tags rather than heads).
*/
static int git_read_refs(const char *zGitDir,const char *zType){
  DIR *dir;
  struct dirent *ent;
  int nCommits = 0;

  char *zFile = mprintf("%s/refs/%s", zGitDir, zType);
  dir = opendir( zFile );
  free(zFile);
  if( dir==NULL ) return 0;

  while( 0!=(ent=readdir(dir)) ){
    char zObject[100];
    char *zContents;
    char *zFile;
    char **azRef;
    struct stat statbuf;
    int cn = 0;

    if( ent->d_name[0]=='.' ) continue;

    zFile = mprintf("%s/refs/%s/%s", zGitDir, zType, ent->d_name);
    if( zFile==0 ) continue;

    if( stat(zFile, &statbuf) ){
      /* Can't read the file, skip */
      free(zFile);
      continue;
    }

    azRef = db_query("SELECT object,cn,seen FROM %s WHERE name='%q'",
                     zType, ent->d_name);

    if( azRef && azRef[0] && azRef[1] && azRef[2] ){
      if( statbuf.st_mtime<=atoi(azRef[2]) ){
        /* file hasn't been modified since last time we looked at it */
        db_query_free(azRef);
        free(zFile);
        continue;
      }
      cn = atoi(azRef[1]);  /* don't need to lose this... */
    }

    zContents = common_readfile( zFile );
    free(zFile);
    if(zContents==0) continue;
    zObject[0] = 0;
    sscanf(zContents,"%50[0-9a-fA-F]",zObject);
    free(zContents);
    if( zObject[0]==0 ) continue;

    /* update the seen time... We'll be updating cn later, after we actually
    ** read any new stuff
    */
    db_execute("REPLACE INTO %s(name,cn,object,seen) VALUES('%q',%d,'%q',%d)",
               zType, ent->d_name, cn, zObject, statbuf.st_mtime);

    if( azRef && azRef[0] && !strcmp(zObject,azRef[0]) ){
      /* contents of the ref haven't changed */
      db_query_free(azRef);
      continue;
    }

    /* Fill out the temporary ci table with any changes. */
    nCommits += git_ingest_commit_tree(zGitDir,zObject);

    db_query_free(azRef);
  }
  closedir(dir);

  return nCommits;
}

static void git_update_refs(const char* zType){
  int i;
  char **azRefs;

  /* assumes a logical ordering for names... There's no other way to really do
  ** this, unfortunately.
  */
  azRefs = db_query("SELECT name,object,seen,cn FROM %s ORDER by name", zType);

  for( i=0; azRefs[i]; i+=4 ){
    const char *zName = azRefs[i];
    const char *zObject = azRefs[i+1];
    time_t nSeen = atoi(azRefs[i+2]);
    int cn = atoi(azRefs[i+3]);
    int chngcn = 0;
    char **azChng;
    char *zCn;

    /* Find the CHNG record for the commit. This isn't as nice as we'd like
    ** because CHNG records, being designed for CVS, don't actually store
    ** version numbers and, unlike Subversion, a cn doesn't directly map
    ** to a revision number. So we need to grab the cn from whatever FILECHNG
    ** record corresponds. Note that we _can_ do this in a single query, but
    ** it's quite, quite slow. Probably faster in SQLite 3...
    */

    zCn = db_short_query(
        "SELECT cn FROM filechng WHERE vers='%s' LIMIT 1", zObject);

    if( zCn==0 ) continue;
    chngcn = atoi(zCn);
    free(zCn);
    if( chngcn==0 ) continue;

    azChng = db_query( "SELECT date, user FROM chng WHERE cn=%d", chngcn );

    /* a FILECHNG without a corresponding CHNG? I think not... */
    assert(azChng);

    if( cn==0 ) cn = next_cnum();

    /* Create/update a milestone. The message text basically contains
    ** some information about the type of ref, the name, the commit object,
    ** and, most importantly, a reference to the actual CHNG which, at
    ** display time, should turn into a hyperlink.  Note that in practice,
    ** the milestone will appear next to the commit in the timeline. But
    ** it serves as the only way to document that the commit itself is
    ** somehow special. At some point we should be able to add some concept
    ** of tag browsing.
    */
    db_execute(
      "REPLACE INTO chng(cn,date,branch,milestone,user,directory,message) "
      "VALUES(%d,%d,'',2,'%q','','%q (%s, commit [%d], object %q)')",
      cn, atoi(azChng[0]), azChng[1], zName, zType, chngcn, zObject
    );

    db_execute(
      "REPLACE INTO %s(name,cn,object,seen) VALUES('%q',%d,'%s',%d)",
      zType, zName, cn, zObject, nSeen
    );
    db_query_free( azChng );
  }
  db_query_free(azRefs);
}

/*
** Process recent activity in the GIT repository.
**
** If any errors occur, output an error page and exit.
**
** If the "isReread" flag is set, it means that the history file is being
** reread to pick up changes that we may have missed earlier.  
*/
static int git_history_update(int isReread){
  const char *zRoot;
  char **azResult;
  int cn;
  int nOldSize = 0;
  int i;
  int nNewRevs;

  db_execute("BEGIN");

  /* Get the path to local repository and last revision number we have in db
   * If there's no repository defined, bail and wait until the admin sets one.
  */
  zRoot = db_config("cvsroot","");
  if( zRoot[0]==0 ) return 0;

  nOldSize = atoi(db_config("historysize","0"));

  /* When the historysize is zero, it means we're in for a fresh start or we've
  ** restarted at the beginning. In either case, we go with an empty HEADS
  ** and TAGS tables.
  */
  if( nOldSize==0 ){
    sqlite3 *pDb = db_open();
    char *zErrMsg = 0;
    char *zHead;

    zHead = mprintf("%s/HEAD");
    if( !access(zHead,R_OK) ){
      /* no HEAD in the project directory means it's probably _not_ a GIT
      ** repository.
      */
      int nErr = 0;
      error_init(&nErr);
      @ <b>Error</b>
      @ <li><p>Unable to locate:
      @ <blockquote><pre>
      @ %h(zHead)
      @ </pre></blockquote></p></li>
      @ Are you sure this is a GIT repository?
      error_finish(nErr);
      db_execute("COMMIT;");
      return -1;
    }

    /* If it doesn't succeed, hope it's just a "already exists" error because we
    ** don't seem to have return codes accurate enough to determine if the table
    ** add failed. If the table already exists, it _will_ fail.
    */
    sqlite3_exec(pDb,"CREATE TABLE "
                     "heads(name text primary key, "
                     "      object text,cn,seen,UNIQUE(name));",
                     0, 0, &zErrMsg);

    sqlite3_exec(pDb, "CREATE TABLE "
                      "tags(name text primary key, "
                      "     object text,cn,seen,UNIQUE(name));",
                      0, 0, &zErrMsg);

    /* Make sure they're empty. We're starting fresh */
    db_execute("DELETE FROM heads; DELETE from tags");
  }

  /* git has multiple "heads", each representing a different
  ** branch. Changes may occur in any of them and it's most efficient
  ** just to check each one separately for new commits, _then_ to combine
  ** everything into one merged linear sequence
  */

  db_execute( "CREATE TEMP TABLE ci(vers,date,author,message,prevvers);");

  nNewRevs = git_read_refs(zRoot,"heads");
  nNewRevs += git_read_refs(zRoot,"tags");

  if( nNewRevs==0 ) {
    git_update_refs("heads");
    git_update_refs("tags");
    db_execute("COMMIT");
    return 0;
  }

  /* That filled the ci table, but we dont't actually generate any CHNG
  ** or FILECHNG entries because walking the revision tree from multiple
  ** leaf nodes isn't going to give us a nice ordering. In fact, the most
  ** recent changes would have lower change numbers than the oldest, among
  ** other things.
  **
  ** Now we turn each revision into a list of files and generate the CHNG,
  ** FILE and FILECHNG records
  */

  azResult = db_query("SELECT vers,date,author,message,prevvers "
                      "FROM ci ORDER BY date");
  assert(azResult);
  for(cn=next_cnum(), i=0; azResult[i]; cn++, i+=5){
    git_ingest_commit_chng(zRoot, cn, azResult[i], atoi(azResult[i+1]),
          azResult[i+2], azResult[i+3], azResult[i+4], (nOldSize==0)?1:0);
  }
  db_query_free(azResult);

  /* We couldn't do this before since GIT tags are basically milestones
  ** that point at other CHNG entries and we may not have had all the CHNG
  ** records.  We do heads here too. What this means is that each head is
  ** basically a moving milestone. Not sure how desirable this really is.
  */
  git_update_refs("heads");
  git_update_refs("tags");
  
  /*
  ** Update the "historysize" entry. For GIT, it only matters that it's
  ** non-zero except when we need to re-read the database.
  */
  db_execute("UPDATE config SET value=%d WHERE name='historysize';",
      nOldSize + nNewRevs );
  db_config(0,0);
  
  /* We delayed populating FILE till now on initial scan */
  if( nOldSize==0 ){
    update_file_table_with_lastcn();
  }
  
  /* Commit all changes to the database
  */
  db_execute("COMMIT;");

  return 0;
}

/*
** Diff two versions of a file, handling all exceptions.
**
** If oldVersion is NULL, then this function will output the
** text of version newVersion of the file instead of doing
** a diff.
*/
static int git_diff_versions(
  const char *oldVersion,
  const char *newVersion,
  const char *zFile
){
  char *zCmd;
  FILE *in;
  
  zCmd = mprintf("GIT_DIR='%s' git-diff-tree -t -p -r '%s' '%s' 2>/dev/null",
                 db_config("cvsroot",""),
                 quotable_string(oldVersion),
                 quotable_string(newVersion), quotable_string(zFile));
  
  in = popen(zCmd, "r");
  free(zCmd);
  if( in==0 ) return -1;
  
  output_pipe_as_html(in,1);
  pclose(in);
  
  return 0;
}

static char *git_get_blob(
  const char *zGitDir,
  const char *zTreeish,
  const char* zPath
){
  FILE *in;
  char zLine[PATH_MAX*2];
  char *zCmd;

  if( zTreeish==0 || zTreeish[0]==0 || zPath==0 || zPath[0]==0 ) return 0;
    
  zCmd = mprintf("GIT_DIR='%s' git-ls-tree -r '%s' '%s'", zGitDir,
                 quotable_string(zTreeish), quotable_string(zPath));
  in = popen(zCmd,"r");
  if( in==0 ){
    err_pipe("Reading tree",zCmd);
    return 0;
  }
  free(zCmd);

  while( !feof(in) && !ferror(in) ){
    char zMode[100], zType[100], zObject[100];

    if( 0==fgets(zLine,sizeof(zLine),in) ) break;

    sscanf(zLine, "%8s %90s %50[0-9a-fA-F]", zMode, zType, zObject);

    if( !strcmp(zType,"blob") ){
      return strdup(zObject);
    }
  }
  return 0;
}

static int git_dump_version(const char *zVersion, const char *zFile,int bRaw){
  int rc = -1;
  char *zCmd;
  const char *zRoot = db_config("cvsroot","");
  const char *zBlob = git_get_blob(zRoot, zVersion, zFile);
  if( zBlob==0 ) return -1;

  zCmd = mprintf("GIT_DIR='%s' git-cat-file blob '%s' 2>/dev/null", zRoot, zBlob);
  rc = common_dumpfile( zCmd, zVersion, zFile, bRaw );
  free(zCmd);

  return rc;
}

static int git_diff_chng(int cn, int bRaw){
  char *zRev;
  char *zCmd;
  char zLine[2000];
  FILE *in;
  
  zRev = db_short_query("SELECT vers FROM filechng WHERE cn=%d", cn);
  if( !zRev || !zRev[0] ) return -1; /* Invalid check-in number */
  
  zCmd = mprintf("GIT_DIR='%s' git-diff-tree -t -p -r '%s' 2>/dev/null",
                 db_config("cvsroot",""), quotable_string(zRev));
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

void init_git(void){
  g.scm.zSCM = "git";
  g.scm.zName = "GIT";
  g.scm.pxHistoryUpdate = git_history_update;
  g.scm.pxDiffVersions = git_diff_versions;
  g.scm.pxDiffChng = git_diff_chng;
  g.scm.pxIsFileAvailable = 0;  /* use the database */
  g.scm.pxDumpVersion = git_dump_version;
}


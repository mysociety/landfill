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
*/
#include "config.h"
#include <time.h>
#include <assert.h>
#include "history.h"

/*
** These are some functions that are commonly used by all SCM modules
*/

/*
** Make sure the given file or directory is contained in the
** FILE table of the database.
*/
void insert_file(const char *zName, int cn){
  int i;
  char *zBase, *zDir;
  char *zToFree;
  int isDir = 0;
	int nLen;
  int isNewer;

  if( zName==0 || zName[0]==0 ) return;
	
  zToFree = zDir = mprintf("%s", zName);
  if( zDir==0 ) return;

  /* Subversion will pass directories too so we need to detect those early on.
  */
  nLen = strlen(zDir)-1;
  if( zDir[nLen]=='/' ){
    zDir[nLen--] = 0;
    isDir = 1;
  }
	
  while( zDir && zDir[0] ){
    for(i=nLen; i>0 && zDir[i]!='/'; i--){}
    if( i==0 ){
      zBase = zDir;
      zDir = "";
    }else{
      zDir[i] = 0;
      zBase = &zDir[i+1];
    }
    isNewer = db_exists(
      "SELECT 1 FROM file WHERE dir='%q' AND base='%q' AND lastcn>%d",
      zDir, zBase, cn
    );
    if( isNewer ) break;
    db_execute(
      "REPLACE INTO file(isdir, base, dir, lastcn) "
      "VALUES(%d,'%q','%q',%d)",
      isDir, zBase, zDir, cn);
    isDir = 1;
    zName = zDir;
  }
  free(zToFree);
}


void update_file_table_with_lastcn(void){
  char **az;
  int i;
  
  az = db_query("SELECT MAX(cn),filename FROM filechng GROUP BY filename");
  for(i=0; az[i]; i+=2){
    insert_file(az[i+1], atoi(az[i]));
  }
  db_query_free(az);
}

/*
** WEBPAGE: /update_file_table
**
** Make sure the FILE table contains every file mentioned in
** FILECHNG with correct lastcn.
*/
void update_file_table(void){
  login_check_credentials();
  if( g.okSetup ){
    db_execute("BEGIN");
    update_file_table_with_lastcn();
    db_execute("COMMIT");
  }
  cgi_redirect("index");
}

/*
** Convert a struct tm* that represents a moment in UTC into the number
** of seconds in 1970, UTC.
*/
time_t mkgmtime(struct tm *p){
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
** This routine is called to complete the generation of an error
** message in the history_update module.
*/
void error_finish(int nErr){
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
void error_init(int *pnErr){
  if( *pnErr==0 ){
    common_standard_menu(0, 0);
    cgi_reset_content();
    cgi_set_status(200, "OK");
    cgi_set_content_type("text/html");
    common_header("Error Reading Repository");
    @ <p>The following errors occurred while trying to read and
    @ interpret
    if( !strcmp(g.scm.zSCM,"cvs") ){
      @ the CVSROOT/history file from the CVS repository.
    }else{
      @ the %h(g.scm.zName) repository.
    }
    @ This indicates a problem in the installation of CVSTrac.  Please save
    @ this page and contact your system administrator.</p>
    @ <ul>
  }
  ++*pnErr;
}

/*
** Perform the following substitutions on the input string zInCmd and
** write the result into a new string obtained from malloc.  Return the
** result.
**
**      %V1      First version number  (if first version is not NULL)
**      %V2      Second version number (if first version is not NULL)
**      %V       Second version number (if first version is NULL)
**      %F       Filename
**      %%       "%"
*/
char *subst(const char *zIn, const char **azSubst){
  int i, k;
  char *zOut;
  int nByte = 1;

  /* Sanitize the substitutions
  */
  for(i=0; azSubst[i]; i+=2){
    azSubst[i+1] = quotable_string(azSubst[i+1]);
  }

  /* Figure out how must space is required to hold the result.
  */
  nByte = 1;  /* For the null terminator */
  for(i=0; zIn[i]; i++){
    if( zIn[i]=='%' ){
      int c = zIn[++i];
      if( c=='%' ){
        nByte++;
      }else{
        int j, len = 0;
        for(j=0; azSubst[j]; j+=2){
          if( azSubst[j][0]!=c ) continue;
          len = strlen(azSubst[j]);
          if( strncmp(&zIn[i], azSubst[j], len)!=0 ) continue;
          break;
        }
        if( azSubst[j]==0 ){
          nByte += 2;
        }else{
          nByte += strlen(azSubst[j+1]);
          i += len - 1;
        }
      }
    }else{
      nByte++;
    }
  }

  /* Allocate a string to hold the result
  */
  zOut = malloc( nByte );
  if( zOut==0 ) exit(1);

  /* Do the substituion
  */
  for(i=k=0; zIn[i]; i++){
    if( zIn[i]=='%' ){
      int c = zIn[++i];
      if( c=='%' ){
        zOut[k++] = '%';
      }else{
        int j, len = 0;
        for(j=0; azSubst[j]; j+=2){
          if( azSubst[j][0]!=c ) continue;
          len = strlen(azSubst[j]);
          if( strncmp(&zIn[i], azSubst[j], len)!=0 ) continue;
          break;
        }
        if( azSubst[j]==0 ){
          zOut[k++] = '%';
          zOut[k++] = c;
        }else{
          strcpy(&zOut[k], azSubst[j+1]);
          k += strlen(&zOut[k]);
          i += len - 1;
        }
      }
    }else{
      zOut[k++] = zIn[i];
    }
  }
  zOut[k] = 0;
  assert( k==nByte-1 );
  return zOut;
}


/*
** Functions below are runtime dispatchers to scm specific functions
*/

/*
** Check the repository for any changes since the last check and update the
** database appropriately.
**
** If any errors occur, output an error page and exit.
**
** If the "isReread" flag is set, it means that the history file is being
** reread to pick up changes that we may have missed earlier.  
*/
void history_update(int isReread){
  if( g.scm.pxHistoryUpdate ) g.scm.pxHistoryUpdate(isReread);
}

/*
** Given a file version number, compute/find the previous version number.
** The new version number overwrites the old one.
*/
void previous_version(char *zVers,const char *zFile){
  char **az = db_query("SELECT prevvers,chngtype FROM filechng "
                       "WHERE filename='%q' AND vers='%q'",
                       zFile, zVers);
  zVers[0] = 0;
  if( az ){
    /* FIXME: we're making an assumption here that previous revision strings
     * are always no longer than the next revision length. That can break, for
     * example, if you had a SCM that did 1.134->2.1
     * Since we normally call previous_version() with a fairly large buffer (i.e.
     *   char zPrev[100];
     *   strcpy(zPrev,zVers);
     *   previous_version(zPrev,zFile);
     * ) this isn't something we need to fix right now...
     */
    strcpy(zVers, az[0]);
    db_query_free(az);
  }
}

/*
** Return non-zero if specified revision of the file is dead.
*/
int is_dead_revision(const char *zRelFile, const char *zVersion){
  int chngtype = 2;
  if( zVersion[0] ) {
    char *az = db_short_query("SELECT chngtype FROM filechng WHERE "
                              "filename='%q' AND vers='%q'",
                              zRelFile, zVersion );
    if(az==0) return 1;

    chngtype = atoi(az);
    free(az);
  }
  return chngtype==2;
}

/*
** Diff two versions of a file, handling all exceptions. Output the diff to
** the usual place (or whatever HTML is needed for an error).
**
** If oldVersion is NULL, then this function will output the
** text of version newVersion of the file instead of doing
** a diff.
**
** Returns zero on success.
*/
int diff_versions(
  const char *oldVersion,
  const char *newVersion,
  const char *file
){
  if( !g.okCheckout ) return -1;
  if( g.scm.pxDiffVersions ){
    return g.scm.pxDiffVersions(oldVersion,newVersion,file);
  }
  return -1;
}

/*
** Output the entire diff for the specified checkin/chng. Output
** should be HTML escaped if bRaw==0. If bRaw is non-zero, output should
** be ASCII and as close to the SCM's "native" diff as possible.
**
** Returns zero on success.
*/
int diff_chng(int cn, int bRaw){
  if( !g.okCheckout ) return -1;
  if( g.scm.pxDiffChng ){
    return g.scm.pxDiffChng(cn,bRaw);
  }
  return -1;
}

/*
** Return non-zero if the file hasn't been deleted, removed, or otherwise made
** unavailable should the user attempt a checkout.
*/
int is_file_available(const char *zFile){
  if( g.scm.pxIsFileAvailable ){
    return g.scm.pxIsFileAvailable(zFile);
  }else{
    /*
    ** Other SCMs can makes sense of the database. However, this only makes
    ** sense when trees are independent.
    */
    char *zLastChng = db_short_query(
      "SELECT chngtype FROM filechng WHERE filename='%q' ORDER BY cn DESC;",
      zFile
    );
    
    if( zLastChng && zLastChng[0] && atoi(zLastChng)!=2 ){
      free(zLastChng);
      return 1; /* File is available */
    }
    if(zLastChng) free(zLastChng);
  }
  return 0;
}

/*
** Output the contents of a particular version of a file. If bRaw is zero, output
** is expected to be HTML. HTML output can be run through extra filters and such
** for pretty display. Raw output shouldn't be touched.
**
** Returns zero on success.
*/
int dump_version(
  const char *version,
  const char *file,
  int bRaw
) {
  if( !g.okCheckout ) return -1;
  if( g.scm.pxDumpVersion ){
    return g.scm.pxDumpVersion(version,file,bRaw);
  }
  return -1;
}

/*
** Construct a filter for the specified filename and version. This get the
** "filefilter" option from CONFIG and runs it through standard substitutions
** to get something suitable for a pipe (including the leading |).
**
** NULL is returned if no filter is available.
*/
static char *get_filefilter(const char *zVersion, const char *zFile){
  char *zTemplate;
  char *zCmd;
  const char *azSubst[10];
  const char* zFilter = db_config("filefilter",NULL);
  if( zFilter==0 ) return 0;

  azSubst[0] = "F";
  azSubst[1] = quotable_string(zFile);
  azSubst[2] = "V";
  azSubst[3] = quotable_string(zVersion);
  azSubst[4] = "RP";
  azSubst[5] = db_config("cvsroot", "");
  azSubst[6] = 0;

  zTemplate = mprintf("| %s", zFilter);
  zCmd = subst(zTemplate, azSubst);
  free(zTemplate);
  return zCmd;
}

/*
** Output a file through a crude HTML filter. If the input is HTML and the
** bForce flag zero, it'll be passed through unchanged. Otherwise, it'll be
** wrapped with <pre> tags and marked up as HTML.
** Returns the number of bytes read from the pipe, or less than zero on failure.
*/
int output_pipe_as_html(FILE *in, int bForce){
  /* Output the result of the command.  If the first non-whitespace
  ** character is "<" then assume the command is giving us HTML.  In
  ** that case, do no translation.  If the first non-whitespace character
  ** is anything other than "<" then assume the output is plain text.
  ** Convert this text into HTML.
  */
  char zLine[2000];
  char *zFormat = 0;
  int i, n=0;
  while( fgets(zLine, sizeof(zLine), in) ){
    n += strlen(zLine);
    for(i=0; isspace(zLine[i]); i++){}
    if( zLine[i]==0 ) continue;
    if( zLine[i]=='<' && bForce==0 ){
      zFormat = "%s";
    }else{
      @ <pre>
      zFormat = "%h";
    }
    cgi_printf(zFormat, zLine);
    break;
  }
  while( fgets(zLine, sizeof(zLine), in) ){
    n += strlen(zLine);
    cgi_printf(zFormat, zLine);
  }
  if( zFormat && zFormat[1]=='h' ){
    @ </pre>
  }
  return n;
}

/*
** Implementas a common file filtering logic. The caller provides the command to
** actually extract the file to stdout and this routine uses the bRaw and filter
** availability to actually implement the file dump logic.
*/
int common_dumpfile(
  const char *zCmd,
  const char *zVersion,
  const char *zFile,
  int bRaw
){
  FILE *in;
  char zLine[2000];

  if( !g.okCheckout ) return -1;
  if( bRaw ){
    in = popen(zCmd, "r");
    if( in==0 ) return -1;

    while( !feof(in) ){
      int amt = fread(zLine, 1, sizeof(zLine), in);
      if( amt<=0 ) break;
      cgi_append_content(zLine, amt);
    }
    pclose(in);
  }else{
    const char* zFilter = get_filefilter(zVersion,zFile);
    char *zMyCmd = mprintf("%s %s", zCmd, zFilter ? zFilter : "");
    if( zMyCmd==0 ) return -1;

    in = popen(zMyCmd, "r");
    free(zMyCmd);
    if( in==0 ) return -1;

    if( zFilter ){
      output_pipe_as_html(in,0);
    }else{
      @ <pre>
      while( fgets(zLine, sizeof(zLine), in) ){
        cgi_printf("%h",zLine);
      }
      @ </pre>
    }
    pclose(in);
  }

  return 0;
}

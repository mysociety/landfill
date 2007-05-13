/*
** Copyright (c) 2006 D. Richard Hipp
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
** This file contains code used to execute external programs on CVSTrac
** objects.
*/
#include "config.h"
#include "tools.h"
#include <sys/times.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

/*
** Output a file through a crude HTML filter. If the input is HTML and the
** bForce flag zero, it'll be passed through unchanged. Otherwise, it'll be
** wrapped with <pre> tags and marked up as HTML.
** Returns the number of bytes read from the pipe, or less than zero on failure.
*/
int output_tool(FILE *in){
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
    if( zLine[i]=='<' ){
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
** Execute the specified tool, handling all the common substitutions and
** putting content into a temporary file.
** Returns number of bytes received from the tool, or less than zero on
** failure.
*/
int execute_tool(
  const char *zTool,
  const char *zAction,
  const char *zContent,
  const char **azTSubst
){
  FILE *in;
  char *zCmd;
  const char *azSubst[32];
  int n = 0;
  char zF[200];

  db_add_functions();
  if( !db_exists("SELECT 1 FROM tool,user "
                 "WHERE tool.name='%q' AND user.id='%q' "
                 "      AND cap_and(user.capabilities,tool.perms)!=''",
                 zTool, g.zUser)){
    return -1;
  }

  while(azTSubst[n]){
    azSubst[n] = azTSubst[n];
    n++;
  }

  azSubst[n++] = "RP";
  azSubst[n++] = db_config("cvsroot", "");
  azSubst[n++] = "B";
  azSubst[n++] = g.zBaseURL;
  azSubst[n++] = "P";
  azSubst[n++] = g.zName;
  azSubst[n++] = "U";
  azSubst[n++] = g.zUser;
  azSubst[n++] = "N";
  azSubst[n++] = mprintf("%d",time(0));
  azSubst[n++] = "T";
  azSubst[n++] = quotable_string(zTool);
  azSubst[n++] = "UC";
  azSubst[n++] = db_short_query("SELECT capabilities FROM user "
                                "WHERE id='%q'",g.zUser);
  zF[0] = 0;
  if( zContent ){
    /* content is special. If provided, we need to write it to a temp file
    ** so that it can be given to the program via the %C subst.
    */
    if( !write_to_temp(zContent,zF,sizeof(zF)) ){
      azSubst[n++] = "C";
      azSubst[n++] = zF;
    }
  }

  azSubst[n] = 0;

  zCmd = subst(zAction, azSubst);

  in = popen(zCmd,"r");
  free(zCmd);
  if( in==0 ) {
    if( zF[0] ) unlink(zF);
    return -1;
  }

  n = output_tool(in);

  pclose(in);
  if( zF[0] ) unlink(zF);

  return n;
}


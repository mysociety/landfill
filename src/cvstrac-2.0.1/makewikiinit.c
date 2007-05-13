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
** This file implements a standalone program used to generate the
** "wiki_init.c" file for CVSTrac.
**
** "wiki_init.c" contains a single routine that initializes a new
** CVSTrac database with wiki pages containing CVSTrac documentation.
**
** This program generates wiki_init.c by reading an existing CVSTrac
** database file, extract all the wiki pages, and writing the code
** necessary to exactly reproduce those pages.
*/
#include <stdio.h>
#include <sqlite3.h>

static int generate_page(void *NotUsed, int argc, char **argv, char **Unused){
  int i;
  char *z;
  printf("static const char z%s[] =\n", argv[0]);
  z = argv[1];
  while( *z ){
    for(i=0; z[i] && z[i]!='\n'; i++){}
    printf("@ %.*s\n", i, z);
    if( z[i]=='\n' ) i++;
    z += i;
  }
  printf(";\n");
  return 0;
}

int main(int argc, char **argv){
  sqlite3 *db;
  char *zErrMsg;
  int rc;
  int i;
  int nName, nCol;
  char **azName;

  if( argc!=2 ){
    fprintf(stderr,"Usage: %s DATABASE >wikiinit.c\n", argv[0]);
    exit(1);
  }
  if( access(argv[1], 4) ){
    perror("Database file does not exist or is not readable");
    exit(1);
  }
  if( SQLITE_OK != sqlite3_open(argv[1], &db) ){
    fprintf(stderr,"Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(1);
  }
  printf(
    "/*** AUTOMATICALLY GENERATED FILE - DO NOT EDIT ****\n"
    "**\n"
    "** This file was generated automatically by the makewikiinit.c\n"
    "** program.  See the sources to that program for additional\n"
    "** information.\n"
    "**\n"
    "** This file contains code used to initialize the wiki for a new\n"
    "** CVSTrac database.\n"
    "*/\n"
    "#include \"config.h\"\n"
    "#include \"wikiinit.h\"\n"
    "#include <time.h>\n"
    "\n"
  );
  rc = sqlite3_get_table(db,
         "SELECT DISTINCT name FROM wiki ORDER BY name",
         &azName, &nName, &nCol, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr,"Database error: %s\n", zErrMsg);
    exit(1);
  }
  for(i=1; i<=nName; i++){
    char *zSql = sqlite3_mprintf(
                         "SELECT name, text FROM wiki WHERE name='%s' LIMIT 1",
                         azName[i]);
    rc = sqlite3_exec(db,zSql,generate_page,0,&zErrMsg);
    free(zSql);
    if( rc!=SQLITE_OK ){
      fprintf(stderr,"Database error: %s\n", zErrMsg);
      exit(1);
    }
  }
  sqlite3_close(db);
  printf(
    "static const struct {\n"
    "  const char *zName;\n"
    "  const char *zText;\n"
    "} aPage[] = {\n"
  );
  for(i=1; i<=nName; i++){
    printf("  { \"%s\", z%s },\n", azName[i], azName[i]);
  }
  printf(
    "};\n"
    "void initialize_wiki_pages(void){\n"
    "  int i;\n"
    "  time_t now = time(0);\n"
    "  for(i=0; i<sizeof(aPage)/sizeof(aPage[0]); i++){\n"
    "    char* zOld = db_short_query(\"SELECT text FROM wiki \"\n"
    "                                \"WHERE name='%%q' \"\n"
    "                                \"ORDER BY invtime LIMIT 1\",\n"
    "                                aPage[i].zName);\n"
    "    if(zOld && zOld[0] && 0==strcmp(zOld,aPage[i].zText)) continue;\n"
    "    db_execute(\"INSERT INTO wiki(name,invtime,locked,who,ipaddr,text)\"\n"
    "       \"VALUES('%%s',%%d,%%d,'setup','','%%q')\",\n"
    "       aPage[i].zName, -(int)now,\n"
    "       strcmp(aPage[i].zName, \"WikiIndex\")!=0, \n"
    "       aPage[i].zText);\n"
    "  }\n"
    "}\n"
  );
  return 0;
}

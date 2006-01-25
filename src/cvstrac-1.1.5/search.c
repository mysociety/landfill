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
** This file contains code used to do text searches of the database
*/
#include "config.h"
#include "search.h"


/*
** We'll use this routine in several places.
*/
extern int sqliteStrNICmp(const char*,const char*,int);

/*
** Search for a keyword in text.  Return a matching score:
**
**     0     No sign of the word was found in the text
**     6     The word was found but not on a word boundry
**     8     The word was found with different capitalization
**    10     The word was found in the text exactly as given.
*/
static int score_word(const char *zWord, const char *zText, int n, int *pIdx){
  int c1, c2, i, best;
  int idx = -1;

  best = 0;
  c1 = zWord[0];
  if( isupper(c1) ){
    c2 = tolower(c1);
  }else{
    c2 = toupper(c1);
  }
  if( n<=0 ) n = strlen(zWord);
  for(i=0; zText[i]; i++){
    if( (zText[i]==c1 || zText[i]==c2) && sqliteStrNICmp(zWord,&zText[i],n)==0){
      int score = 6;
      if( (i==0 || !isalnum(zText[i-1]))
           && (zText[i+n]==0 || !isalnum(zText[i+n])) ){
        score = 8;
        if( strncmp(zWord,&zText[i],n)==0 ){
          idx = i;
          best = score = 10;
          break;
        }
      }
      if( score>best ){
        best = score;
        idx = i;
      }
    }
  }
  if( pIdx ) *pIdx = idx;
  return best;
}

/*
**    search(PATTERN, STRING, ...)
**
** This routine implements the SQLite "search()" function.  There are two
** arguments: text to be searched and a keyword pattern.  The function
** returns an integer score which is higher depending on how well the
** search went.
*/
static void srchFunc(sqlite_func *context, int argc, const char **argv){
  int i, j, k, score, total;
  const char *zPattern;

  if( argc<2 || argv[0]==0 || argv[0][0]==0 ) return;
  total = 0;
  zPattern = argv[0]; 
  if( zPattern && zPattern[0] ){
    for(i=0; zPattern[i]; i++){
      if( isspace(zPattern[i]) ) continue;
      for(j=1; zPattern[i+j] && !isspace(zPattern[i+j]); j++){}
      score = 0;
      for(k=1; k<argc; k++){
        int one_score;
        if( argv[k]==0 ) continue;
        one_score = score_word(&zPattern[i], argv[k], j, 0);
        if( one_score>score ) score = one_score;
      }
      total += score;
      i += j-1;
    }
  }
  sqlite_set_result_int(context, total);
}

/*
**    highlight(WORD-LIST, TEXT, ...)
**
** Return HTML which contains TEXT but with every word in WORD-LIST
** enclosed within <b>...</b>.  Long segments of TEXT that do not
** contain any matching words are replace with elipses.
*/
static void highlightFunc(sqlite_func *context, int argc, const char **argv){
  int i, j, k;
  int size;
  int wn, idx;
  int nKey;                 /* Number of search terms in zPattern */
  char *azKey[50];          /* Up to 50 search terms in zPattern */
  int keySize[50];          /* Number of characters in each search term */
  int begin[5], end[5];     /* Up to 5 100-character segments of text */
  int sbegin[5], send[5];   /* The same 5 segments, sorted and coalesced */
  char mask[256];           /* True if first character of a key */
  char *zAll;
  char *zPattern;
  char *z;

  /* Output is written into zOut[].  There are at most 5 exemplars of
  ** about 100 characters each - total 500 characters.  But then we have
  ** to insert <b> and </b> around search terms and escape HTML characters
  ** such as < and > and &.  The maximum expansion is "& " converted
  ** into "<b>&amp;</b> " or less than 7-to-1.  So the maximum output
  ** is 3500.  Add a little extra space for the ellipses between
  ** exemplars.  Plus some margin in case the calculation above is
  ** wrong in some way.
  */
  char zOut[8000];

  /*
  ** We must have a pattern and at least one text block to function.
  */
  if( argc<2 || argv[0]==0 || argv[0][0]==0 ) return;

  /*
  ** Concatenate all text to be analyzed.
  */
  size = 0;
  for(k=1; k<argc; k++){
    if( argv[k]==0 ) continue;  
    size += strlen(argv[k])+1;
  }
  zAll = malloc( size );
  if( zAll==0 ) return;
  for(i=0, k=1; k<argc; k++){
    if( argv[k]==0 ) continue;
    if( i>0 ){ zAll[i++] = '\n'; }
    strcpy(&zAll[i], argv[k]);
    i += strlen(&zAll[i]);
  }

  /*
  ** Make a copy of the pattern.
  */
  zPattern = malloc( strlen(argv[0])+1 ); 
  if( zPattern==0 ) return;
  strcpy(zPattern, argv[0]);

  /*
  ** Find as many as 5 exemplar segments in the text with each segment
  ** as long as 100 characters.
  */
  nKey = 0;
  for(wn=i=0; wn<50 && zPattern[i];){
    int score;
    if( isspace(zPattern[i]) ){ i++; continue; }
    for(j=1; zPattern[i+j] && !isspace(zPattern[i+j]); j++){}
    azKey[nKey] = &zPattern[i];
    keySize[nKey++] = j;
    score = score_word(&zPattern[i], zAll, j, &idx);
    i += j;
    if( zPattern[i] ) zPattern[i++] = 0;
    if( score==0 ) continue;
    begin[wn] = idx-50 > 0 ? idx-50 : 0;
    end[wn] = begin[wn]+100 > size ? size : begin[wn]+100;
    wn++;
  }

  /*
  ** Sort an coalesce the exemplars
  */
  if( wn==0 ){
    begin[0] = 0;
    end[0] = size>100 ? 100 : size;
    wn = 1;
  }
  for(i=0; i<5 && wn>0; i++){
    int min = begin[0];
    k = 0;
    for(j=1; j<wn; j++){
      if( begin[j]<min ){
        k = j;
        min = begin[j];
      }
    }
    sbegin[i] = begin[k];
    send[i] = end[k];
    wn--;
    begin[k] = begin[wn];
    end[k] = end[wn];
  }
  wn = i;
  begin[0] = sbegin[0];
  end[0] = send[0];
  for(i=j=1; i<wn; i++){
    if( sbegin[i]>end[j-1]+1 ){
      begin[j] = sbegin[i];
      end[j] = send[i];
      j++;
    }else if( send[i]>end[j-1] ){
      end[j-1] = send[i];
    }
  }
  wn = j;

  /*
  ** Initialize the mask[] array so that mask[x] has value 1 if x is the
  ** first character of any key pattern.
  */
  memset(mask, 0, sizeof(mask));
  for(i=0; i<nKey; i++){
    mask[toupper(azKey[i][0])] = 1;
    mask[tolower(azKey[i][0])] = 1;
  }

  /*
  ** Generate the output stream
  */
  z = zOut;
  for(i=0; i<wn; i++){
    if( begin[i]>0 ){
      if( i>0 ){ *(z++) = ' '; }
      strcpy(z, "<b>...</b> ");
      z += strlen(z);
    }
    for(j=begin[i]; j<end[i]; j++){
      int c = zAll[j];
      if( c=='&' ){
        strcpy(z, "&amp;");
        z += 5;
      }else if( c=='<' ){
        strcpy(z, "&lt;");
        z += 4;
      }else if( c=='>' ){
        strcpy(z, "&gt;");
        z += 4;
      }else if( mask[c] ){
        for(k=0; k<nKey; k++){
          int n;
          if( tolower(c)!=tolower(azKey[k][0]) ) continue;
          n = keySize[k];
          if( sqliteStrNICmp(&zAll[j],azKey[k],n)==0 ){
            strcpy(z,"<b>");
            z += 3;
            while( n ){
              c = zAll[j++];
              if( c=='&' ){
                strcpy(z, "&amp;");
                z += 5;
              }else if( c=='<' ){
                strcpy(z, "&lt;");
                z += 4;
              }else if( c=='>' ){
                strcpy(z, "&gt;");
                z += 4;
              }else{
                *(z++) = c;
              }
              n--;
            }
            j--;
            strcpy(z,"</b>");
            z += 4;
            break;
          }
        }
        if( k>=nKey ){
          *(z++) = c;
        }
      }else{
        *(z++) = c;
      }
    }
  }
  if( end[wn-1]<size ){
    strcpy(z, " <b>...</b>");
  }else{
    *z = 0;
  }

  /*
  ** Report back the results
  */
  sqlite_set_result_string(context, zOut, -1);
  free(zAll);
  free(zPattern);
}

/*
** WEBPAGE: /search
*/
void search_page(void){
  int srchTkt = PD("t","0")[0]!='0';
  int srchCkin = PD("c","0")[0]!='0';
  int srchWiki = PD("w","0")[0]!='0';
  char *zPattern;
  char **azResult = 0;
  sqlite *db;
  char zSql[10000];
  extern char *sqlite_mprintf(const char*, ...);

  login_check_credentials();
  zPattern = sqlite_mprintf("%q",PD("s",""));
  if( srchTkt+srchCkin+srchWiki==0 ){
    srchTkt = srchCkin = srchWiki = 1;
  }
  if( !g.okRead ) srchTkt = 0;
  if( !g.okCheckout ) srchCkin = 0;
  if( !g.okRdWiki ) srchWiki = 0;
  if( srchTkt==0 && srchCkin==0 && srchWiki==0 ){
    login_needed();
    return;
  }
  if( zPattern && zPattern[0] && strlen(zPattern)<100 ){
    int i;
    char *zConnect = " (";

    sprintf(zSql,"SELECT score, url, title, body FROM");
    i = strlen(zSql);
    db = db_open();
    sqlite_create_function(db, "search", -1, srchFunc, 0);
    sqlite_create_function(db, "highlight", -1, highlightFunc, 0);

    if( srchTkt ){
      sprintf(&zSql[i],"%s SELECT "
         "search('%s',title,description,remarks) as score, "
         "'tktview?tn=' || tn as url, "
         "'Ticket #' || tn || ': ' || highlight('%s',title) as title, "
         "highlight('%s',description,remarks) as body FROM ticket "
         "WHERE score>0",
         zConnect, zPattern, zPattern, zPattern);
      i += strlen(&zSql[i]);
      zConnect = " UNION ALL";
    }
    if( srchCkin ){
      sprintf(&zSql[i],"%s SELECT "
         "search('%s',message) as score, "
         "'chngview?cn=' || cn as url, "
         "'Check-in [' || cn || ']' as title, "
         "highlight('%s',message) as body FROM chng "
         "WHERE score>0",
         zConnect, zPattern, zPattern);
      i += strlen(&zSql[i]);
      zConnect = " UNION ALL";
    }
    if( srchWiki ){
      sprintf(&zSql[i],"%s SELECT "
         "search('%s',name, text) as score, "
         "'wiki?p=' || name as url, "
         "'Wiki Page ' || highlight('%s',name) as title, "
         "highlight('%s',text) as body "
         "FROM (select name as nm, min(invtime) as tm FROM wiki GROUP BY name) "
         "    as x, wiki "
         "WHERE score>0 AND x.nm=wiki.name AND x.tm=wiki.invtime",
         zConnect, zPattern, zPattern, zPattern);
      i += strlen(&zSql[i]);
      zConnect = " UNION ALL";
    }
    sprintf(&zSql[i], ") ORDER BY score DESC LIMIT 30;");
    azResult = db_query(zSql);
  }
  common_standard_menu("search", 0);
  common_header("Search");
  @ <form action="search" method="GET">
  @ Search:
  @ <input type="text" size=40 name="s" value="%h(PD("s",""))">
  @ <input type="submit" value="Go">
  if( g.okRead + g.okCheckout + g.okRdWiki>1 ){
    char *z;
    @ <br>Look in:
    if( g.okRead ){
      z = srchTkt ? " checked " : "";
      @ <input type="checkbox" name="t" value="1"%s(z)> Tickets
    }
    if( g.okCheckout ){
      z = srchCkin ? " checked " : "";
      @ <input type="checkbox" name="c" value="1"%s(z)> Checkins
    }
    if( g.okRdWiki ){
      z = srchWiki ? " checked " : "";
      @ <input type="checkbox" name="w" value="1"%s(z)> Wiki Pages
    }
  }
  @ </form>
  if( azResult && azResult[0] ){
    int i;
    @ <hr>
    for(i=0; azResult[i]; i+=4){
      @ <p><a href="%s(azResult[i+1])">%s(azResult[i+2])</a><br>
      @ %s(azResult[i+3])</p>
    }
  }
  common_footer();
}

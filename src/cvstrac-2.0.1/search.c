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
extern int sqlite3StrNICmp(const char*,const char*,int);

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
    if( (zText[i]==c1 || zText[i]==c2) && sqlite3StrNICmp(zWord,&zText[i],n)==0){
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
void srchFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  int i, j, k, score, total;
  char *zPattern;

  if( argc<2 || argv[0]==0 ) return;
  zPattern = (char *)sqlite3_value_text(argv[0]);
  total = 0;
  if( zPattern && zPattern[0] ){
    for(i=0; zPattern[i]; i++){
      if( isspace(zPattern[i]) ) continue;
      for(j=1; zPattern[i+j] && !isspace(zPattern[i+j]); j++){}
      score = 0;
      for(k=1; k<argc; k++){
        int one_score;
        char *zWord = (char *)sqlite3_value_text(argv[k]);
        if( zWord==0 || zWord[0]==0 ) continue;
        one_score = score_word(&zPattern[i], zWord, j, 0);
        if( one_score>score ) score = one_score;
      }
      total += score;
      i += j-1;
    }
  }
  sqlite3_result_int(context, total);
}

/*
**    highlight(WORD-LIST, TEXT, ...)
**
** Return HTML which contains TEXT but with every word in WORD-LIST
** enclosed within <b>...</b>.  Long segments of TEXT that do not
** contain any matching words are replace with elipses.
*/
static void highlightFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
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
  if( argc<2 || argv[0]==0 ) return;
  zPattern = (char *)sqlite3_value_text(argv[0]);
  if( zPattern[0]==0 ) return;

  /*
  ** Make a copy of the pattern.
  */
  zPattern = strdup(zPattern);
  if( zPattern==0 ) return;

  /*
  ** Concatenate all text to be analyzed.
  */
  size = 0;
  for(k=1; k<argc; k++){
    if( argv[k]==0 ) continue;  
    z = (char*)sqlite3_value_text(argv[k]);
    if( z==0 ) continue;
    size += strlen(z)+1;
  }
  zAll = malloc( size );
  if( zAll==0 ) return;
  for(i=0, k=1; k<argc; k++){
    if( argv[k]==0 ) continue;
    z = (char*)sqlite3_value_text(argv[k]);
    if( z==0 ) continue;
    if( i>0 ){ zAll[i++] = '\n'; }
    strcpy(&zAll[i], z);
    i += strlen(&zAll[i]);
  }

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
  if( g.useUTF8 ){
    /*
    ** Avoid splitting UTF-8 characters by detecting high bits in the
    ** begin/end characters.
    */
    for(i=0; i<wn; i++){
      while( begin[i]>0 && (zAll[begin[i]]&0xc0)==0x80 ){
        begin[i]--;
      }
      while( (zAll[end[i]]&0xc0)==0x80 ){
        end[i]++;
      }
    }
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
          if( sqlite3StrNICmp(&zAll[j],azKey[k],n)==0 ){
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
  sqlite3_result_text(context, zOut, -1, SQLITE_TRANSIENT);
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
  int srchFile = PD("f","0")[0]!='0';
  int nPage = atoi(PD("p","0"));
  const char *zPattern;
  char **azResult = 0;
  char *zPage;
  sqlite3 *db;
  char zSql[10000];

  login_check_credentials();
  zPattern = P("s");
  if( zPattern && zPattern[0] ){
    zPattern = sqlite3_mprintf("%q",zPattern);
  }
  if( srchTkt+srchCkin+srchWiki+srchFile==0 ){
    srchTkt = srchCkin = srchWiki = srchFile = 1;
  }
  if( !g.okRead ) srchTkt = 0;
  if( !g.okCheckout ) srchFile = srchCkin = 0;
  if( !g.okRdWiki ) srchWiki = 0;
  if( srchTkt==0 && srchCkin==0 && srchWiki==0 && srchFile==0 ){
    login_needed();
    return;
  }
  if( zPattern && zPattern[0] && strlen(zPattern)<100 ){
    int i = 0;
    char *zConnect = " (";

    db = db_open();
    db_add_functions();
    sqlite3_create_function(db, "highlight", -1, SQLITE_ANY, 0,
                            highlightFunc, 0, 0);

    appendf(zSql,&i,sizeof(zSql),"SELECT type, score, obj, title, body FROM");
    if( srchTkt ){
      appendf(zSql,&i,sizeof(zSql), "%s SELECT "
         "1 AS type, "
         "search('%s',title,description,remarks) as score, "
         "tn AS obj, "
         "highlight('%s',title) as title, "
         "highlight('%s',description,remarks) as body FROM ticket "
         "WHERE score>0",
         zConnect, zPattern, zPattern, zPattern);
      zConnect = " UNION ALL";
    }
    if( srchCkin ){
      appendf(zSql,&i,sizeof(zSql),"%s SELECT "
         "2 AS type, "
         "search('%s',message) as score, "
         "cn AS obj, "
         "NULL as title, " /* ignored */
         "highlight('%s',message) as body FROM chng "
         "WHERE score>0",
         zConnect, zPattern, zPattern);
      zConnect = " UNION ALL";
    }
    if( srchWiki ){
      appendf(zSql,&i,sizeof(zSql),"%s SELECT "
         "3 AS type, "
         "search('%s',name, text) as score, "
         "name AS obj, "
         "highlight('%s',name) as title, "
         "highlight('%s',text) as body "
         "FROM (select name as nm, min(invtime) as tm FROM wiki GROUP BY name) "
         "    as x, wiki "
         "WHERE score>0 AND x.nm=wiki.name AND x.tm=wiki.invtime",
         zConnect, zPattern, zPattern, zPattern);
      zConnect = " UNION ALL";
    }
    if( srchFile ){
      appendf(zSql,&i,sizeof(zSql),"%s SELECT "
         "CASE WHEN isdir THEN 6 ELSE 4 END as type, "
         "search('%s', dir || '/' || base) as score, "
         "dir || '/' || base AS obj, "
         "dir || '/' || base as title, "
         "highlight('%s',dir || '/' || base) as body FROM file "
         "WHERE score>0",
         zConnect, zPattern, zPattern);
      zConnect = " UNION ALL";

      appendf(zSql,&i,sizeof(zSql),"%s SELECT "
         "5 AS type, "
         "search('%s', fname, description) as score, "
         "atn AS obj, "
         "fname as title, "
         "highlight('%s',fname, description) as body FROM attachment "
         "WHERE score>0 AND tn!=0",
         zConnect, zPattern, zPattern);
      zConnect = " UNION ALL";
    }
    appendf(zSql,&i,sizeof(zSql), ") ORDER BY score DESC LIMIT 30 OFFSET %d;",
            nPage*30);
    azResult = db_query("%s",zSql);
  }
  common_standard_menu("search", 0);
  common_add_help_item("CvstracSearch");
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
      @ <label for="t"><input type="checkbox" name="t" id="t" value="1"%s(z)>
      @    Tickets</label>
    }
    if( g.okCheckout ){
      z = srchCkin ? " checked " : "";
      @ <label for="c"><input type="checkbox" name="c" id="c" value="1"%s(z)>
      @    Checkins</label>
    }
    if( g.okRdWiki ){
      z = srchWiki ? " checked " : "";
      @ <label for="w"><input type="checkbox" name="w" id="w" value="1"%s(z)>
      @    Wiki Pages</label>
    }
    if( g.okCheckout ){
      z = srchFile ? " checked " : "";
      @ <label for="f"><input type="checkbox" name="f" id="f" value="1"%s(z)>
      @    Filenames</label>
    }
  }
  @ </form>
  @ <div>
  if( azResult && azResult[0] ){
    int i, n;
    @ <hr>
    for(i=n=0; azResult[i]; i+=5, n++){
      @ <p>
      switch( atoi(azResult[i]) ){
        case 1: /* ticket */
          @ Ticket
          output_ticket(atoi(azResult[i+2]),0);
          @ : %s(azResult[i+3])
          break;
        case 2: /* check-in */
          @ Check-in
          output_chng(atoi(azResult[i+2]));
          break;
        case 3: /* wiki */
          @ Wiki Page <a href="wiki?p=%s(azResult[i+2])">%s(azResult[i+3])</a>
          break;
        case 4: /* file */
          @ File <a href="rlog?f=%t(azResult[i+2])">%h(azResult[i+3])</a>
          break;
        case 5: /* attachment */
          zPage = db_short_query("SELECT tn FROM attachment WHERE atn=%d",
                                 atoi(azResult[i+2]));
          @ Attachment
          @   <a href="attach_get/%s(azResult[i+2])/%t(azResult[i+3])">\
          @   %s(azResult[i+3])</a> to
          if( is_integer(zPage) ){
            output_ticket(atoi(zPage),0);
          }else{
            @ wiki page <a href="wiki?p=%s(zPage)">%s(zPage)</a>
          }
          break;
        case 6:{ /* directory */
          const char* zDir = azResult[i+2];
          if( zDir[0]=='/' ) zDir ++;
          @ Directory <a href="dir?d=%t(zDir)">%h(zDir)</a>
          break;
        }
      }
      @ <br>%s(azResult[i+4])</p>
    }
    if( nPage || n>=30 ){
      @ <hr><p align="center">
      /* provide a history */
      cgi_printf("Page: ");
      for( i=0; i<nPage; i++ ){
        cgi_printf("<a href=\"search?s=%T&t=%d&c=%d&w=%d&f=%d&p=%d\">%d</a> ",
            zPattern, srchTkt, srchCkin, srchWiki, srchFile, i, i+1);
      }
      cgi_printf("%d&nbsp;&nbsp;", nPage+1);
      if( n>=30 ){
        cgi_printf("<a href=\"search?s=%T&t=%d&c=%d&w=%d&f=%d&p=%d\">More</a>",
            zPattern, srchTkt, srchCkin, srchWiki, srchFile, nPage+1);
      }
      @ </p>
    }
  }else if( zPattern && zPattern[0] ){
    @ No results for <strong>%h(zPattern)</strong>
  }
  @ </div>
  common_footer();
}

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
** This file contains code used to generate convert wiki text into HTML.
*/
#include "config.h"
#include "format.h"
#include <time.h>
#include <limits.h>  /* for PATH_MAX */

/*
** Format a relative link for output. The idea here is to determine from context
** whether the link needs to be relative or absolute (i.e. for RSS output, e-mail
** notifications, etc). Returns the formatted string.
*/
char *format_link(const char* zFormat,...){
  char *zLink;
  va_list ap;
  va_start(ap,zFormat);
  zLink = vmprintf(zFormat,ap);
  va_end(ap);
  if( g.zLinkURL && g.zLinkURL[0] ){
    zLink = mprintf("%s/%z",g.zLinkURL,zLink);
  }
  return zLink;
}

/*
** Return the number digits at the beginning of the string z.
*/
int ndigit(const char *z){
  int i = 0;
  while( isdigit(*z) ){ i++; z++; }
  return i;
}

/*
** Check to see if *z contains nothing but spaces up to the next
** newline.  If so, return the number of spaces plus one for the
** newline characters.  If not, return 0.
**
** If two or more blank lines occur in a row, go ahead and return
** a number of characters sufficient to cover them all.
*/
static int is_blank_line(const char *z){
  int i = 0;
  int r = 0;
  while( isspace(z[i]) ){
    if( z[i]=='\n' ){ r = i+1; }
    i++;
  }
  return r;
}

/*
** Return TRUE if *z points to the terminator for a word.  Words
** are terminated by whitespace or end of input or any of the
** characters in zEnd.
** Note that is_eow() ignores zEnd characters _inside_ a word. They
** only count if they're followed by other EOW characters.
*/
int is_eow(const char *z, const char *zEnd){
  if( zEnd==0 ) zEnd = ".,:;?!)\"'";
  while( *z!=0 && !isspace(*z) ){
    int i;
    for(i=0; zEnd[i]; i++){ if( *z==zEnd[i] ) break; }
    if( zEnd[i]==0 ) return 0;
    z++;
  }
  return 1;
}

/*
** Check to see if *z points to the beginning of a Wiki page name.
** If it does, return the number of characters in that name.  If not,
** return 0.
**
** A Wiki page name contains only alphabetic characters.  The first
** letter must be capital and there must be at least one other capital
** letter in the word.  And every capital leter must be followed by
** one or more lower-case letters.
*/
int is_wiki_name(const char *z){
  int i;
  int nCap = 0;
  if( !isupper(z[0]) ) return 0;
  for(i=0; z[i]; i++){
    if( isupper(z[i]) ){
      if( !islower(z[i+1]) ) return 0;
      nCap++;
    }else if( !islower(z[i]) ){
      break;
    }
  }
  return (nCap>=2 && is_eow(&z[i],0)) ? i : 0;
}

/*
** Check to see if *z points to the beginning of a file in the repository.
** If it does, return the number of characters in that name.  If not,
** return 0.
**
** The filename must start with a slash and there'll have to be another slash
** somewhere inside. Spaces in filenames aren't supported.
*/
int is_repository_file(const char *z){
  char *s;
  int i;
  int gotslash=0;
  if( z[0]!='/' ) return 0;
  for(i=1; z[i] && !is_eow(&z[i],0); i++){
    if(z[i]=='/') gotslash=1;
  }
  if(!gotslash) return 0;

  /* see if it's in the repository. Note that we strip the leading '/' from the
   * query.
   */
  s = mprintf("%.*s", i-1, &z[1]);
  gotslash = db_exists("SELECT filename FROM filechng WHERE filename='%q'", s );
  free(s);
  return gotslash ? i : 0;
}

/*
** Check to see if z[] is a form that indicates the beginning of a
** bullet or enumeration list element.  z[] can be of the form "*:"
** or "_:" for a bullet or "N:" for an enumeration element where N
** is any number.  The colon can repeat 1 or more times.
**
** If z[] is not a list element marker, then return 0.  If z[] is
** a list element marker, set *pLevel to indicate the list depth
** (the number of colons) and the type (bullet or enumeration).  
** *pLevel is negative for enumerations and positive for bullets and
** the magnitude is the depth.  Then return the number of characters
** in the marker (which will always be at least 2.)
*/
static int is_list_elem(const char *z, int *pLevel){
  int type;
  int depth;
  const char *zStart = z;
  if( isdigit(*z) ){
    z++;
    while( isdigit(*z) ){ z++; }
    type = -1;
  }else if( *z=='*' || *z=='_' ){
    z++;
    type = +1;
  }else{
    *pLevel = 0;
    return 0;
  }
  depth = 0;
  while( *z==':' ){ z++; depth++; }
  while( isspace(*z) && *z!='\n' ){ z++; }
  if( depth==0 || depth>10 || *z==0 || *z=='\n' ){
    *pLevel = 0;
    return 0;
  }
  if( type<0 ){ 
    *pLevel = -depth;
  }else{
    *pLevel = depth;
  }
  return z - zStart;
}

/*
** If *z points to horizontal rule markup, return the number of
** characters in that markup.  Otherwise return 0.
**
** Horizontal rule markup consists of four or more '-' or '=' characters
** at the beginning of a line followed by nothing but whitespace
** to the end of the line.
*/
static int is_horizontal_rule(const char *z){
  int i;
  int c = z[0];
  if( c!='-' && c!='=' ) return 0;
  for(i=0; z[i]==c; i++){}
  if( i<4 ) return 0;
  while( isspace(z[i]) && z[i]!='\n' ){ i++; }
  return z[i]=='\n' || z[i]==0 ? i : 0;
}

/*
** Return the number of characters in the URL that begins
** at *z.  Return 0 if *z is not the beginning of a URL.
**
** Algorithm: Advance to the first whitespace character or until
** then end of the string.  Then back up over the following
** characters:  .)]}?!"':;,
*/
int is_url(const char *z){
  int i;
  int minlen = 6;
  switch( z[0] ){
    case 'h':
     if( strncmp(z,"http:",5)==0 ) minlen = 7;
     else if( strncmp(z,"https:",6)==0 ) minlen = 8;
     else return 0;
     break;
    case 'f':
     if( strncmp(z,"ftp://",6)==0 ) minlen = 7;
     else return 0;
     break;
    case 'm':
     if( strncmp(z,"mailto:",7)==0 ) minlen = 10;
     else return 0;
     break;
    default:
     return 0;
  }
  for(i=0; z[i] && !isspace(z[i]); i++){}
  while( i>0 ){
    switch( z[i-1] ){
      case '.':
      case ')':
      case ']':
      case '}':
      case '?':
      case '!':
      case '"':
      case '\'':
      case ':':
      case ';':
      case ',':
        i--;
        break;
      default:
        return i>=minlen ? i : 0;
    }
  }
  return 0;
}

/*
** Return true if the given URL points to an image.  An image URL is
** any URL that ends with ".gif", ".jpg", ".jpeg", or ".png"
*/
static int is_image(const char *zUrl, int N){
  int i;
  char zBuf[10];
  if( N<5 ) return 0;
  for(i=0; i<5; i++){
    zBuf[i] = tolower(zUrl[N-5+i]);
  }
  zBuf[i] = 0;
  return strcmp(&zBuf[1],".gif")==0 ||
         strcmp(&zBuf[1],".png")==0 ||
         strcmp(&zBuf[1],".jpg")==0 ||
         strcmp(&zBuf[1],".jpe")==0 ||
         strcmp(zBuf,".jpeg")==0;
}

/*
** Output N characters of text from zText.
*/
static void put_htmlized_text(const char **pzText, int N){
  if( N>0 ){
    char *z = htmlize(*pzText, N);
    cgi_printf("%s", z);
    free(z);
    *pzText += N;
  }
}

/*
** Search ahead in text z[] looking for a font terminator consisting
** of "n" consecutive instances of character "c".  The font terminator
** must be at the end of a word and it must occur before a paragraph break.
** Also, z[] must begin a new word.  If any of these conditions are false,
** return false.  If all conditions are meet, return true.
**
** TODO:  Ignore terminators that occur inside of special markup such
** as "{quote: not-a-terminator_}"
*/
static int font_terminator(const char *z, int c, int n){
  int seenNL = 0;
  int cnt = 0;
  if( isspace(*z) || *z==0 || *z==c ) return 0;
  z++;
  while( *z ){
    if( *z==c && !isspace(z[-1]) ){
      cnt++;
      if( cnt==n && is_eow(&z[1],0) ){
        return 1;
      }
    }else{
      cnt = 0;
      if( *z=='\n' ){
        if( seenNL ) return 0;
        seenNL = 1;
      }else if( !isspace(*z) ){
        seenNL = 0;
      }
    }
    z++;
  }
  return 0;
}

/*
** Return the number of asterisks at z[] and beyond.
*/
static int count_stars(const char *z){
  int n = 0;
  while( *z=='*' ){ n++; z++; }
  return n;
}

/*
** The following structure is used to record information about a single
** instance of markup.  Markup is text of the following form:
**
**         {type: key args}
**    or   {type: key}
**    or   {type}
**
** The key is permitted to begin with "}".  If args is missing, key is
** used in its place.  So {type: key} is equivalent to {type: key key}.
** If key is missing, then type is used in its place.  So {type} is the
** same as {type: type} which is the same as {type: type type}
*/
typedef struct Markup Markup;
struct Markup {
  int lenTotal;        /* Total length of the markup */
  int lenType;         /* Length of the "type" field */
  int lenKey;          /* Length of the "key" field */
  int lenArgs;         /* Length of the "args" field */
  const char *zType;   /* Pointer to the start of "type" */
  const char *zKey;    /* Pointer to the start of "key" */
  const char *zArgs;   /* Pointer to the start of "args" */
};

/*
** z[] is a string of text beginning with "{".  Check to see if it is
** valid markup.  If it is, fill in the pMarkup structure and return true.
** If it is not valid markup, return false.
*/
static int is_markup(const char *z, Markup *pMarkup){
  int i, j;
  int nest = 1;
  if( *z!='{' ) return 0;
  for(i=1; isalpha(z[i]); i++){}
  if( z[i]=='}' ){
    pMarkup->lenTotal = i+1;
    pMarkup->lenType = i-1;
    pMarkup->lenKey = i-1;
    pMarkup->lenArgs = i-1;
    pMarkup->zType = &z[1];
    pMarkup->zKey = &z[1];
    pMarkup->zArgs = &z[1];
    return 1;
  }
  if( z[i]!=':' ) return 0;
  pMarkup->lenType = i-1;
  pMarkup->zType = &z[1];
  i++;
  while( isspace(z[i]) && z[i]!='\n' ){ i++; }
  if( z[i]==0 || z[i]=='\n' ) return 0;
  j = i;
  pMarkup->zKey = &z[i];
  while( z[i] && !isspace(z[i]) ){
    if( z[i]=='}' ) nest--;
    if( z[i]=='{' ) nest++;
    if( nest==0 ) break;
    i++;
  }
  if( z[i]==0 || z[i]=='\n' ) return 0;
  pMarkup->lenKey = i - j;
  if( nest==0 ){
    pMarkup->lenArgs = i - j;
    pMarkup->lenTotal = i+1;
    pMarkup->zArgs = pMarkup->zKey;
    return 1;
  }
  while( isspace(z[i]) && z[i]!='\n' ){ i++; }
  if( z[i]=='\n' || z[i]==0 ) return 0;
  j = i;
  while( z[i] && z[i]!='\n' ){
    if( z[i]=='}' ) nest--;
    if( z[i]=='{' ) nest++;
    if( nest==0 ) break;
    i++;
  }
  if( z[i]!='}' || nest>0 ) return 0;
  pMarkup->zArgs = &z[j];
  pMarkup->lenArgs = i - j;
  pMarkup->lenTotal = i+1;
  return 1;
}

/*
** Calculate the length of the table cell starting just after a | and
** extending to the next non-quoted (i.e. not in {} markup) |
** or end-of-line. Returns zero if there's
** no complete (i.e. |-terminated) cell. Cell length does _not_ include
** the ending |.
*/
static int table_cell_length(const char *z){
  Markup markup;
  int i = 0;

  while( z[i] && z[i]!='|' && z[i]!='\n' ){
    if( z[i]=='{' && is_markup(&z[i],&markup) ){
      i += markup.lenTotal;
    }else{
      i++;
    }
  }
  return (z[i]=='|') ? i : 0;
}
/*
** If *z points to a row of table markup, return the number of
** characters in that markup.  Otherwise return 0.
**
** Table markup consists of a line starting with '|' and each cell
** separated by more '|' characters. The line ends with a '|' followed by
** nothing but whitespace to the end-of-line.
*/
static int is_table_row(const char *z){
  int i = 0, j;
  if( z[0]!='|' ) return 0;
  while( z[i]=='|' && (j=table_cell_length(&z[++i]))!=0 ){
    i += j;
  }

  for(; z[i]!='\n' && isspace(z[i]); i++){}

  return (z[i]=='\n' || z[i]==0) ? i : 0;
}

/*
** Output the table row defined by z. Individual cells can be wiki formatted
** (within reason), so knowing cell boundaries depends on checking for
** wiki markup and such.
*/
static void output_table_row(const char *z, int nLen){
  int i = 0, j;
  char *zCell;

  @ <tr>
  while( i<nLen && z[i]=='|' && (j=table_cell_length(&z[++i]))!=0 ){
    zCell = mprintf("%.*s",j,&z[i]);
    @ <td>
    output_formatted(zCell,0);
    free(zCell);
    @ </td>
    i += j;
  }
  @ </tr>
}

/*
** The aList[] array records the current nesting of <ul> and <ol>.  
** aList[0] records the stack depth.  (Max depth of 10).  aList[1]
** is +1 if the outer layer is <ul> and -1 if the outer layer is <ol>
** aList[2] holds similar information for the second layer, and so forth.
**
** The iTarget parameter specifies the desired depth of the stack and
** whether the inner most level is <ul> or <ol>  The absolute value of
** iTarget is the desired depth.  iTarget is negative for <ol> on the
** inner layer and positive for <ul> on the inner layer.
**
** The routine outputs HTML to adjust the list nesting to the desired
** level.
*/
static void adjust_list_nesting(int *aList, int iTarget){
  int iDepth = iTarget;
  if( iDepth<0 ) iDepth = 0x7fffffff & -iDepth;
  if( aList[0]==iDepth && iDepth>0 && aList[iDepth]*iTarget<0 ){
    iDepth--;
  }
  while( aList[0]>iDepth ){
    if( aList[aList[0]--]>0 ){
      cgi_printf("</ul>\n");
    }else{
      cgi_printf("</ol>\n");
    }
  }
  while( aList[0]<iDepth-1 ){
    cgi_printf("<ul>\n");
    aList[0]++;
    aList[aList[0]] = +1;
  }
  iDepth = iTarget;
  if( iDepth<0 ) iDepth = 0x7fffffff & -iDepth;  
  if( aList[0]==iDepth-1 ){
    if( iTarget<0 ){
      cgi_printf("<ol>\n");
      aList[iDepth] = -1;
    }else{
      cgi_printf("<ul>\n");
      aList[iDepth] = +1;
    }
    aList[0]++;
  }
}

/*
** Return non-zero if the specified string is in the given sorted list.
*/
static int inSortedList(const char *z, int nCh, const char* azList[], int nList){
  int i;
  int upr, lwr, mid, c;
  char zBuf[32];
  if( nCh<=0 || nCh>sizeof(zBuf)-1 ) return 0;
  for(i=0; i<nCh; i++) zBuf[i] = tolower(z[i]);
  zBuf[i] = 0;
  upr = nList - 1;
  lwr = 0;
  while( upr>=lwr ){
    mid = (upr+lwr)/2;
    c = strcmp(azList[mid],zBuf);
    if( c==0 ) return 1;
    if( c<0 ){
      lwr = mid+1;
    }else{
      upr = mid-1;
    }
  }
  return 0;
}

/*
** The following table contains all of the allows HTML markup for the
** restricted HTML output routine.  If an HTML element is found which is
** not on this list, it is escaped.
**
** A binary search is done on this list, so it must be in sorted order.
*/
static const char *azAllowedHtml[] = {
  "a",
  "address",
  "b",
  "big",
  "blockquote",
  "br",
  "center",
  "cite",
  "code",
  "dd",
  "dfn",
  "dir",
  "dl",
  "dt",
  "em",
  "font",
  "h1",
  "h2",
  "h3",
  "h4",
  "h5",
  "h6",
  "hr",
  "i",
  "img",
  "kbd",
  "li",
  "menu",
  "nobr",
  "ol",
  "p",
  "pre",
  "s",
  "samp",
  "small",
  "strike",
  "strong",
  "sub",
  "sup",
  "table",
  "td",
  "th",
  "tr",
  "tt",
  "u",
  "ul",
  "var",
  "wbr",
};

/*
** The following table is a list of accepted HTML element attributes.
** Any attribute not on the list will be stripped out during processing.
**
** A binary search is done on this list, so it must be in sorted order.
*/
static const char *azAllowedAttr[] = {
  "abbr",
  "accesskey",
  "align",
  "alt",
  "axis",
  "bgcolor",
  "border",
  "cellpadding",
  "cellspacing",
  "char",
  "charoff",
  "charset",
  "cite", /* URI */
  "class",
  "clear",
  "color",
  "colspan",
  "compact",
  "dir",
  "face",
  "frame",
  "headers"
  "height",
  "href", /* uri */
  "hreflang",
  "hspace",
  "id",
  "lang",
  "longdesc",
  "name",
  "noshade",
  "nowrap",
  "rel",
  "rev",
  "rowspan",
  "rules",
  "scope",
  "size",
  "span",
  "src",  /* URI */
  "start",
  "summary",
  "title",
  "valign",
  "value",
  "width",
};

/*
** Return TRUE if all HTML attributes up to the next '>' in the input string
** are on the allowed list (and pass any other checks we might want to add
** down the road...)
*/
static int isAllowedAttr(const char *zAttr,int nAttr){
  int i,j;
  int inquote = 0;
  int inbody = 0;

  for(i=0; i<nAttr && zAttr[i]!='>'; i++){
    if( !inbody && !inquote && isalpha(zAttr[i]) ){
      for(j=1; i+j<nAttr && isalnum(zAttr[i+j]); j++){}

      if( !inSortedList(&zAttr[i], j, azAllowedAttr,
                        sizeof(azAllowedAttr)/sizeof(azAllowedAttr[0]))){
        return 0;
      }
      i += j-1;
      inbody = 0;
    }else if( inquote && zAttr[i]=='"' ){
      inquote=0;
    }else if( !inquote && zAttr[i]=='"' ){
      inquote=1;
    }else if( isspace(zAttr[i]) ){
      inbody = 0;
    }
  }
  return 1;
}

/*
** Return TRUE if the HTML element given in the argument is on the allowed
** element list.
*/
static int isAllowed(const char *zElem, int nElem){
  return inSortedList(zElem, nElem, azAllowedHtml,
                      sizeof(azAllowedHtml)/sizeof(azAllowedHtml[0]));
}

/*
** Return TRUE if the HTML element given in the argument is a form of
** external reference (i.e. A, IMG, etc).
*/
extern int sqlite3StrNICmp(const char *, const char*, int);
static int isLinkTag(const char *zElem, int nElem){
  return (nElem==1 && 0==sqlite3StrNICmp(zElem,"A",nElem))
      || (nElem==3 && 0==sqlite3StrNICmp(zElem,"IMG",nElem))
      || (nElem==4 && 0==sqlite3StrNICmp(zElem,"CITE",nElem));
}

/*
** If the input string begins with "<html>" and contains "</html>" somewhere
** before it ends, then return the number of characters through the end of
** the </html>.  If the <html> or the </html> is missing, return 0.
*/
extern int sqlite3StrNICmp(const char *, const char*, int);
static int is_html(const char *z){
  int i;
  if( sqlite3StrNICmp(z, "<html>", 6) ) return 0;
  for(i=6; z[i]; i++){
    if( z[i]=='<' && sqlite3StrNICmp(&z[i],"</html>",7)==0 ) return i+7;
  }
  return 0;
}

/*
** Output nText characters zText as HTML.  Do not allow markup other
** than the markup for which isAllowed() returns true.
**
** In the case of tags with external links, ensure they have a rel="nofollow"
** attribute when g.noFollow is set.
**
** FIXME: would be nice to translate relative URL targets if g.zLinkURL!=0
*/
static void output_restricted_html(const char *zText, int nText){
  int i, j, k;
  for(i=0; i<nText; i++){
    if( zText[i]!='<' ) continue;
    if( i+1<nText ){
      k = 1 + (zText[i+1]=='/');
      for(j=k; i+j<nText && isalnum(zText[i+j]); j++){}
      if( isAllowed(&zText[i+k], j-k)
          && isAllowedAttr(&zText[i+j],nText-(i+j)) ){
        if( g.noFollow && zText[i+j]!='>' && isLinkTag(&zText[i+k],j-k) ){
          /* link tags are special. We want to allow them
          ** but in order to discourage wiki spam we want to insert
          ** something in the attributes... Note that we don't bother
          ** when the tag doesn't have attributes.
          */
          cgi_append_content(zText,i + j);
          zText += i+j;
          nText -= i+j;
          cgi_printf(" rel=\"nofollow\" ");
          i = -1;
        }
        continue;
      }
    }
    cgi_append_content(zText,i);
    cgi_printf("&lt;");
    zText += i+1;
    nText -= i+1;
    i = -1;
  }
  cgi_append_content(zText,i);
}

/*
** Output a formatted ticket link
*/
void output_ticket(int tn, int rn){
  if( g.okRead ){
    char *zLink = (rn>0) ? format_link("tktview?tn=%d,%d",tn,rn)
                         : format_link("tktview?tn=%d",tn);
    if( g.okTicketLink ) {
      char **az = db_query(
        "SELECT title,status FROM ticket WHERE tn=%d", tn);
      if( az && az[0] && az[1] ){
        cgi_printf("<a href=\"%z\" title=\"%h\">",zLink,az[0]);
        /* FIXME: should use a <span> with the ticket status as a class */
        if( az[1][0] == 'n' || az[1][0] == 'a' ){
          cgi_printf("#%d",tn);
        }else{
          cgi_printf("<strike>#%d</strike>",tn);
        }
        cgi_printf("</a>");
        return;
      }
    }
    cgi_printf("<a href=\"%z\">#%d</a>",zLink,tn);
  }else{
    cgi_printf("#%d",tn);
  }
}

/*
** Output a formatted checkin link
*/
void output_chng(int cn){
  if( g.okRead ){
    char *zLink = format_link("chngview?cn=%d",cn);
    if( g.okCheckinLink ){
      char **az = db_query(
           "SELECT milestone,user,message,branch FROM chng WHERE cn=%d", cn);
      if( az && az[0] && az[1] && az[2] ){
        if( az[0][0] && az[0][0] != '0' ){
          cgi_printf("<a href=\"%z\"",zLink);
          cgi_printf("title=\"Milestone [%d] %h (By %h)\">",
                     cn, az[2], az[1]);
          cgi_printf("[%d]</a>", cn);
        }else{
          char *z = az[2];
          int trimmed;

          /* Mozilla and Firefox are quite sensitive to newlines
          ** in link titles so we can't use '@' formatting here.
          */
          cgi_printf("<a href=\"%z\" title=\"Check-in [%d]", zLink,cn);
          if( az[3] && az[3][0] ){
            cgi_printf("on branch %h", az[3]);
          }
          cgi_printf(":");  /* want this tight to last text */
          trimmed = output_trim_message(z, MN_CKIN_MSG, MX_CKIN_MSG);
          cgi_printf("%h%s (By %h)\">[%d]</a>", z, trimmed?"...":"",
                     az[1],cn);
        }
        return;
      }
    }
    cgi_printf("<a href=\"%z\">[%d]</a>", zLink, cn);
  }else{
    cgi_printf("[%d]", cn);
  }
}

/*
** Replace single quotes and backslashes with spaces.
*/
static void sanitize_string( char *z ){
  int i;
  for( i=0;z && z[i]; i++){
    if( z[i] == '\'' || z[i] == '\\' ) {
      z[i] = ' ';
    }
  }
}

static char *markup_substitution(
  int strip_quotes,
  const char *zF,
  const Markup* sMarkup,
  const char *zInBlock,
  int lenBlock
){
  char *zOutput = NULL;
  unsigned const char *zFormat = (unsigned const char*)zF;
  char *azStrings[256];
  int  anLens[256];
  int j, k;

  /* If we don't treat args as blank where there aren't any,
  ** we can't create rules like <b>%k %a</b> that work
  ** with both {markup: this} and {markup: this is} formats. This
  ** is a fairly common convention with most of the existing markups.
  ** We strdup() the blank string because we _will_ free it when
  ** we leave this subroutine.
  */
  char *zArgs = (sMarkup->zArgs==sMarkup->zKey)
                ? strdup("")
                : mprintf("%.*s", sMarkup->lenArgs, sMarkup->zArgs );
  char *zMarkup = mprintf("%.*s", sMarkup->lenType, sMarkup->zType );
  char *zKey = mprintf("%.*s", sMarkup->lenKey, sMarkup->zKey );
  char *zBlock = mprintf("%.*s", lenBlock, zInBlock );
  const char *zRoot = db_config("cvsroot", "");

  if( strip_quotes ){
    /* if we're dealing with a program markup, strip out
    ** backslashes and quotes. This is why we can't just use
    ** "subst".
    */
    sanitize_string(zMarkup);
    sanitize_string(zKey);
    sanitize_string(zArgs);
    sanitize_string(zBlock);
  }

  memset( anLens, 0, sizeof(anLens) );
  memset( azStrings, 0, sizeof(azStrings) );

  azStrings['%'] = "%";
  anLens['%'] = 1;

  /* markup name substitution */
  azStrings['m'] = zMarkup;
  anLens['m'] = sMarkup->lenType;

  /* key substitution */
  azStrings['k'] = zKey;
  anLens['k'] = sMarkup->lenKey;

  /* block substitution */
  azStrings['b'] = zBlock;
  anLens['b'] = lenBlock;

  /* argument substitution. args isn't necessarily the same as
  ** sMarkup->zArgs. */
  azStrings['a'] = zArgs;
  anLens['a'] = strlen(zArgs);

  /* argument substitution. args isn't necessarily the same as
  ** sMarkup->zArgs. */
  azStrings['x'] = zArgs[0] ? zArgs : zKey;
  anLens['x'] = zArgs[0] ? strlen(zArgs) : sMarkup->lenKey;

  /* cvsroot */
  azStrings['r'] = (char*)zRoot;
  anLens['r'] = strlen(zRoot);

  /* basename... from this someone can get the db name */
  azStrings['n'] = (char*)g.zName;
  anLens['n'] = strlen(g.zName);

  /* logged in user */
  azStrings['u'] = (char*)g.zUser;
  anLens['u'] = strlen(g.zUser);

  /* capabilities */
  azStrings['c'] = db_short_query(
      "SELECT capabilities FROM user WHERE id='%q'",g.zUser);
  anLens['c'] = azStrings['c'] ? strlen(azStrings['c']) : 0;

  /* Calculate the space needed for the % subs.
  */
  for(k=j=0; zFormat[j]; j++){
    if( zFormat[j] == '%' && anLens[zFormat[j+1]] ){
      j ++;
      k += anLens[zFormat[j]];
      continue;
    }
    k ++;
  }

  /* (over)allocate an output buffer. By "over", I mean we get
  ** the length of the original plus the length we think we need
  ** for a fully substituted buffer.
  */
  zOutput = malloc(j + k + 1);
  if( zOutput == NULL ){
    free(zKey);
    free(zArgs);
    free(zMarkup);
    free(zBlock);
    if(azStrings['c']) free(azStrings['c']);
    return NULL;
  }

  /* actually perform the substitutions */
  for(k=j=0; zFormat[j]; j++){
    if( zFormat[j] == '%' && azStrings[zFormat[j+1]]!=0 ){
      j ++;
      memcpy(&zOutput[k],azStrings[zFormat[j]],anLens[zFormat[j]]);
      k += anLens[zFormat[j]];
      continue;
    }
    zOutput[k++] = zFormat[j];
  }
  zOutput[k] = 0;

  free(zKey);
  free(zArgs);
  free(zMarkup);
  free(zBlock);
  if(azStrings['c']) free(azStrings['c']);
  return zOutput;
}

/*
** Run the block (if any) out through the standard input of the pipeline
** and feed the output of the pipeline into the CGI output.
**
** It's assumed that zPipeline has been sanitized and stuff.
*/
static void pipe_block(
  const char *zPipeline,
  const char *zBlock,
  int lenBlock
){
  char zFile[PATH_MAX];
  char *zB = mprintf("%.*s",lenBlock,zBlock);
  FILE *fin = NULL;
  char *zP;

  /* Doing this without a temporary file is a bit nasty because of
  ** potential deadlocks. It _can_ be done if you want to fight with
  ** pipe(2) and stuff, but CVSTrac already has a write_to_temp() function
  ** so we might as well be lazy and use it. Note that if lenBlock==0
  ** we can just skip out using /dev/null.
  */

  zFile[0] = 0;
  if( lenBlock==0 ) {
    /* In case the program takes arguments from the command line, we
    ** don't want to just treat it as a no-op. So pipe in /dev/null.
    */
    zP = mprintf( "%s </dev/null", zPipeline );
  }else if( !write_to_temp( zB, zFile, sizeof(zFile) ) ){
    zP = mprintf( "%s <%s", zPipeline, zFile );
  }else{
    if( zB ) free(zB);
    return;
  }

  /* Block has been written, free so we don't forget later
  */
  if( zB ) free(zB);

  fin = popen(zP,"r");
  free(zP);

  /* HTML scrubbing doesn't work effectively on just individual lines. We
  ** really need to feed in the entire buffer or we're vulnerable to all
  ** sorts of whitespace stupidity.
  */
  zP = common_readfp(fin);
  if( zP ){
    output_restricted_html(zP, strlen(zP));
    free( zP );
  }

  if( fin ){
    pclose(fin);
  }

  if( zFile[0] ){
    unlink(zFile);
  }
}

/*
** Output Wiki text while inserting the proper HTML control codes.
** The following formatting conventions are implemented:
**
**    *    Characters with special meaning to HTML are escaped.
**
**    *    Blank lines results in a paragraph break.
**
**    *    Paragraphs where the first line is indented by two or more
**         spaces are shown verbatim.  None of the following rules apply
**         to verbatim text.
**
**    *    Lines beginning with "*: " begin a bullet in a bullet list.
**
**    *    Lines beginning with "1: " begin an item in an enumerated list.
**
**    *    Paragraphs beginning with "_: " are indented.
**
**    *    Multiple colons can be used in *:, 1:, and _: for multiple
**         levels of indentation.
**
**    *    Text within _..._ is italic and text in *...* is bold.  
**         Text within **...** or ***...*** bold with a larger font.
**         Text within =...= is fixed (code) font.
**
**    *    Wiki pages names (Words in initial caps) are enclosed in an
**         appropriate hyperlink.
**
**    *    Words that begin with "http:", "https:", "ftp:", or "mailto:"
**         are enclosed in an appropriate hyperlink.
**
**    *    Text of the form "#NNN" where NNN is a valid ticket number
**         is converted into a hyperlink to the corresponding ticket.
**
**    *    Text of the form "[NNN]" where NNN is a valid check-in number
**         becomes a hyperlink to the checkin.
**
**    *    {quote: XYZ} renders XYZ with all special meanings for XYZ escaped.
**
**    *    {link: URL TEXT} renders TEXT with a link to URL.  URL can be 
**         relative.
**
**    *    {linebreak} renders a linebreak.
**
**    *    {image: URL ALT} renders an in-line image from URL.  URL can be
**         relative or it can be the name of an attachment to zPageId.
**         {leftimage: URL ALT} and {rightimage: URL ALT} create wrap-around
**         images at the left or right margin.
**
**    *    {clear} skips down the page far enough to clear any wrap-around
**         images.
**
**    *    {report: RN CAPTION} inlines the specified report, RN.
**         {leftreport: RN CAPTION} and {rightreport: RN CAPTION} are also
**         usable.
**
**    *    Text between <html>...</html> is interpreted as HTML.  A restricted
**         subset of tags are supported - things like forms and javascript are
**         intentionally excluded.  The initial <html> must occur at the
**         beginning of a paragraph.
*/
void output_wiki(
  const char *zText,          /* The text to be formatted */
  const char *zLinkSuffix,    /* Suffix added to hyperlinks to Wiki */
  const char *zPageId         /* Name of current page */
){
  int i, j, k;
  int aList[20];         /* See adjust_list_nesting for details */
  int inPRE = 0;
  int inB = 0;
  int inI = 0;
  int inT = 0;
  int inTab = 0;
  int v;
  int wordStart = 1;     /* At the start of a word */
  int lineStart = 1;     /* At the start of a line */
  int paraStart = 1;     /* At the start of a paragraph */
  const char *zEndB;     /* Text used to end a run of bold */
  char **azAttach;       /* Attachments to zPageId */
  const char *zExtLink = "<font color=\"#a0a0a0\">&curren;</font>";
  static int once = 1;
  static int nTicket, nCommit;
  if( once ){
    nTicket = atoi(db_short_query("SELECT max(tn) FROM ticket"));
    nCommit = atoi(db_short_query("SELECT max(cn) FROM chng"));
    once = 0;
  }

  i = 0;
  aList[0] = 0;
  azAttach = 0;
  zEndB = "";
  while( zText[i] ){
    char *z;
    int n;
    Markup sMarkup;
    int c = zText[i];

    /* Text between <html>...</html> is interpreted as HTML.
    */
    if( c=='<' && (n = is_html(&zText[i]))>0 ){
      put_htmlized_text(&zText, i);
      zText += 6;
      cgi_printf("<div>");
      output_restricted_html(zText, n-13);
      cgi_printf("</div>");
      zText += n - 6;
      i = 0;
      continue;
    }

    /* Markup may consist of special strings contained in curly braces.
    ** Examples:  "{linebreak}"  or "{quote: *:}"
    */
    if( c=='{' && is_markup(&zText[i], &sMarkup) ){
      /*
      ** Markup of the form "{linebreak}" forces a line break.
      */
      if( sMarkup.lenType==9 && strncmp(sMarkup.zType,"linebreak",9)==0 ){
        put_htmlized_text(&zText, i);
        zText += sMarkup.lenTotal;
        i = 0;
        cgi_printf("<br>\n");
        wordStart = lineStart = paraStart = 0;
        continue;
      }

      /*
      ** Markup of the form "{clear}" moves down past any left or right
      ** aligned images.
      */
      if( sMarkup.lenType==5 && strncmp(sMarkup.zType,"clear",5)==0 ){
        put_htmlized_text(&zText, i);
        zText += sMarkup.lenTotal;
        i = 0;
        cgi_printf("<br clear=\"both\">\n");
        wordStart = lineStart = paraStart = 0;
        continue;
      }

      /*
      ** Markup of the form "{quote: ABC}" writes out the text ABC exactly
      ** as it appears.  This can be used to escape special meanings 
      ** associated with ABC.
      */
      if( sMarkup.lenType==5 && strncmp(sMarkup.zType,"quote",5)==0 ){
        put_htmlized_text(&zText, i);
        if( sMarkup.zKey==sMarkup.zArgs ){
          n = sMarkup.lenKey;
        }else{
          n = &sMarkup.zArgs[sMarkup.lenArgs] - sMarkup.zKey;
        }
        put_htmlized_text(&sMarkup.zKey, n);
        zText += sMarkup.lenTotal;
        i = 0;
        wordStart = lineStart = paraStart = 0;
        continue;
      }

      /*
      ** Markup of the form "{wiki: NAME TEXT}" creates a hyperlink
      ** to wiki page. The hyperlink appears on the screen as TEXT.
      */
      if( sMarkup.lenType==4 && strncmp(sMarkup.zType,"wiki",4)==0 ){
        int exists = 1;
        put_htmlized_text(&zText, i);
        if( g.okRdWiki ) {
          char *zPage = mprintf("%.*s", sMarkup.lenKey, sMarkup.zKey);
          exists = db_exists("SELECT 1 FROM wiki WHERE name='%q'", zPage);
          if( !exists ) cgi_printf("<em>");
          cgi_printf("<a href=\"wiki?p=%t%s\">", zPage, zLinkSuffix);
          free(zPage);
        }
        put_htmlized_text(&sMarkup.zArgs, sMarkup.lenArgs);
        if( g.okRdWiki ) {
          cgi_printf("</a>");
          if( !exists ) cgi_printf("</em>");
        }
        zText += sMarkup.lenTotal;
        i = 0;
        wordStart = lineStart = paraStart = 0;
        continue;
      }

      /*
      ** Markup of the form "{link: TO TEXT}" creates a hyperlink to TO.
      ** The hyperlink appears on the screen as TEXT.  TO can be a any URL,
      ** including a relative URL such as "chngview?cn=123".
      */
      if( sMarkup.lenType==4 && strncmp(sMarkup.zType,"link",4)==0 ){
        put_htmlized_text(&zText, i);
        if( is_url(sMarkup.zKey)>0 ){
          cgi_printf("%s<a href=\"%.*s\"%s>",
                     zExtLink, sMarkup.lenKey, sMarkup.zKey,
                     g.noFollow ? " rel=\"nofollow\"" : "");
        }else{
          char *zLink = format_link("%.*s", sMarkup.lenKey, sMarkup.zKey);
          cgi_printf("<a href=\"%z\">", zLink);
        }
        put_htmlized_text(&sMarkup.zArgs, sMarkup.lenArgs);
        cgi_printf("</a>");
        zText += sMarkup.lenTotal;
        i = 0;
        wordStart = lineStart = paraStart = 0;
        continue;
      }

      /*
      ** Markup of the form "{image: URL ALT}" creates an in-line image to
      ** URL with ALT as the alternate text.  URL can be relative (for example
      ** the URL of an attachment.
      **
      ** If the URL is the name of an attachment, then automatically
      ** convert it to the correct URL for that attachment.
      */
      if( (sMarkup.lenType==5 && strncmp(sMarkup.zType,"image",5)==0)
       || (sMarkup.lenType==9 && strncmp(sMarkup.zType,"leftimage",9)==0)
       || (sMarkup.lenType==10 && strncmp(sMarkup.zType,"rightimage",10)==0)
      ){
        char *zUrl = 0;
        const char *zAlign;
        char *zAlt = htmlize(sMarkup.zArgs, sMarkup.lenArgs);
        if( azAttach==0 && zPageId!=0 ){
          azAttach = (char **)
                     db_query("SELECT fname, atn FROM attachment "
                              "WHERE tn='%q'", zPageId);
        }
        if( azAttach ){
          int ix;
          for(ix=0; azAttach[ix]; ix+=2){
            if( strncmp(azAttach[ix],sMarkup.zKey,sMarkup.lenKey)==0 ){
              free(zUrl);
              zUrl = format_link("attach_get/%s/%h",
                                 azAttach[ix+1], azAttach[ix]);
              break;
            }
          }
        }
        if( zUrl==0 ){
          zUrl = htmlize(sMarkup.zKey, sMarkup.lenKey);
        }
        put_htmlized_text(&zText, i);
        switch( sMarkup.zType[0] ){
          case 'l': case 'L':   zAlign = " align=\"left\"";  break;
          case 'r': case 'R':   zAlign = " align=\"right\""; break;
          default:              zAlign = "";                 break;
        }
        cgi_printf("<img src=\"%s\" alt=\"%s\"%s>", zUrl, zAlt, zAlign);
        free(zUrl);
        free(zAlt);
        zText += sMarkup.lenTotal;
        i = 0;
        wordStart = lineStart = paraStart = 0;
        continue;
      }

      /*
      ** Markup of the form "{report: RN}" embeds a report into the output.
      */
      if( (sMarkup.lenType==6 && strncmp(sMarkup.zType,"report",6)==0)
          || (sMarkup.lenType==11 && strncmp(sMarkup.zType,"rightreport",11)==0)
          || (sMarkup.lenType==10 && strncmp(sMarkup.zType,"leftreport",10)==0)
      ){
        char *zCaption = mprintf("%.*s", sMarkup.lenArgs, sMarkup.zArgs);
        char *zAlign = 0;
        if( sMarkup.lenType==11 ){
          zAlign = "align=\"right\"";
        }else if( sMarkup.lenType==10 ){
          zAlign = "align=\"left\"";
        }
        put_htmlized_text(&zText, i);
        embed_view( atoi(sMarkup.zKey),
                    (sMarkup.zArgs==sMarkup.zKey) ? "" : zCaption,
                    zAlign );
        free(zCaption);
        zText += sMarkup.lenTotal;
        i = 0;
        wordStart = lineStart = paraStart = 0;
        continue;
      }

      /* Markup of the form "{markups}" outputs the list of custom markups
      ** formats with descriptions.
      */
      if( sMarkup.lenType==7 && strncmp(sMarkup.zType,"markups",7)==0 ){
        char **azMarkup;
        put_htmlized_text(&zText, i);

        azMarkup = db_query(
              "SELECT markup, description FROM markup ORDER BY markup;");
        if( azMarkup && azMarkup[0] ){
          @ <p><big><b>Custom Markup Rules</b></big></p>
          @ <p>The following are custom markup rules implemented
          @ on this server.</p>
          for(j=0; azMarkup[j]; j+=2){
            if( azMarkup[j+1] && azMarkup[j+1][0] ){
              /* this markup has a description, output it.
              */
              @ <p>
              output_formatted(azMarkup[j+1],NULL);
              @ </p>
            }else{
              @ <p>{%h(azMarkup[j])} (no description)</p>
            }
          }
        }

        zText += sMarkup.lenTotal;
        i = 0;
        wordStart = lineStart = paraStart = 0;
        continue;
      }

      /* It could be custom markup. There are two kinds of custom markups
      ** available. The first is a simple format string such
      ** "key=%k args=%a" where %k is replaced by the markup key and %a
      ** by any following arguments. More flexibility would probably be
      ** nice, but that's how the existing markup logic works. The second
      ** form of markup is an external executable which gets passed the
      ** key and args on the command line and any output is dumped right
      ** into the output stream.
      */
      if( sMarkup.zType && sMarkup.lenType ) {
        /* sMarkup.zType is a pointer into the text buffer, not a NUL
        ** terminated token. This is actually the case with everything
        ** in sMarkup. Note that the markup type is already checked to
        ** be only chars that pass isalpha() so we can avoid "%.*q".
        */
        char **azMarkup = db_query(
          "SELECT type,formatter FROM markup WHERE markup='%.*s';",
          sMarkup.lenType, sMarkup.zType);

        if( azMarkup && azMarkup[0] && azMarkup[1] ){
          /* We've found a custom formatter for this type */

          int bl = sMarkup.lenTotal;
          int cl = 0;
          int type = atoi(azMarkup[0]);
          char *zOutput;

          put_htmlized_text(&zText, i);

          /* handle blocks. This basically means we scan ahead to find
          ** "end<markup>. bl becomes the total length of the block
          ** and cl is everything up the the {end<markup>}. If we can't
          ** find a match, bl becomes zero and we end up just outputting
          ** the raw markup tag.
          */
          if( type==2 || type==3 ){
            char *zEnd = mprintf("{end%.*s}", sMarkup.lenType, sMarkup.zType);
            int el = strlen(zEnd);
            while( zText[bl] && strncmp(&zText[bl],zEnd,el)){ bl++; }
            if( zText[bl]!=0 ){
              /* found a matching end tag. Note that bl includes the
              ** length of the initial markup which is not part of the
              ** actual content. Fix that. bl doesn't include the length
              ** of the end markup tag. Fix that too.
              */
              cl = bl - sMarkup.lenTotal;
              bl += el;
            } else {
              /* that didn't work, restore to original value.
              */
              bl = sMarkup.lenTotal;
            }
            free(zEnd);
          }

          /* Substitutions are basically the same for all types of
          ** formatters, except that quotes are stripped from arguments
          ** to programs.
          */
          zOutput = markup_substitution( (type==1 || type==3),
            azMarkup[1], &sMarkup, &zText[sMarkup.lenTotal], cl );
          if( bl && zOutput ){
            if( type == 0 || type == 2 ){
              output_restricted_html(zOutput, strlen(zOutput));
            }else if( type==1 || (type==3 && cl==0) ){
              pipe_block(zOutput, "", 0);
            }else if( type==3 ){
              pipe_block(zOutput, &zText[sMarkup.lenTotal], cl);
            }

            free(zOutput);
          }

          zText += bl;
          i = 0;
          wordStart = lineStart = paraStart = 0;
          continue;
        }
      }
    }

    if( paraStart ){
      put_htmlized_text(&zText, i);

      /* Blank lines at the beginning of a paragraph are ignored.
      */
      if( isspace(c) && (j = is_blank_line(&zText[i]))>0 ){
        zText += j;
        continue;
      }

      /* If the first line of a paragraph begins with a tab or with two
      ** or more spaces, then that paragraph is printed verbatim.
      */
      if( c=='\t' || (c==' ' && (zText[i+1]==' ' || zText[i+1]=='\t')) ){
        if( !inPRE ){
          if( inB ){ cgi_printf(zEndB); inB=0; }
          if( inI ){ cgi_printf("</i>"); inI=0; }
          if( inT ){ cgi_printf("</tt>"); inT=0; }
          if( inTab ){ cgi_printf("</table>"); inTab=0; }
          adjust_list_nesting(aList, 0);
          cgi_printf("<pre>\n");
          inPRE = 1;
        }
      }
    } /* end if( paraStart ) */

    if( lineStart ){
      /* Blank lines in the middle of text cause a paragraph break
      */
      if( isspace(c) && (j = is_blank_line(&zText[i]))>0 ){
        put_htmlized_text(&zText, i);
        zText += j;
        if( inB ){ cgi_printf(zEndB); inB=0; }
        if( inI ){ cgi_printf("</i>"); inI=0; }
        if( inT ){ cgi_printf("</tt>"); inT=0; }
        if( inTab ){ cgi_printf("</table>"); inTab=0; }
        if( inPRE ){ cgi_printf("</pre>\n"); inPRE = 0; }
        is_list_elem(zText, &k);
        if( abs(k)<aList[0] ) adjust_list_nesting(aList, k);
        if( zText[0]!=0 ){ cgi_printf("\n<p>"); }
        wordStart = lineStart = paraStart = 1;
        i = 0;
        continue;
      }
    } /* end if( lineStart ) */

    if( lineStart && !inPRE ){
      /* If we are not in verbatim text and a line begins with "*:", then
      ** generate a bullet.  Or if the line begins with "NNN:" where NNN
      ** is a number, generate an enumeration item.
      */
      if( (j = is_list_elem(&zText[i], &k))>0 ){
        put_htmlized_text(&zText, i);
        adjust_list_nesting(aList, k);
        if( inTab ){ cgi_printf("</table>"); inTab=0; }
        if( zText[0]!='_' ) cgi_printf("<li>");
        zText += j;
        i = 0;
        wordStart = 1;
        lineStart = paraStart = 0;
        continue;
      }

      /* Four or more "-" characters on at the beginning of a line that
      ** contains no other text results in a horizontal rule.
      */
      if( (c=='-' || c=='=') && (j = is_horizontal_rule(&zText[i]))>0 ){
        put_htmlized_text(&zText, i);
        adjust_list_nesting(aList, 0);
        if( inTab ){ cgi_printf("</table>"); inTab=0; }
        cgi_printf("<hr>\n");
        zText += j;
        if( *zText ) zText++;
        i = 0;
        lineStart = wordStart = 1;
        paraStart = 1;
        continue;
      }

      /* '|' at the start of a line may be a table
      */
      if( c=='|' && (j = is_table_row(&zText[i]))>0 ){
        put_htmlized_text(&zText, i);
        adjust_list_nesting(aList, 0);
        if( !inTab ){
          cgi_printf("<table border=\"1\" cellspacing=\"0\">\n");
          inTab = 1;
        }
        output_table_row(zText,j);
        zText += j;
        i = 0;
        wordStart = 1;
        lineStart = paraStart = 0;
        continue;
      }
    } /* end if( lineStart && !inPre ) */

    if( wordStart && !inPRE ){
      /* A wiki name at the beginning of a word which is not in verbatim
      ** text generates a hyperlink to that wiki page.
      ** 
      ** Special case: If the name is in CamelCase but ends with a "_", then
      ** suppress the "_" and do not generate the hyperlink.  This allows
      ** CamelCase words that are not wiki page names to appear in text.
      */
      if( g.okRdWiki && isupper(c) && (j = is_wiki_name(&zText[i]))>0 ){
        int exists = db_exists("SELECT 1 FROM wiki WHERE name='%.*s'",
                               j, &zText[i]);
        put_htmlized_text(&zText, i);
        if( !exists ) cgi_printf("<em>");
        cgi_printf("<a href=\"%z\">%.*s</a>",
            format_link("wiki?p=%.*s%s", j, zText, zLinkSuffix),
            j, zText);
        if( !exists ) cgi_printf("</em>");
        zText += j;
        i = 0;
        wordStart = lineStart = paraStart = 0;
        continue;
      }

      if( g.okCheckout && c=='/' && (j = is_repository_file(&zText[i]))>0 ){
        char *zFile;
        put_htmlized_text(&zText, i);
        zFile = mprintf("%.*s", j-1, zText+1);
        cgi_printf("<a href=\"%z\">/%h</a>",
            format_link("rlog?f=%T", zFile), zFile);
        free(zFile);
        zText += j;
        i = 0;
        wordStart = lineStart = paraStart = 0;
        continue;
      }

      /* A "_" at the beginning of a word puts us into an italic font.
      */
      if( c=='_' && !inB && !inI && !inT && font_terminator(&zText[i+1],c,1) ){
        put_htmlized_text(&zText, i);
        i = 0;
        zText++;
        cgi_printf("<i>");
        inI = 1;
        continue;
      }

      /* A "=" at the beginning of a word puts us into an fixed font.
      */
      if( c=='=' && !inB && !inI && !inT && font_terminator(&zText[i+1],c,1) ){
        put_htmlized_text(&zText, i);
        i = 0;
        zText++;
        cgi_printf("<tt>");
        inT = 1;
        continue;
      }

      /* A "*" at the beginning of a word puts us into a bold font.
      */
      if( c=='*' && !inB && !inI && !inT && (j = count_stars(&zText[i]))>=1
              && j<=3 && font_terminator(&zText[i+j],c,j) ){
        const char *zBeginB = "";
        put_htmlized_text(&zText, i);
        i = 0;
        zText += j;
        switch( j ){
          case 1: zBeginB = "<b>";           zEndB = "</b>";             break;
          case 2: zBeginB = "<big><b>";      zEndB = "</b></big>";       break;
          case 3: zBeginB = "<big><big><b>"; zEndB = "</b></big></big>"; break;
        }
        cgi_printf(zBeginB);
        inB = j;
        continue;
      }


      /* Words that begin with "http:" or "https:" or "ftp:" or "mailto:"
      ** become hyperlinks.
      */
      if( (c=='h' || c=='f' || c=='m') && (j=is_url(&zText[i]))>0 ){
        put_htmlized_text(&zText, i);
        z = htmlize(zText, j);
        if( is_image(z, strlen(z)) ){
          cgi_printf("<img src=\"%s\" alt=\"%s\"%s>", z, z,
                     g.noFollow ? " rel=\"nofollow\"" : "");
        }else{
          cgi_printf("%s<a href=\"%s\"%s>%s</a>",
                     zExtLink, z,
                     g.noFollow ? " rel=\"nofollow\"" : "", z);
        }
        free(z);
        zText += j;
        i = 0;
        wordStart = lineStart = paraStart = 0;
        continue;
      }

      /* If the user has read permission on tickets and a word is of the
      ** form "#NNN" where NNN is a sequence of digits, then generate a
      ** hyperlink to ticket number NNN.
      */
      if( c=='#' && g.okRead && (j = ndigit(&zText[i+1]))>0 
                 && is_eow(&zText[i+1+j],0)
                 && (v = atoi(&zText[i+1]))>0 && v<=nTicket ){
        put_htmlized_text(&zText, i);
        output_ticket(v,0);
        zText += j;
        if( *zText ) zText++;
        i = 0;
        wordStart = lineStart = paraStart = 0;
        continue;
      }

      /* If the user has checkout permissions and a word is of the form
      ** "[NNN]" where NNN is a checkin number, then generate a hyperlink
      ** to check-in NNN.
      */
      if( c=='[' && g.okRead && (j = ndigit(&zText[i+1]))>0
                 && is_eow(&zText[i+j+2],0)
                 && (v = atoi(&zText[i+1]))>0 && v<=nCommit 
                 && zText[i+j+1]==']' ){
        put_htmlized_text(&zText, i);
        output_chng(v);
        zText += j+1;
        if( *zText ) zText++;
        i  = 0;
        wordStart = lineStart = paraStart = 0;
        continue;
      }
    } /* end if( wordStart && !inPre ) */

    /* A "*", "=", or a "_" at the end of a word takes us out of bold,
    ** fixed or italic mode.
    */
    if( inB && c=='*' && !isspace(zText[i-1]) && zText[i-1]!='*' &&
            (j = count_stars(&zText[i]))==inB && is_eow(&zText[i+j],0) ){
      inB = 0;
      put_htmlized_text(&zText, i);
      i = 0;
      zText += j;
      cgi_printf(zEndB);
      continue;
    }
    if( inT && c=='=' && !isspace(zText[i-1]) && is_eow(&zText[i+1],0) ){
      put_htmlized_text(&zText, i);
      i = 0;
      zText++;
      inT = 0;
      cgi_printf("</tt>");
      continue;
    }
    if( inI && c=='_' && !isspace(zText[i-1]) && is_eow(&zText[i+1],0) ){
      put_htmlized_text(&zText, i);
      i = 0;
      zText++;
      inI = 0;
      cgi_printf("</i>");
      continue;
    }
    if( wordStart ){
      wordStart = isspace(c) || c=='(' || c=='"';
    }else{
      wordStart = isspace(c);
    }
    lineStart = c=='\n';
    paraStart = 0;
    i++;
  }
  if( zText[0] ) cgi_printf("%h", zText);
  if( inB ) cgi_printf("%s\n",zEndB);
  if( inT ) cgi_printf("</tt>\n");
  if( inI ) cgi_printf("</i>\n");
  if( inTab ){ cgi_printf("</table>"); inTab=0; }
  adjust_list_nesting(aList, 0);
  if( inPRE ) cgi_printf("</pre>\n");
}

/*
** Output text while inserting hyperlinks to ticket and checkin reports.
** Within the text, an occurance of "#NNN" (where N is a digit) results
** in a hyperlink to the page that shows that ticket.  Any occurance of
** [NNN] gives a hyperlink to check-in number NNN.
**
** (Added later:)  Also format the text as HTML.  Insert <p> in place
** of blank lines.  Insert <pre>..</pre> around paragraphs that are
** indented by two or more spaces.  Make lines that begin with "*:"
** or "1:" into <ul> or <ol> list elements.
**
** (Later:)  The formatting is now extended to include all of the
** Wiki formatting options.
*/
void output_formatted(const char *zText, const char *zPageId){
  output_wiki(zText,"",zPageId);
}

/* 
** This routine alters a check-in message to make it more readable
** in a timeline.  The following changes are made:
**
** *:  Remove all leading whitespace.  This prevents the text from
**     being display verbatim.
**
** *:  If the message begins with "*:" or "N:" (where N is a number)
**     then strip it out.
**
** *:  Change all newlines to spaces.  This will disable paragraph
**     breaks, verbatim paragraphs, enumerations, and bullet lists.
**
** *:  Replace all internal list markups with '+' followed by spaces.
**     (Otherwise, bullet lists turn into boldface).
**
** *:  Collapse contiguous whitespace into a single space character
**
** *:  Truncate the string at the first whitespace character that
**     is more than mxChar characters from the beginning of the string.
**     Or if the string is longer than mxChar character and but there
**     was a paragraph break after mnChar characters, truncate at the
**     paragraph break.
**
** This routine changes the message in place.  It returns non-zero if
** the message was truncated and zero if the original text is still
** all there (though perhaps altered.)
*/
int output_trim_message(char *zMsg, int mnChar, int mxChar){
  int i, j, k, n;
  int brkpt = 0;    /* First paragraph break after zMsg[mnChar] */

  if( zMsg==0 ) return 0;
  for(i=0; isspace(zMsg[i]); i++){}
  i += is_list_elem(&zMsg[i], &k);
  for(j=0; zMsg[i]; i++){
    int c = zMsg[i];
    if( c=='\n' ){
      if( j>mnChar && is_blank_line(&zMsg[i+1]) && brkpt==0 ){
        brkpt = j;
      }
      c = ' ';
      if( (n = is_list_elem(&zMsg[i+1],&k))>0 ) {
        zMsg[i+1] = '+';
        memset(&zMsg[i+2],' ',n-1);
      }
    }
    if( isspace(c) ){
      if( j>=mxChar ){
        zMsg[j] = 0;
        if( brkpt>0 ) zMsg[brkpt] = 0;
        return 1;
      }
      if( j>0 && !isspace(zMsg[j-1]) ){
        zMsg[j++] = ' ';
      }
    }else{
      zMsg[j++] = c;
    }
  }
  zMsg[j] = 0;
  return 0;
}

/*
** Append HTML text to the output that describes the formatting
** conventions implemented by the output_formatted() function
** above.
*/
void append_formatting_hints(void){
  char **az;
  int j;
  @ <ul>
  @ <li><p>
  @ Blank lines divide paragraphs.
  @ </p></li>
  @
  @ <li><p>
  @ If a paragraph is indented by a tab or by two or more spaces,
  @ it is displayed verbatim -- in a constant-width font with all
  @ spacing and line breaks preserved.
  @ </p></li>
  @
  @ <li><p>
  @ Surround phrases by underscores, asterisks or equals 
  @ for italic, bold or fixed text.
  @ (Ex: "<tt>_italic text_, *bold text*, =fixed text=</tt>")
  @ Use two or three asterisks for bold text in a larger font.
  @ </p></li>
  @
  @ <li><p>
  if( g.okRead ){
    @ Text like "<tt>#123</tt>" becomes a hyperlink to ticket #123.
  }
  if( g.okCheckout ){
    @ Text like "<tt>[456]</tt>" becomes a hyperlink to
    @ check-in [456].
  }
  if( g.okRdWiki ){
    @ An absolute URL, a wiki page name becomes a hyperlink.
    @ Also markup of the form "<tt>{wiki: <i>title text</i>}</tt>"
    @ becomes a hyperlink to the wiki document of <i>title</i>.
  } else {
    @ An absolute URL becomes a hyperlink.
  }
  @ Or use markup of the form:
  @ "<tt>{link: <i>url text</i>}</tt>".
  @ </p></li>
  @
  @ <li><p>
  @ A path to a file in the repository becomes a link to its rlog page:
  @ "<tt>/path/to/format.c</tt>".
  @ </p></li>
  @
  @ <li><p>
  @ The characters "<tt>*:</tt>" or "<tt>1:</tt>" at the beginning of a line
  @ produce a bullet or enumeration list.
  @ Use additional colons for nested lists.
  @ </p></li>
  @
  @ <li><p>
  @ Create a table by wrapping cells with "<tt>|</tt>", starting at the
  @ beginning of a line. Each cell is separated with a "<tt>|</tt>" and
  @ each line should end with a "<tt>|</tt>".
  @ </p></li>
  @
  @ <li><p>
  @ Use "<tt>_:</tt>" at the beginning of a paragraph to indent that
  @ paragraph.  Multiple colons indent more.
  @ </p></li>
  @
  @ <li><p>
  @ Four or more "-" or "=" characters on a line by themselves generate a
  @ horizontal rule (the &lt;hr&gt; markup of HTML).
  @ </p></li>
  @
  @ <li><p>
  @ Create a line-break using "<tt>{linebreak}</tt>".
  @ </p></li>
  @
  @ <li><p>
  @ Use "<tt>{quote: <i>text</i>}</tt>" to display <i>text</i>.
  @ </p></li>
  @
  @ <li><p>
  @ Insert in-line images using "<tt>{image: <i>url</i>}</tt>".
  @ The <i>url</i> can be the filename of an attachment.
  @ </p></li>
  @
  @ <li><p>
  @ Insert in-line reports using "<tt>{report: <i>rn</i>}</tt>". The <i>rn</i>
  @ is the report number (which isn't necessarily the same as the numbers on
  @ the <a href="reportlist">report list</a>).
  @ </p></li>
  @
  @ <li><p>
  @ Text between "<tt>&lt;html&gt;...&lt;/html&gt;</tt>" is interpreted as HTML.
  @ </p></li>
  @

  /* output custom markups.
  */
  az = db_query("SELECT markup, description FROM markup;");
  if( az && az[0] ){
    for(j=0; az[j]; j+=2){
      if( az[j+1] && az[j+1][0] ){
        /* this markup has a description, output it.
        */
        @ <li><p>
        output_formatted(az[j+1],NULL);
        @ </p></li>
      }else{
        @ <li><p>{%h(az[j])} (no description)</p></li>
      }
    }
  }
  @ </ul>
}

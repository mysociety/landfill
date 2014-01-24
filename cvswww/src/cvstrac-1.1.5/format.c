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


/*
** Return the number digits at the beginning of the string z.
*/
static int ndigit(const char *z){
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
*/
static int is_eow(const char *z, const char *zEnd){
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
  return nCap>=2 && is_eow(&z[i],0) ? i : 0;
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
static int is_url(const char *z){
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
  i++;
  while( z[i] && !isspace(z[i]) && z[i]!='}' ){ i++; }
  if( z[i]==0 || z[i]=='\n' ) return 0;
  pMarkup->lenKey = i - j;
  if( z[i]=='}' ){
    pMarkup->lenArgs = i - j;
    pMarkup->lenTotal = i+1;
    pMarkup->zArgs = pMarkup->zKey;
    return 1;
  }
  while( isspace(z[i]) && z[i]!='\n' ){ i++; }
  if( z[i]=='\n' || z[i]==0 ) return 0;
  j = i;
  i++;
  while( z[i] && z[i]!='}' && z[i]!='\n' ){ i++; }
  if( z[i]!='}' ) return 0;
  pMarkup->zArgs = &z[j];
  pMarkup->lenArgs = i - j;
  pMarkup->lenTotal = i+1;
  return 1;
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
** Return TRUE if the HTML element given in the argument is on the allowed
** element list.
*/
static int isAllowed(const char *zElem, int nElem){
  int i;
  int upr, lwr, mid, c;
  char zBuf[12];
  if( nElem>sizeof(zBuf)-1 ) return 0;
  for(i=0; i<nElem; i++) zBuf[i] = tolower(zElem[i]);
  zBuf[i] = 0;
  upr = sizeof(azAllowedHtml)/sizeof(azAllowedHtml[0]) - 1;
  lwr = 0;
  while( upr>=lwr ){
    mid = (upr+lwr)/2;
    c = strcmp(azAllowedHtml[mid],zBuf);
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
** If the input string begins with "<html>" and contains "</html>" somewhere
** before it ends, then return the number of characters through the end of
** the </html>.  If the <html> or the </html> is missing, return 0.
*/
static int is_html(const char *z){
  int i;
  extern int sqliteStrNICmp(const char *, const char*, int);
  if( sqliteStrNICmp(z, "<html>", 6) ) return 0;
  for(i=6; z[i]; i++){
    if( z[i]=='<' && sqliteStrNICmp(&z[i],"</html>",7)==0 ) return i+7;
  }
  return 0;
}

/*
** Output nText characters zText as HTML.  Do not allow markup other
** than the markup for which isAllowed() returns true.
*/
static void output_restricted_html(const char *zText, int nText){
  int i, j, k;
  cgi_printf("<div>");
  for(i=0; i<nText; i++){
    if( zText[i]!='<' ) continue;
    k = 1 + (zText[i+1]=='/');
    for(j=k; isalnum(zText[i+j]); j++){}
    if( isAllowed(&zText[i+k], j-k) ) continue;
    cgi_printf("%.*s&lt;", i, zText);
    zText += i+1;
    nText -= i+1;
    i = -1;
  }
  cgi_printf("%.*s</div>", i, zText);
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
**         Text with in **...** or ***...*** bold with a larger font.
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
  int v;
  int wordStart = 1;     /* At the start of a word */
  int lineStart = 1;     /* At the start of a line */
  int paraStart = 1;     /* At the start of a paragraph */
  const char *zEndB;     /* Text used to end a run of bold */
  char **azAttach;       /* Attachments to zPageId */
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
      output_restricted_html(zText, n-13);
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
        int n;
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
      ** Markup of the form "{link: TO TEXT}" creates a hyperlink to TO.
      ** The hyperlink appears on the screen as TEXT.  TO can be a any URL,
      ** including a relative URL such as "chngview?cn=123".
      */
      if( sMarkup.lenType==4 && strncmp(sMarkup.zType,"link",4)==0 ){
        put_htmlized_text(&zText, i);
        cgi_printf("<a href=\"%.*s\">", sMarkup.lenKey, sMarkup.zKey);
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
              zUrl = mprintf("attach_get/%s/%h",
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
        cgi_printf("<hr>\n");
        zText += j;
        if( *zText ) zText++;
        i = 0;
        lineStart = wordStart = 1;
        paraStart = 1;
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
        put_htmlized_text(&zText, i);
        cgi_printf("<a href=\"wiki?p=%.*s%s\">%.*s</a>",
            j, zText, zLinkSuffix, j, zText);
        zText += j;
        i = 0;
        wordStart = lineStart = paraStart = 0;
        continue;
      }

      /* A "_" at the beginning of a word puts us into an italic font.
      */
      if( c=='_' && !inB && !inI && font_terminator(&zText[i+1],c,1) ){
        put_htmlized_text(&zText, i);
        i = 0;
        zText++;
        cgi_printf("<i>");
        inI = 1;
        continue;
      }

      /* A "*" at the beginning of a word puts us into a bold font.
      */
      if( c=='*' && !inB && !inI && (j = count_stars(&zText[i]))>=1
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
          cgi_printf("<img src=\"%s\" alt=\"%s\">", z, z);
        }else{
          cgi_printf("<a href=\"%s\">%s</a>", z, z);
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
        cgi_printf("<a href=\"tktview?tn=%d\">#%d</a>", v, v);
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
      if( c=='[' && g.okCheckout && (j = ndigit(&zText[i+1]))>0
                 && is_eow(&zText[i+j+2],0)
                 && (v = atoi(&zText[i+1]))>0 && v<=nCommit 
                 && zText[i+j+1]==']' ){
        put_htmlized_text(&zText, i);
        cgi_printf("<a href=\"chngview?cn=%d\">[%d]</a>", v, v);
        zText += j+1;
        if( *zText ) zText++;
        i  = 0;
        wordStart = lineStart = paraStart = 0;
        continue;
      }
    } /* end if( wordStart && !inPre ) */

    /* A "*" or a "_" at the end of a word takes us out of bold or
    ** italic mode.
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
  if( inI ) cgi_printf("</i>\n");
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
**     then convert the ":" into "." to disable the bullet or enumeration.
**
** *:  Change all newlines to spaces.  This will disable paragraph
**     breaks, verbatim paragraphs, enumerations, and bullet lists.
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
  int i, j, k;
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
  @ Surround phrases by underscores or asterisks for italic or bold text.
  @ (Ex: "<tt>_italic text_ and *bold text*</tt>")
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
  @ An absolute URL
  if( g.okRdWiki ){
    @ or a wiki page name 
  }
  @ becomes a hyperlink.  Or use markup of the form:
  @ "<tt>{link: <i>url text</i>}</tt>".
  @ </p></li>
  @
  @ <li><p>
  @ The characters "<tt>*:</tt>" or "<tt>1:</tt>" at the beginning of a line
  @ produce a bullet or enumeration list.
  @ Use additional colons for nested lists.
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
  @ Text between "<tt>&lt;html&gt;...&lt;/html&gt;</tt>" is interpreted as HTML.
  @ </p></li>
  @ </ul>
}

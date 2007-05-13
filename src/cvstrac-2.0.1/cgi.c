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
** This file contains C functions and procedures that provide useful
** services to CGI programs.  There are procedures for parsing and
** dispensing QUERY_STRING parameters and cookies, the "mprintf()"
** formatting function and its cousins, and routines to encode and
** decode strings in HTML or HTTP.
*/
#include <config.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>
#include "cgi.h"

#if INTERFACE
/*
** Shortcuts for cgi_parameter.  P("x") returns the value of query parameter
** or cookie "x", or NULL if there is no such parameter or cookie.  PD("x","y")
** does the same except "y" is returned in place of NULL if there is not match.
*/
#define P(x)        cgi_parameter((x),0)
#define PD(x,y)     cgi_parameter((x),(y))
#define QP(x)       quotable_string(cgi_parameter((x),0))
#define QPD(x,y)    quotable_string(cgi_parameter((x),(y)))

#endif /* INTERFACE */

/*
** Provide a reliable implementation of a caseless string comparison
** function.
*/
#define stricmp sqlite3StrICmp
extern int sqlite3StrICmp(const char*, const char*);

/*
** The body of the HTTP reply text is stored here.
*/
static int nAllocTxt = 0; /* Amount of space allocated for HTTP reply text */
static int nUsedTxt = 0;  /* Amount of space actually used */
static char *zTxt = 0;    /* Pointer to malloced space */

/*
** Append reply content to what already exists.  Return a pointer
** to the start of the appended text.
*/
const char *cgi_append_content(const char *zData, int nAmt){
  if( nUsedTxt+nAmt >= nAllocTxt ){
    nAllocTxt = nUsedTxt*2 + nAmt + 1000;
    zTxt = realloc( zTxt, nAllocTxt );
    if( zTxt==0 ) exit(1);
  }
  memcpy(&zTxt[nUsedTxt], zData, nAmt);
  nUsedTxt += nAmt;
  return &zTxt[nUsedTxt-nAmt];
}

/*
** Reset the HTTP reply text to be an empty string.
*/
void cgi_reset_content(void){
  nUsedTxt = 0;
  g.zLinkURL = 0;
}

/*
** Return a pointer to the HTTP reply text.  The text is reset
*/
char *cgi_extract_content(int *pnAmt){
  char *z;
  *pnAmt = nUsedTxt;
  if( zTxt ){
    zTxt[nUsedTxt] = 0;
  }
  z = zTxt;
  zTxt = 0;
  nAllocTxt = 0;
  nUsedTxt = 0;
  return z;
}

/*
** Additional information used to form the HTTP reply
*/
static char *zContentType = "text/html"; /* Content type of the reply */
static char *zReplyStatus = "OK";        /* Reply status description */
static int iReplyStatus = 200;           /* Reply status code */
static char *zExtraHeader = 0;     /* Extra header text */
static int fullHttpReply = 0;      /* True for a full-blown HTTP header */
static const char *zLogFile = 0;   /* Name of the log file */
static const char *zLogArg = 0;    /* Argument to the log message */

/*
** Set the reply content type
*/
void cgi_set_content_type(const char *zType){
  zContentType = mprintf("%s", zType);
}

/*
** Set the reply status code
*/
void cgi_set_status(int iStat, const char *zStat){
  zReplyStatus = mprintf("%s", zStat);
  iReplyStatus = iStat;
}

/*
** Append text to the header of an HTTP reply
*/
void cgi_append_header(const char *zLine){
  if( zExtraHeader ){
    zExtraHeader = mprintf("%z%s", zExtraHeader, zLine);
  }else{
    zExtraHeader = mprintf("%s", zLine);
  }  
}

/*
** Set a cookie.
**
** Zero lifetime implies a session cookie.
*/
void cgi_set_cookie(
  const char *zName,    /* Name of the cookie */
  const char *zValue,   /* Value of the cookie.  Automatically escaped */
  const char *zPath,    /* Path cookie applies to.  NULL means "/" */
  int lifetime          /* Expiration of the cookie in seconds */
){
  char *zCookie;
  if( zPath==0 ) zPath = "/";
  if( lifetime>0 ){
		lifetime += (int)time(0);
    zCookie = mprintf("Set-Cookie: %s=%t; Path=%s; expires=%s; Version=1\r\n",
                      zName, zValue, zPath, cgi_rfc822_datestamp(lifetime));
  }else{
    zCookie = mprintf("Set-Cookie: %s=%t; Path=%s; Version=1\r\n",
                      zName, zValue, zPath);
  }
  cgi_append_header(zCookie);
  free(zCookie);
}

/*
** This routine sets up the name of a logfile and an argument to the
** log message.  The log message is written when cgi_reply() is invoked.
*/
void cgi_logfile(const char *zFile, const char *zArg){
  if( zFile ) zLogFile = zFile;
  zLogArg = zArg;
}

static char *cgi_add_etag(char *zTxt, int nLen){
  MD5Context ctx;
  unsigned char digest[16];
  int i, j;
  char zETag[64];

  MD5Init(&ctx);
  MD5Update(&ctx,(unsigned char *)zTxt,(unsigned int)nLen);
  MD5Final(digest,&ctx);
  for(j=i=0; i<16; i++,j+=2){
    bprintf(&zETag[j],sizeof(zETag)-j,"%02x",(int)digest[i]);
  }
  cgi_append_header( mprintf("ETag: %s\r\n", zETag) );
  return strdup(zETag);
}

/*
** Do some cache control stuff. First, we generate an ETag and include it in
** the response headers. Second, we do whatever is necessary to determine if
** the request was asking about caching and whether we need to send back the
** response body. If we shouldn't send a body, return non-zero.
**
** Currently, we just check the ETag against any If-None-Match header.
**
** FIXME: In some cases (attachments, file contents) we could check
** If-Modified-Since headers and always include Last-Modified in responses.
*/
static int check_cache_control(void){
  /* FIXME: there's some gotchas wth cookies and some headers. */
  char *zETag = cgi_add_etag(zTxt,nUsedTxt);
  char *zMatch = getenv("HTTP_IF_NONE_MATCH");

  if( zETag!=0 && zMatch!=0 ) {
    char *zBuf = strdup(zMatch);
    if( zBuf!=0 ){
      char *zTok = 0;
      char *zPos;
      for( zTok = strtok_r(zBuf, ",\"",&zPos);
           zTok && strcasecmp(zTok,zETag);
           zTok =  strtok_r(0, ",\"",&zPos)){}
      free(zBuf);
      if(zTok) return 1;
    }
  }
  
  return 0;
}

/*
** Do a normal HTTP reply
*/
void cgi_reply(void){
  FILE *log;
  if( iReplyStatus<=0 ){
    iReplyStatus = 200;
    zReplyStatus = "OK";
  }

  if( iReplyStatus==200 && check_cache_control() ) {
    /* change the status to "unchanged" and we can skip sending the
    ** actual response body. Obviously we only do this when we _have_ a
    ** body (code 200).
    */
    iReplyStatus = 304;
    zReplyStatus = "Not Modified";
  }

  if( fullHttpReply ){
    printf("HTTP/1.0 %d %s\r\n", iReplyStatus, zReplyStatus);
    printf("Date: %s\r\n", cgi_rfc822_datestamp(time(0)));
    printf("Connection: close\r\n");
  }else{
    printf("Status: %d %s\r\n", iReplyStatus, zReplyStatus);
  }

  if( zExtraHeader ){
    printf("%s", zExtraHeader);
  }

  if( g.isConst ){
    /* constant means that the input URL will _never_ generate anything
    ** else. In the case of attachments, the contents won't change because
    ** an attempt to change them generates a new attachment number. In the
    ** case of most /getfile calls for specific versions, the only way the
    ** content changes is if someone breaks the SCM. And if that happens, a
    ** stale cache is the least of the problem. So we provide an Expires
    ** header set to a reasonable period (default: one week).
    */
    time_t expires = time(0) + atoi(db_config("constant_expires","604800"));
    printf( "Expires: %s\r\n", cgi_rfc822_datestamp(expires));
  }

  if( g.isAnon ){
    printf("Cache-control: public\r\n");
  }else{
    /* Content intended for logged in users should only be cached in
    ** the browser, not some shared location.
    */
    printf("Cache-control: private\r\n");
  }

#if CVSTRAC_I18N
  printf( "Content-Type: %s; charset=%s\r\n", zContentType, /*el_langinfo(CODESET)) */ "UTF-8" /* HACK! */ );
#else
  #error We should use I18N version
  printf( "Content-Type: %s; charset=ISO-8859-1\r\n", zContentType);
#endif

  if( iReplyStatus != 304 ) {
    printf( "Content-Length: %d\r\n", nUsedTxt );
  }
  printf("\r\n");
  if( zTxt && iReplyStatus != 304 ){
    fwrite(zTxt, 1, nUsedTxt, stdout);
  }
  if( zLogFile && (log = fopen(zLogFile,"a"))!=0 ){
    time_t now;
    struct tm *pTm;
    const char *zPath;
    const char *zAddr;
    struct tms sTms;
    double rScale;
    char zDate[200];

    if( zLogArg==0 ) zLogArg = "*";
    zPath = getenv("REQUEST_URI");
    if( zPath==0 ) zPath = "/";
    zAddr = getenv("REMOTE_ADDR");
    if( zAddr==0 ) zAddr = "*";
    time(&now);
    pTm = localtime(&now);
    strftime(zDate, sizeof(zDate), "%Y-%m-%d %H:%M:%S", pTm);
    fprintf(log, "%s %s %s %d %s", zDate, zAddr, zPath, iReplyStatus,zLogArg);
    times(&sTms);
    rScale = 1.0/(double)sysconf(_SC_CLK_TCK);
    fprintf(log, " %g %g %g %g\n",
      rScale*sTms.tms_utime,
      rScale*sTms.tms_stime,
      rScale*sTms.tms_cutime,
      rScale*sTms.tms_cstime);
    fclose(log);
  }
}

static int is_same_page(const char *zPage1, const char *zPage2){
  size_t s1 = strcspn(zPage1,"?#");
  size_t s2 = strcspn(zPage2,"?#");
  if( s1 != s2 ) return 0;
  return !strncmp(zPage1,zPage2,s1);
}

/*
** Do a redirect request to the URL given in the argument.
**
** The URL might be relative to the current document.  If so,
** this routine has to translate the URL into an absolute
** before formatting the reply.
*/
void cgi_redirect(const char *zURL){
  char *zLocation;
  if( strncmp(zURL,"http:",5)==0 || strncmp(zURL,"https:",6)==0 || *zURL=='/' ){
    /* An absolute URL.  Do nothing */
  }else{
    int i, j, k=0;
    char *zDest;
    char *zCur = getenv("REQUEST_URI");
    if( zCur==0 ) zCur = "";
    for(i=0; zCur[i] && zCur[i]!='?' && zCur[i]!='#'; i++){}
    if( g.zExtra ){
      /* Skip to start of extra stuff, then pass over any /'s that might
      ** have separated the document root from the extra stuff. This
      ** ensures that the redirection actually redirects the root, not
      ** something deep down at the bottom of a URL.
      */
      i -= strlen(g.zExtra);
      while( i>0 && zCur[i-1]=='/' ){ i--; }
    }
    while( i>0 && zCur[i-1]!='/' ){ i--; }
    zDest = mprintf("%.*s/%s", i, zCur, zURL);
    /* don't touch the protocol stuff, if it exists */
    if( !strncmp(zDest,"http://",7) ){
      k = 7;
    }else if( !strncmp(zDest,"https://",8) ){
      k = 8;
    }
    /* strip out constructs like .., /./, //, etc */
    for(i=j=k; zDest[i]; i++){
      if( zDest[i]=='/' ){
        if( zDest[i+1]=='.' ){
          if( zDest[i+2]=='/' ){
            i += 2;
            continue;
          }
          if( zDest[i+2]=='.' && zDest[i+3]=='/' ){
            if( j==0 ){
              i += 3;
              continue;
            }
            j--;
            while( j>0 && zDest[j-1]!='/' ){ j--; }
            continue;
          }
        }
        if( zDest[i+1]=='/' && (i==0 || zDest[i-1]!=':') ) continue;
      }
      zDest[j++] = zDest[i];
    }
    zDest[j] = 0;
    zURL = zDest;

    /* see if we've got a cycle by matching everything up to the ? or #
    ** in the new and old URLs.
    */
    if( is_same_page(zDest,zCur) ){
      cgi_reset_content();
      cgi_printf("<html>\n<p>Cyclic redirection in %s</p>\n</html>\n", zURL);
      cgi_set_status(500, "Internal Server Error");
      cgi_reply();
      exit(0);
    }
  }

/*
** The lynx browser complains if the "http:" prefix is missing
** from a redirect.  But if we use it, we lose the ability to
** run on a secure server using stunnel.
**
** Relative redirects are used by default.  This works with stunnel.
** Lynx sends us a nasty message, but it still works.  So with
** relative redirects everybody works.  With absolute redirects,
** stunnel will not work.  So the default configuration is to go
** with what works for everybody, even if it happens to be technically
** incorrect.
*/
#ifdef ABSOLUTE_REDIRECT
  {
    char *zHost;
    if( strncmp(zURL,"http:",5)!=0 && strncmp(zURL,"https:",6)!=0 
         && (zHost = getenv("HTTP_HOST"))!=0 ){
      char *zMode = getenv("HTTPS");
      if( zMode && strcmp(zMode,"on")==0 ){
        zURL = mprintf("https://%s%s", zHost, zURL);
      }else{
        zURL = mprintf("http://%s%s", zHost, zURL);
      }
    }
  }
#endif
  zLocation = mprintf("Location: %s\r\n", zURL);
  cgi_append_header(zLocation);
  cgi_reset_content();
  cgi_printf("<html>\n<p>Redirect to %h</p>\n</html>\n", zURL);
  cgi_set_status(302, "Moved Temporarily");
  free(zLocation);
  cgi_reply();
  exit(0);
}

/*
** Information about all query parameters and cookies are stored
** in these variables.
*/
static int nAllocQP = 0; /* Space allocated for aParamQP[] */
static int nUsedQP = 0;  /* Space actually used in aParamQP[] */
static int sortQP = 0;   /* True if aParamQP[] needs sorting */
static struct QParam {   /* One entry for each query parameter or cookie */
  char *zName;              /* Parameter or cookie name */
  char *zValue;             /* Value of the query parameter or cookie */
} *aParamQP;             /* An array of all parameters and cookies */

/*
** Add another query parameter or cookie to the parameter set.
** zName is the name of the query parameter or cookie and zValue
** is its fully decoded value.
**
** zName and zValue are not copied and must not change or be
** deallocated after this routine returns.
*/
static void cgi_set_parameter_nocopy(char *zName, char *zValue){
  if( nAllocQP<=nUsedQP ){
    nAllocQP = nAllocQP*2 + 10;
    aParamQP = realloc( aParamQP, nAllocQP*sizeof(aParamQP[0]) );
    if( aParamQP==0 ) exit(1);
  }
  aParamQP[nUsedQP].zName = zName;
  aParamQP[nUsedQP].zValue = zValue;
  nUsedQP++;
  sortQP = 1;
}

/*
** Add another query parameter or cookie to the parameter set.
** zName is the name of the query parameter or cookie and zValue
** is its fully decoded value.
**
** Copies are made of both the zName and zValue parameters.
*/
void cgi_set_parameter(const char *zName, const char *zValue){
  cgi_set_parameter_nocopy(mprintf("%s",zName), mprintf("%s",zValue));
}


/*
** Add a list of query parameters or cookies to the parameter set.
**
** Each parameter is of the form NAME=VALUE.  Both the NAME and the
** VALUE may be url-encoded ("+" for space, "%HH" for other special
** characters).  But this routine assumes that NAME contains no
** special character and therefore does not decode it.
**
** Parameters are separated by the "terminator" character.  Whitespace
** before the NAME is ignored.
**
** The input string "z" is modified but no copies is made.  "z"
** should not be deallocated or changed again after this routine
** returns or it will corrupt the parameter table.
*/
static void add_param_list(char *z, int terminator){
  while( *z ){
    char *zName;
    char *zValue;
    while( isspace(*z) ){ z++; }
    zName = z;
    while( *z && *z!='=' && *z!=terminator ){ z++; }
    if( *z=='=' ){
      *z = 0;
      z++;
      zValue = z;
      while( *z && *z!=terminator ){ z++; }
      if( *z ){
        *z = 0;
        z++;
      }
      dehttpize(zValue);
      cgi_set_parameter_nocopy(zName, zValue);
    }else{
      if( *z ){ *z++ = 0; }
      cgi_set_parameter_nocopy(zName, "");
    }
  }
}

/*
** *pz is a string that consists of multiple lines of text.  This
** routine finds the end of the current line of text and converts
** the "\n" or "\r\n" that ends that line into a "\000".  It then
** advances *pz to the beginning of the next line and returns the
** previous value of *pz (which is the start of the current line.)
*/
static char *get_line_from_string(char **pz, int *pLen){
  char *z = *pz;
  int i;
  if( z[0]==0 ) return 0;
  for(i=0; z[i]; i++){
    if( z[i]=='\n' ){
      if( i>0 && z[i-1]=='\r' ){
        z[i-1] = 0;
      }else{
        z[i] = 0;
      }
      i++;
      break;
    }
  }
  *pz = &z[i];
  *pLen -= i;
  return z;
}

/*
** The input *pz points to content that is terminated by a "\r\n"
** followed by the boundry marker zBoundry.  An extra "--" may or
** may not be appended to the boundry marker.  There are *pLen characters
** in *pz.
**
** This routine adds a "\000" to the end of the content (overwriting
** the "\r\n" and returns a pointer to the content.  The *pz input
** is adjusted to point to the first line following the boundry.
** The length of the content is stored in *pnContent.
*/
static char *get_bounded_content(
  char **pz,         /* Content taken from here */
  int *pLen,         /* Number of bytes of data in (*pz)[] */
  char *zBoundry,    /* Boundry text marking the end of content */
  int *pnContent     /* Write the size of the content here */
){
  char *z = *pz;
  int len = *pLen;
  int i;
  int nBoundry = strlen(zBoundry);
  *pnContent = len;
  for(i=0; i<len; i++){
    if( z[i]=='\n' && strncmp(zBoundry, &z[i+1], nBoundry)==0 ){
      if( i>0 && z[i-1]=='\r' ) i--;
      z[i] = 0;
      *pnContent = i;
      i += nBoundry;
      break;
    }
  }
  *pz = &z[i];
  get_line_from_string(pz, pLen);
  return z;      
}

/*
** Tokenize a line of text into as many as nArg tokens.  Make
** azArg[] point to the start of each token.
**
** Tokens consist of space or semi-colon delimited words or
** strings inside double-quotes.  Example:
**
**    content-disposition: form-data; name="fn"; filename="index.html"
**
** The line above is tokenized as follows:
**
**    azArg[0] = "content-disposition:"
**    azArg[1] = "form-data"
**    azArg[2] = "name="
**    azArg[3] = "fn"
**    azArg[4] = "filename="
**    azArg[5] = "index.html"
**    azArg[6] = 0;
**
** '\000' characters are inserted in z[] at the end of each token.
** This routine returns the total number of tokens on the line, 6
** in the example above.
*/
static int tokenize_line(char *z, int mxArg, char **azArg){
  int i = 0;
  while( *z ){
    while( isspace(*z) || *z==';' ){ z++; }
    if( *z=='"' && z[1] ){
      *z = 0;
      z++;
      if( i<mxArg-1 ){ azArg[i++] = z; }
      while( *z && *z!='"' ){ z++; }
      if( *z==0 ) break;
      *z = 0;
      z++;
    }else{
      if( i<mxArg-1 ){ azArg[i++] = z; }
      while( *z && !isspace(*z) && *z!=';' && *z!='"' ){ z++; }
      if( *z && *z!='"' ){
        *z = 0;
        z++;
      }
    }
  }
  azArg[i] = 0;
  return i;
}

/*
** Scan the multipart-form content and make appropriate entries
** into the parameter table.
**
** The content string "z" is modified by this routine but it is
** not copied.  The calling function must not deallocate or modify
** "z" after this routine finishes or it could corrupt the parameter
** table.
*/
static void process_multipart_form_data(char *z, int len){
  char *zLine;
  int nArg, i;
  char *zBoundry;
  char *zValue;
  char *zName = 0;
  int showBytes = 0;
  char *azArg[50];

  zBoundry = get_line_from_string(&z, &len);
  if( zBoundry==0 ) return;
  while( (zLine = get_line_from_string(&z, &len))!=0 ){
    if( zLine[0]==0 ){
      int nContent = 0;
      zValue = get_bounded_content(&z, &len, zBoundry, &nContent);
      if( zName && zValue ){
        cgi_set_parameter_nocopy(zName, zValue);
        if( showBytes ){
          cgi_set_parameter_nocopy(mprintf("%s:bytes", zName),
               mprintf("%d",nContent));
        }
      }
      zName = 0;
      showBytes = 0;
    }else{
      nArg = tokenize_line(zLine, sizeof(azArg)/sizeof(azArg[0]), azArg);
      for(i=0; i<nArg; i++){
        int c = tolower(azArg[i][0]);
        if( c=='c' && stricmp(azArg[i],"content-disposition:")==0 ){
          i++;
        }else if( c=='n' && stricmp(azArg[i],"name=")==0 ){
          zName = azArg[++i];
        }else if( c=='f' && stricmp(azArg[i],"filename=")==0 ){
          char *z = azArg[++i];
          if( zName && z ){
            cgi_set_parameter_nocopy(mprintf("%s:filename",zName), z);
          }
          showBytes = 1;
        }else if( c=='c' && stricmp(azArg[i],"content-type:")==0 ){
          char *z = azArg[++i];
          if( zName && z ){
            cgi_set_parameter_nocopy(mprintf("%s:mimetype",zName), z);
          }
        }
      }
    }
  }        
}

/*
** Initialize the query parameter database.  Information is pulled from
** the QUERY_STRING environment variable (if it exists), from standard
** input if there is POST data, and from HTTP_COOKIE.
*/
void cgi_init(void){
  char *z;
  char *zType;
  int len;
  z = getenv("QUERY_STRING");
  if( z ){
    z = mprintf("%s",z);
    add_param_list(z, '&');
  }

  z = getenv("CONTENT_LENGTH");
  len = z ? atoi(z) : 0;
  zType = getenv("CONTENT_TYPE");
  if( len>0 && zType && 
    (strcmp(zType,"application/x-www-form-urlencoded")==0 
      || strncmp(zType,"multipart/form-data",19)==0) ){
    z = malloc( len+1 );
    if( z==0 ) exit(1);
    len = fread(z, 1, len, stdin);
    z[len] = 0;
    if( zType[0]=='a' ){
      add_param_list(z, '&');
    }else{
      process_multipart_form_data(z, len);
    }
  }

  z = getenv("HTTP_COOKIE");
  if( z ){
    z = mprintf("%s",z);
    add_param_list(z, ';');
  }
}

/*
** This is the comparison function used to sort the aParamQP[] array of
** query parameters and cookies.
*/
static int qparam_compare(const void *a, const void *b){
  struct QParam *pA = (struct QParam*)a;
  struct QParam *pB = (struct QParam*)b;
  return strcmp(pA->zName, pB->zName);
}

/*
** Return the value of a query parameter or cookie whose name is zName.
** If there is no query parameter or cookie named zName, then return
** zDefault instead.
*/
const char *cgi_parameter(const char *zName, const char *zDefault){
  int lo, hi, mid, c;
  if( nUsedQP<=0 ) return zDefault;
  if( sortQP ){
    qsort(aParamQP, nUsedQP, sizeof(aParamQP[0]), qparam_compare);
    sortQP = 0;
  }
  lo = 0;
  hi = nUsedQP-1;
  while( lo<=hi ){
    mid = (lo+hi)/2;
    c = strcmp(aParamQP[mid].zName, zName);
    if( c==0 ){
      return aParamQP[mid].zValue;
    }else if( c>0 ){
      hi = mid-1;
    }else{
      lo = mid+1;
    }
  }
  return zDefault;
}

/*
** Return true if any of the query parameters in the argument
** list are defined.
*/
int cgi_any(const char *z, ...){
  va_list ap;
  char *z2;
  if( cgi_parameter(z,0)!=0 ) return 1;
  va_start(ap, z);
  while( (z2 = va_arg(ap, char*))!=0 ){
    if( cgi_parameter(z2,0)!=0 ) return 1;
  }
  va_end(ap);
  return 0;
}

/*
** Return true if all of the query parameters in the argument list
** are defined.
*/
int cgi_all(const char *z, ...){
  va_list ap;
  char *z2;
  if( cgi_parameter(z,0)==0 ) return 0;
  va_start(ap, z);
  while( (z2 = va_arg(ap, char*))==0 ){
    if( cgi_parameter(z2,0)==0 ) return 0;
  }
  va_end(ap);
  return 1;
}

/*
** Print all query parameters on standard output.  Format the
** parameters as HTML.  This is used for testing and debugging.
*/
void cgi_print_all(void){
  int i;
  cgi_parameter("","");  /* For the parameters into sorted order */
  for(i=0; i<nUsedQP; i++){
    cgi_printf("%s = %s  <br />\n",
       htmlize(aParamQP[i].zName, -1), htmlize(aParamQP[i].zValue, -1));
  }
}

/*
** Write HTML text for an option menu to standard output.  zParam
** is the query parameter that the option menu sets.  zDflt is the
** initial value of the option menu.  Additional arguments are name/value
** pairs that define values on the menu.  The list is terminated with
** a single NULL argument.
*/
void cgi_optionmenu(int in, const char *zP, const char *zD, ...){
  va_list ap;
  char *zName, *zVal;
  int dfltSeen = 0;
  cgi_printf("%*s<select size=1 name=\"%s\">\n", in, "", zP);
  va_start(ap, zD);
  while( (zName = va_arg(ap, char*))!=0 && (zVal = va_arg(ap, char*))!=0 ){
    if( strcmp(zVal,zD)==0 ){ dfltSeen = 1; break; }
  }
  va_end(ap);
  if( !dfltSeen ){
    if( zD[0] ){
      cgi_printf("%*s<option value=\"%h\" selected>%h</option>\n",
        in+2, "", zD, zD);
    }else{
      cgi_printf("%*s<option value=\"\" selected>&nbsp;</option>\n", in+2, "");
    }
  }
  va_start(ap, zD);
  while( (zName = va_arg(ap, char*))!=0 && (zVal = va_arg(ap, char*))!=0 ){
    if( zName[0] ){
      cgi_printf("%*s<option value=\"%h\"%s>%h</option>\n",
        in+2, "",
        zVal,
        strcmp(zVal, zD) ? "" : " selected",
        zName
      );
    }else{
      cgi_printf("%*s<option value=\"\"%s>&nbsp;</option>\n",
        in+2, "",
        strcmp(zVal, zD) ? "" : " selected"
      );
    }
  }
  va_end(ap);
  cgi_printf("%*s</select>\n", in, "");
}

/*
** This routine works a lot like cgi_optionmenu() except that the list of
** values is contained in an array.  Also, the values are just values, not
** name/value pairs as in cgi_optionmenu.
*/
void cgi_v_optionmenu(
  int in,              /* Indent by this amount */
  const char *zP,      /* The query parameter name */
  const char *zD,      /* Default value */
  const char **az      /* NULL-terminated list of allowed values */
){
  const char *zVal;
  int i;
  cgi_printf("%*s<select size=1 name=\"%s\">\n", in, "", zP);
  for(i=0; az[i]; i++){
    if( strcmp(az[i],zD)==0 ) break;
  }
  if( az[i]==0 ){
    if( zD[0]==0 ){
      cgi_printf("%*s<option value=\"\" selected>&nbsp;</option>\n",
       in+2, "");
    }else{
      cgi_printf("%*s<option value=\"%h\" selected>%h</option>\n",
       in+2, "", zD, zD);
    }
  }
  while( (zVal = *(az++))!=0  ){
    if( zVal[0] ){
      cgi_printf("%*s<option value=\"%h\"%s>%h</option>\n",
        in+2, "",
        zVal,
        strcmp(zVal, zD) ? "" : " selected",
        zVal
      );
    }else{
      cgi_printf("%*s<option value=\"\"%s>&nbsp;</option>\n",
        in+2, "",
        strcmp(zVal, zD) ? "" : " selected"
      );
    }
  }
  cgi_printf("%*s</select>\n", in, "");
}

/*
** This routine works a lot like cgi_v_optionmenu() except that the list
** is a list of pairs.  The first element of each pair is the value used
** internally and the second element is the value displayed to the user.
*/
void cgi_v_optionmenu2(
  int in,              /* Indent by this amount */
  const char *zP,      /* The query parameter name */
  const char *zD,      /* Default value */
  const char **az      /* NULL-terminated list of allowed values */
){
  const char *zVal;
  int i;
  cgi_printf("%*s<select size=1 name=\"%s\">\n", in, "", zP);
  for(i=0; az[i]; i+=2){
    if( strcmp(az[i],zD)==0 ) break;
  }
  if( az[i]==0 ){
    if( zD[0]==0 ){
      cgi_printf("%*s<option value=\"\" selected>&nbsp;</option>\n",
       in+2, "");
    }else{
      cgi_printf("%*s<option value=\"%h\" selected>%h</option>\n",
       in+2, "", zD, zD);
    }
  }
  while( (zVal = *(az++))!=0  ){
    const char *zName = *(az++);
    if( zName[0] ){
      cgi_printf("%*s<option value=\"%h\"%s>%h</option>\n",
        in+2, "",
        zVal,
        strcmp(zVal, zD) ? "" : " selected",
        zName
      );
    }else{
      cgi_printf("%*s<option value=\"%h\"%s>&nbsp;</option>\n",
        in+2, "",
        zVal,
        strcmp(zVal, zD) ? "" : " selected"
      );
    }
  }
  cgi_printf("%*s</select>\n", in, "");
}

/*
** This routine should never be called directly. Use wrapper functions below.
** Generates HTML input element to be used in forms.
** Parameters are explained below inline. If any param is 0 that 
** attribute/feature will not be used. zValue is required, except for text
** fields. zName is also required except for submit, reset and button.
*/
void cgi_input_elem(
  int nType,              /* 1:submit, 2:reset, 3:button, 4:file, 5:hidden,
                          ** 6:checkbox, 7:radio, 8:password, 9:text */
  const char *zName,      /* CGI param name */
  const char *zId,        /* HTML element id */
  const char *zClass,     /* CSS class to apply */
  char nAccessKey,        /* Access key to assign */
  int nTabIndex,          /* Element's tab index */
  int nSize,              /* Used only for text fields */
  int nMaxLen,            /* Used only for text fields */
  int nLabelOnLeft,       /* If set, put label text left of element */
  const char *zValue,     /* Element's value */
  const char *zDflt,      /* If same as zValue, "select" this element */
  const char *zLabel      /* Label text. No HTML escaping is done on it */
){
  /* Buttons and hidden fields can't have label
  */
  int bHasLabel = ( nType>4 && zLabel && zLabel[0] );

  assert( nType > 0 );
  assert( nType <= 9 );
  if( zValue==0 ) return;
  if( nType<1 && nType>3 && (!zName || !zName[0]) ) return;
  if( bHasLabel ){
    /* Make sure we have some valid id because <label> won't work in IE w/o it
    */
    if( !zId || !zId[0] ) zId = mprintf("%s%h", zName, zValue);
    @ <label for="%h(zId)">\
    if( nLabelOnLeft>0 ){
      @ %s(zLabel) \
    }
  }
  
  @ <input\
  switch( nType ){
    case 1: cgi_printf(" type=\"submit\""); break;
    case 2: cgi_printf(" type=\"reset\""); break;
    case 3: cgi_printf(" type=\"button\""); break;
    case 4: cgi_printf(" type=\"file\""); break;
    case 5: cgi_printf(" type=\"hidden\""); break;
    case 6: cgi_printf(" type=\"checkbox\""); break;
    case 7: cgi_printf(" type=\"radio\""); break;
    case 8: cgi_printf(" type=\"password\""); break;
    case 9:
      @  type="text"\
      if( nSize>0 ){
        @  size="%d(nSize)"\
      }
      if( nMaxLen>0 ){
        @  maxlength="%d(nMaxLen)"\
      }
      break;
    default: return;
  }
  
  if( zName && zName[0] ){
    @  name="%h(zName)"\
  }
  if( zId && zId[0] ){
    @  id="%h(zId)"\
  }
  if( zClass && zClass[0] ){
    @  class="%h(zClass)"\
  }
  if( isalnum(nAccessKey) ){
    @  accesskey="%c(nAccessKey)"\
  }
  if( nTabIndex>0 ){
    @  tabindex="%d(nTabIndex)"\
  }
  if( zValue && zValue[0] ){
    @  value="%h(zValue)"\
  }
  if( zDflt && zDflt[0] && strcmp(zDflt, zValue)==0 ){
    @  checked\
  }  
  @ >\
  
  if( bHasLabel ){
    if( nLabelOnLeft<=0 ){
      @  %s(zLabel)\
    }
    @ </label>
  }else{
    @ 
  }
}

void cgi_submit(
  const char *zName,      /* CGI param name */
  const char *zId,        /* HTML element id */
  const char *zClass,     /* CSS class to apply */
  char nAccessKey,        /* Access key to assign */
  int nTabIndex,          /* Element's tab index */
  const char *zValue      /* Element's value */
){
  cgi_input_elem(
    1, zName, zId, zClass, nAccessKey, nTabIndex, 0, 0, 0, zValue, 0, 0
  );
}

void cgi_reset(
  const char *zName,      /* CGI param name */
  const char *zId,        /* HTML element id */
  const char *zClass,     /* CSS class to apply */
  char nAccessKey,        /* Access key to assign */
  int nTabIndex,          /* Element's tab index */
  const char *zValue      /* Element's value */
){
  cgi_input_elem(
    2, zName, zId, zClass, nAccessKey, nTabIndex, 0, 0, 0, zValue, 0, 0
  );
}

void cgi_button(
  const char *zName,      /* CGI param name */
  const char *zId,        /* HTML element id */
  const char *zClass,     /* CSS class to apply */
  char nAccessKey,        /* Access key to assign */
  int nTabIndex,          /* Element's tab index */
  const char *zValue      /* Element's value */
){
  cgi_input_elem(
    3, zName, zId, zClass, nAccessKey, nTabIndex, 0, 0, 0, zValue, 0, 0
  );
}

void cgi_file(
  const char *zName,      /* CGI param name */
  const char *zId,        /* HTML element id */
  const char *zClass,     /* CSS class to apply */
  char nAccessKey,        /* Access key to assign */
  int nTabIndex,          /* Element's tab index */
  const char *zValue      /* Element's value */
){
  cgi_input_elem(
    4, zName, zId, zClass, nAccessKey, nTabIndex, 0, 0, 0, zValue, 0, 0
  );
}


void cgi_hidden(
  const char *zName,      /* CGI param name */
  const char *zId,        /* HTML element id */
  const char *zValue      /* Element's value */
){
  cgi_input_elem(5, zName, zId, 0, 0, 0, 0, 0, 0, zValue, 0, 0);
}

void cgi_checkbox(
  const char *zName,      /* CGI param name */
  const char *zId,        /* HTML element id */
  const char *zClass,     /* CSS class to apply */
  char nAccessKey,        /* Access key to assign */
  int nTabIndex,          /* Element's tab index */
  int nLabelOnLeft,       /* If set, put label text left of element */
  const char *zValue,     /* Element's value */
  const char *zDflt,      /* If same as zValue, "select" this element */
  const char *zLabel      /* Label text. No HTML escaping is done on it */
){
  cgi_input_elem(
    6, zName, zId, zClass, nAccessKey, nTabIndex, 0, 0, nLabelOnLeft,
    zValue, zDflt, zLabel
  );
}

void cgi_radio(
  const char *zName,      /* CGI param name */
  const char *zId,        /* HTML element id */
  const char *zClass,     /* CSS class to apply */
  char nAccessKey,        /* Access key to assign */
  int nTabIndex,          /* Element's tab index */
  int nLabelOnLeft,       /* If set, put label text left of element */
  const char *zValue,     /* Element's value */
  const char *zDflt,      /* If same as zValue, "select" this element */
  const char *zLabel      /* Label text. No HTML escaping is done on it */
){
  cgi_input_elem(
    7, zName, zId, zClass, nAccessKey, nTabIndex, 0, 0, nLabelOnLeft,
    zValue, zDflt, zLabel
  );
}

void cgi_password(
  const char *zName,      /* CGI param name */
  const char *zId,        /* HTML element id */
  const char *zClass,     /* CSS class to apply */
  char nAccessKey,        /* Access key to assign */
  int nTabIndex,          /* Element's tab index */
  int nSize,              /* Field size */
  int nMaxLen,            /* Maximum number of chars field will accept */
  int nLabelOnLeft,       /* If set, put label text left of element */
  const char *zValue,     /* Element's value */
  const char *zLabel      /* Label text. No HTML escaping is done on it */
){
  cgi_input_elem(
    8, zName, zId, zClass, nAccessKey, nTabIndex, nSize, nMaxLen,
    nLabelOnLeft, zValue, 0, zLabel
  );
}

void cgi_text(
  const char *zName,      /* CGI param name */
  const char *zId,        /* HTML element id */
  const char *zClass,     /* CSS class to apply */
  char nAccessKey,        /* Access key to assign */
  int nTabIndex,          /* Element's tab index */
  int nSize,              /* Field size */
  int nMaxLen,            /* Maximum number of chars field will accept */
  int nLabelOnLeft,       /* If set, put label text left of element */
  const char *zValue,     /* Element's value */
  const char *zLabel      /* Label text. No HTML escaping is done on it */
){
  cgi_input_elem(
    9, zName, zId, zClass, nAccessKey, nTabIndex, nSize, nMaxLen,
    nLabelOnLeft, zValue, 0, zLabel
  );
}

/*
** Generates radio button elements grouped in <fieldset>.
** Parameters are explained below inline. If any param is 0 that 
** attribute/feature will not be used. zName and at least one
** accesskey/value/label trio are required.
*/
void cgi_radio_fieldset(
  const char *zTitle,     /* Title of <fieldset>. If 0 <fieldset> is not used */
  const char *zName,      /* CGI param name */
  const char *zClass,     /* CSS class to apply to fieldset */
  int *nTabIndex,         /* This will be incremented for each element. 
                          ** Calling func should be able to continue with it */
  const char *zDflt,      /* Element with this value will be checked */
  ...                     /* NULL-terminated list of value/accesskey/label trios */
){
  char *zVal, *zLabel;
  char nAccessKey;
  va_list ap;
  if( !zName || !zName[0] ) return;
  
  if( zTitle && zTitle[0] ){
    @ <fieldset\
    if( zClass && zClass[0] ){
      @  class="%h(zClass)"\
    }
    @ ><legend>%s(zTitle)</legend>
  }

  /*
  ** args are: accesskey, value, label
  */
  va_start(ap, zDflt);
  while( (zVal = va_arg(ap, char*))!=0 ){
    nAccessKey = va_arg(ap, int);
    zLabel = va_arg(ap, char*);
    if( zLabel==0 ) break;

    cgi_radio(zName, 0, 0, isprint(nAccessKey)? nAccessKey : 0,
              nTabIndex==0 ? 0 : ++(*nTabIndex), 0,
              zVal, zDflt, zLabel);
  }
  va_end(ap);
  if( nTabIndex ) (*nTabIndex)++;
  if( zTitle && zTitle[0] ){
    @ </fieldset>
  }
}

/*
** Generates checkbox elements grouped in <fieldset>.
** Parameters are explained below inline. If any param is 0 that 
** attribute/feature will not be used. At least one quintet of
** name/accesskey/value/default value/label is required.
** If value and default value are equal, element will be "checked".
*/
void cgi_checkbox_fieldset(
  const char *zTitle,     /* Title of <fieldset> */
  const char *zClass,     /* CSS class to apply to fieldset */
  int *nTabIndex,         /* This will be incremented for each element. 
                          ** Calling func should be able to continue with it */
  ...                     /* NULL-terminated list of
                          ** name/value/accesskey/label trios */
){
  va_list ap;
  int nAccessKey;
  char *zName, *zVal, *zDflt, *zLabel;
  
  if( zTitle && zTitle[0] ){
    @ <fieldset\
    if( zClass && zClass[0] ){
      @  class="%h(zClass)"\
    }
    @ ><legend>%s(zTitle)</legend>
  }
  va_start(ap, nTabIndex);
  while( (zName = va_arg(ap, char*))!=0 ){
    zVal = va_arg(ap, char*);
    nAccessKey = va_arg(ap, int);
    zDflt = va_arg(ap, char*);
    zLabel = va_arg(ap, char*);
    if( !zName || !zName[0] || !zVal || !zVal[0] || !zLabel || !zLabel[0] ){
      break;
    }
    /* Label won't work without id in IE so we dummy up id here.
    ** Hopefully this won't clash with anyones's CSS.
    */
    cgi_checkbox(zName, 0, 0, nAccessKey, nTabIndex==0 ? 0 : ++(*nTabIndex), 0,
              zVal, zDflt, zLabel);
  }
  va_end(ap);
  if( nTabIndex ) (*nTabIndex)++;
  if( zTitle && zTitle[0] ){
    @ </fieldset>
  }
}

/*
** Generates HTML links (<a> element).
** Parameters are explained below inline. If any param is 0 that 
** attribute/feature will not be used. zText and zHref are required.
*/
void cgi_href(
  const char *zText,      /* Link text */
  const char *zId,        /* HTML element id */
  const char *zClass,     /* CSS class to apply */
  char nAccessKey,        /* Access key to assign */
  int nTabIndex,          /* Element's tab index */
  const char *zTitle,     /* Title to apply to <a> */
  const char *zHref,      /* Link address (vxprintf() format) */
  ...
){
  va_list ap;
  
  if( !zText || !zText[0] || !zHref || !zHref[0] ) return;
  
  @ <a href="\
  va_start(ap, zHref);
  cgi_vprintf(zHref, ap);
  va_end(ap);
  @ "\
  
  if( zId && zId[0] ){
    @  id="%h(zId)"\
  }
  if( zClass && zClass[0] ){
    @  class="%h(zClass)"\
  }
  if( isalnum(nAccessKey) ){
    @  accesskey="%c(nAccessKey)"\
  }
  if( nTabIndex>0 ){
    @  tabindex="%d(nTabIndex)"\
  }
  if( zTitle && zTitle[0] ){
    @  title="%h(zTitle)"\
  }
  
  @ >%h(zText)</a>
}

/*
** The "printf" code that follows dates from the 1980's.  It is in
** the public domain.  The original comments are included here for
** completeness.  They are slightly out-of-date.
**
** The following modules is an enhanced replacement for the "printf" programs
** found in the standard library.  The following enhancements are
** supported:
**
**      +  Additional functions.  The standard set of "printf" functions
**         includes printf, fprintf, sprintf, vprintf, vfprintf, and
**         vsprintf.  This module adds the following:
**
**           *  snprintf -- Works like sprintf, but has an extra argument
**                          which is the size of the buffer written to.
**
**           *  mprintf --  Similar to sprintf.  Writes output to memory
**                          obtained from malloc.
**
**           *  xprintf --  Calls a function to dispose of output.
**
**           *  nprintf --  No output, but returns the number of characters
**                          that would have been output by printf.
**
**           *  A v- version (ex: vsnprintf) of every function is also
**              supplied.
**
**      +  A few extensions to the formatting notation are supported:
**
**           *  The "=" flag (similar to "-") causes the output to be
**              be centered in the appropriately sized field.
**
**           *  The %b field outputs an integer in binary notation.
**
**           *  The %c field now accepts a precision.  The character output
**              is repeated by the number of times the precision specifies.
**
**           *  The %' field works like %c, but takes as its character the
**              next character of the format string, instead of the next
**              argument.  For example,  printf("%.78'-")  prints 78 minus
**              signs, the same as  printf("%.78c",'-').
**
**      +  When compiled using GCC on a SPARC, this version of printf is
**         faster than the library printf for SUN OS 4.1.
**
**      +  All functions are fully reentrant.
**
*/

/*
** Undefine COMPATIBILITY to make some slight changes in the way things
** work.  I think the changes are an improvement, but they are not
** backwards compatible.
*/
/* #define COMPATIBILITY       / * Compatible with SUN OS 4.1 */

/*
** Conversion types fall into various categories as defined by the
** following enumeration.
*/
enum et_type {    /* The type of the format field */
   etRADIX,            /* Integer types.  %d, %x, %o, and so forth */
   etFLOAT,            /* Floating point.  %f */
   etEXP,              /* Exponentional notation. %e and %E */
   etGENERIC,          /* Floating or exponential, depending on exponent. %g */
   etSIZE,             /* Return number of characters processed so far. %n */
   etSTRING,           /* Strings. %s */
   etPERCENT,          /* Percent symbol. %% */
   etCHARX,            /* Characters. %c */
   etERROR,            /* Used to indicate no such conversion type */
/* The rest are extensions, not normally found in printf() */
   etCHARLIT,          /* Literal characters.  %' */
   etDYNAMIC,          /* Like %s but free() called on input */
   etORDINAL,          /* 1st, 2nd, 3rd and so forth */
   etHTMLIZE,          /* Make text safe for HTML */
   etHTTPIZE,          /* Make text safe for HTTP.  "/" encoded as %2f */
   etURLIZE            /* Make text safe for HTTP.  "/" not encoded */
};

/*
** Each builtin conversion character (ex: the 'd' in "%d") is described
** by an instance of the following structure
*/
typedef struct et_info {   /* Information about each format field */
  int  fmttype;              /* The format field code letter */
  int  base;                 /* The base for radix conversion */
  char *charset;             /* The character set for conversion */
  int  flag_signed;          /* Is the quantity signed? */
  char *prefix;              /* Prefix on non-zero values in alt format */
  enum et_type type;          /* Conversion paradigm */
} et_info;

/*
** The following table is searched linearly, so it is good to put the
** most frequently used conversion types first.
*/
static et_info fmtinfo[] = {
  { 'd',  10,  "0123456789",       1,    0, etRADIX,      },
  { 's',   0,  0,                  0,    0, etSTRING,     }, 
  { 'z',   0,  0,                  0,    0, etDYNAMIC,    }, 
  { 'h',   0,  0,                  0,    0, etHTMLIZE,    },
  { 't',   0,  0,                  0,    0, etHTTPIZE,    }, /* / -> %2F */
  { 'T',   0,  0,                  0,    0, etURLIZE,     }, /* / -> / */
  { 'c',   0,  0,                  0,    0, etCHARX,      },
  { 'o',   8,  "01234567",         0,  "0", etRADIX,      },
  { 'u',  10,  "0123456789",       0,    0, etRADIX,      },
  { 'x',  16,  "0123456789abcdef", 0, "x0", etRADIX,      },
  { 'X',  16,  "0123456789ABCDEF", 0, "X0", etRADIX,      },
  { 'r',  10,  "0123456789",       0,    0, etORDINAL,    },
  { 'f',   0,  0,                  1,    0, etFLOAT,      },
  { 'e',   0,  "e",                1,    0, etEXP,        },
  { 'E',   0,  "E",                1,    0, etEXP,        },
  { 'g',   0,  "e",                1,    0, etGENERIC,    },
  { 'G',   0,  "E",                1,    0, etGENERIC,    },
  { 'i',  10,  "0123456789",       1,    0, etRADIX,      },
  { 'n',   0,  0,                  0,    0, etSIZE,       },
  { '%',   0,  0,                  0,    0, etPERCENT,    },
  { 'b',   2,  "01",               0, "b0", etRADIX,      }, /* Binary */
  { 'p',  10,  "0123456789",       0,    0, etRADIX,      }, /* Pointers */
  { '\'',  0,  0,                  0,    0, etCHARLIT,    }, /* Literal char */
};
#define etNINFO  (sizeof(fmtinfo)/sizeof(fmtinfo[0]))

/*
** If NOFLOATINGPOINT is defined, then none of the floating point
** conversions will work.
*/
#ifndef etNOFLOATINGPOINT
/*
** "*val" is a double such that 0.1 <= *val < 10.0
** Return the ascii code for the leading digit of *val, then
** multiply "*val" by 10.0 to renormalize.
**
** Example:
**     input:     *val = 3.14159
**     output:    *val = 1.4159    function return = '3'
**
** The counter *cnt is incremented each time.  After counter exceeds
** 16 (the number of significant digits in a 64-bit float) '0' is
** always returned.
*/
static int et_getdigit(double *val, int *cnt){
  int digit;
  double d;
  if( (*cnt)++ >= 16 ) return '0';
  digit = (int)*val;
  d = digit;
  digit += '0';
  *val = (*val - d)*10.0;
  return digit;
}
#endif

#define etBUFSIZE 1000  /* Size of the output buffer */

/*
** The root program.  All variations call this core.
**
** INPUTS:
**   func   This is a pointer to a function taking three arguments
**            1. A pointer to anything.  Same as the "arg" parameter.
**            2. A pointer to the list of characters to be output
**               (Note, this list is NOT null terminated.)
**            3. An integer number of characters to be output.
**               (Note: This number might be zero.)
**
**   arg    This is the pointer to anything which will be passed as the
**          first argument to "func".  Use it for whatever you like.
**
**   fmt    This is the format string, as in the usual print.
**
**   ap     This is a pointer to a list of arguments.  Same as in
**          vfprint.
**
** OUTPUTS:
**          The return value is the total number of characters sent to
**          the function "func".  Returns -1 on a error.
**
** Note that the order in which automatic variables are declared below
** seems to make a big difference in determining how fast this beast
** will run.
*/
static int vxprintf(
  void (*func)(void*,char*,int),
  void *arg,
  const char *format,
  va_list ap
){
  register const char *fmt; /* The format string. */
  register int c;           /* Next character in the format string */
  register char *bufpt;     /* Pointer to the conversion buffer */
  register int  precision;  /* Precision of the current field */
  register int  length;     /* Length of the field */
  register int  idx;        /* A general purpose loop counter */
  int count;                /* Total number of characters output */
  int width;                /* Width of the current field */
  int flag_leftjustify;     /* True if "-" flag is present */
  int flag_plussign;        /* True if "+" flag is present */
  int flag_blanksign;       /* True if " " flag is present */
  int flag_alternateform;   /* True if "#" flag is present */
  int flag_zeropad;         /* True if field width constant starts with zero */
  int flag_long;            /* True if "l" flag is present */
  int flag_center;          /* True if "=" flag is present */
  unsigned long longvalue;  /* Value for integer types */
  double realvalue;         /* Value for real types */
  et_info *infop;           /* Pointer to the appropriate info structure */
  char buf[etBUFSIZE];      /* Conversion buffer */
  char prefix;              /* Prefix character.  "+" or "-" or " " or '\0'. */
  int  errorflag = 0;       /* True if an error is encountered */
  enum et_type xtype;       /* Conversion paradigm */
  char *zMem;               /* String to be freed */
  char *zExtra;             /* Extra memory to be freed after use */
  static char spaces[] = "                                                  "
     "                                                                      ";
#define etSPACESIZE (sizeof(spaces)-1)
#ifndef etNOFLOATINGPOINT
  int  exp;                 /* exponent of real numbers */
  double rounder;           /* Used for rounding floating point values */
  int flag_dp;              /* True if decimal point should be shown */
  int flag_rtz;             /* True if trailing zeros should be removed */
  int flag_exp;             /* True to force display of the exponent */
  int nsd;                  /* Number of significant digits returned */
#endif

  fmt = format;                     /* Put in a register for speed */
  count = length = 0;
  bufpt = 0;
  for(; (c=(*fmt))!=0; ++fmt){
    if( c!='%' ){
      register int amt;
      bufpt = (char *)fmt;
      amt = 1;
      while( (c=(*++fmt))!='%' && c!=0 ) amt++;
      (*func)(arg,bufpt,amt);
      count += amt;
      if( c==0 ) break;
    }
    if( (c=(*++fmt))==0 ){
      errorflag = 1;
      (*func)(arg,"%",1);
      count++;
      break;
    }
    /* Find out what flags are present */
    flag_leftjustify = flag_plussign = flag_blanksign = 
     flag_alternateform = flag_zeropad = flag_center = 0;
    do{
      switch( c ){
        case '-':   flag_leftjustify = 1;     c = 0;   break;
        case '+':   flag_plussign = 1;        c = 0;   break;
        case ' ':   flag_blanksign = 1;       c = 0;   break;
        case '#':   flag_alternateform = 1;   c = 0;   break;
        case '0':   flag_zeropad = 1;         c = 0;   break;
        case '=':   flag_center = 1;          c = 0;   break;
        default:                                       break;
      }
    }while( c==0 && (c=(*++fmt))!=0 );
    if( flag_center ) flag_leftjustify = 0;
    /* Get the field width */
    width = 0;
    if( c=='*' ){
      width = va_arg(ap,int);
      if( width<0 ){
        flag_leftjustify = 1;
        width = -width;
      }
      c = *++fmt;
    }else{
      while( c>='0' && c<='9' ){
        width = width*10 + c - '0';
        c = *++fmt;
      }
    }
    if( width > etBUFSIZE-10 ){
      width = etBUFSIZE-10;
    }
    /* Get the precision */
    if( c=='.' ){
      precision = 0;
      c = *++fmt;
      if( c=='*' ){
        precision = va_arg(ap,int);
#ifndef etCOMPATIBILITY
        /* This is sensible, but SUN OS 4.1 doesn't do it. */
        if( precision<0 ) precision = 0x7fffffff & -precision;
#endif
        c = *++fmt;
      }else{
        while( c>='0' && c<='9' ){
          precision = precision*10 + c - '0';
          c = *++fmt;
        }
      }
      /* Limit the precision to prevent overflowing buf[] during conversion */
      /* if( precision>etBUFSIZE-40 ) precision = etBUFSIZE-40; */
    }else{
      precision = -1;
    }
    /* Get the conversion type modifier */
    if( c=='l' ){
      flag_long = 1;
      c = *++fmt;
    }else{
      flag_long = 0;
    }
    /* Fetch the info entry for the field */
    infop = 0;
    for(idx=0; idx<etNINFO; idx++){
      if( c==fmtinfo[idx].fmttype ){
        infop = &fmtinfo[idx];
        break;
      }
    }
    /* No info entry found.  It must be an error. */
    if( infop==0 ){
      xtype = etERROR;
    }else{
      xtype = infop->type;
    }
    zExtra = 0;

    /*
    ** At this point, variables are initialized as follows:
    **
    **   flag_alternateform          TRUE if a '#' is present.
    **   flag_plussign               TRUE if a '+' is present.
    **   flag_leftjustify            TRUE if a '-' is present or if the
    **                               field width was negative.
    **   flag_zeropad                TRUE if the width began with 0.
    **   flag_long                   TRUE if the letter 'l' (ell) prefixed
    **                               the conversion character.
    **   flag_blanksign              TRUE if a ' ' is present.
    **   width                       The specified field width.  This is
    **                               always non-negative.  Zero is the default.
    **   precision                   The specified precision.  The default
    **                               is -1.
    **   xtype                       The class of the conversion.
    **   infop                       Pointer to the appropriate info struct.
    */
    switch( xtype ){
      case etORDINAL:
      case etRADIX:
        if( flag_long )  longvalue = va_arg(ap,long);
	else             longvalue = va_arg(ap,int);
#ifdef etCOMPATIBILITY
        /* For the format %#x, the value zero is printed "0" not "0x0".
        ** I think this is stupid. */
        if( longvalue==0 ) flag_alternateform = 0;
#else
        /* More sensible: turn off the prefix for octal (to prevent "00"),
        ** but leave the prefix for hex. */
        if( longvalue==0 && infop->base==8 ) flag_alternateform = 0;
#endif
        if( infop->flag_signed ){
          if( *(long*)&longvalue<0 ){
            longvalue = -*(long*)&longvalue;
            prefix = '-';
          }else if( flag_plussign )  prefix = '+';
          else if( flag_blanksign )  prefix = ' ';
          else                       prefix = 0;
        }else                        prefix = 0;
        if( flag_zeropad && precision<width-(prefix!=0) ){
          precision = width-(prefix!=0);
	}
        bufpt = &buf[etBUFSIZE];
        if( xtype==etORDINAL ){
          long a,b;
          a = longvalue%10;
          b = longvalue%100;
          bufpt -= 2;
          if( a==0 || a>3 || (b>10 && b<14) ){
            bufpt[0] = 't';
            bufpt[1] = 'h';
          }else if( a==1 ){
            bufpt[0] = 's';
            bufpt[1] = 't';
          }else if( a==2 ){
            bufpt[0] = 'n';
            bufpt[1] = 'd';
          }else if( a==3 ){
            bufpt[0] = 'r';
            bufpt[1] = 'd';
          }
        }
        {
          register char *cset;      /* Use registers for speed */
          register int base;
          cset = infop->charset;
          base = infop->base;
          do{                                           /* Convert to ascii */
            *(--bufpt) = cset[longvalue%base];
            longvalue = longvalue/base;
          }while( longvalue>0 );
	}
        length = (long)&buf[etBUFSIZE]-(long)bufpt;
        if( precision>etBUFSIZE-40 ) precision = etBUFSIZE - 40;
        for(idx=precision-length; idx>0; idx--){
          *(--bufpt) = '0';                             /* Zero pad */
	}
        if( prefix ) *(--bufpt) = prefix;               /* Add sign */
        if( flag_alternateform && infop->prefix ){      /* Add "0" or "0x" */
          char *pre, x;
          pre = infop->prefix;
          if( *bufpt!=pre[0] ){
            for(pre=infop->prefix; (x=(*pre))!=0; pre++) *(--bufpt) = x;
	  }
        }
        length = (long)&buf[etBUFSIZE]-(long)bufpt;
        break;
      case etFLOAT:
      case etEXP:
      case etGENERIC:
        realvalue = va_arg(ap,double);
#ifndef etNOFLOATINGPOINT
        if( precision<0 ) precision = 6;         /* Set default precision */
        if( precision>etBUFSIZE-10 ) precision = etBUFSIZE-10;
        if( realvalue<0.0 ){
          realvalue = -realvalue;
          prefix = '-';
	}else{
          if( flag_plussign )          prefix = '+';
          else if( flag_blanksign )    prefix = ' ';
          else                         prefix = 0;
	}
        if( infop->type==etGENERIC && precision>0 ) precision--;
        rounder = 0.0;
#ifdef COMPATIBILITY
        /* Rounding works like BSD when the constant 0.4999 is used.  Wierd! */
        for(idx=precision, rounder=0.4999; idx>0; idx--, rounder*=0.1);
#else
        /* It makes more sense to use 0.5 */
        for(idx=precision, rounder=0.5; idx>0; idx--, rounder*=0.1);
#endif
        if( infop->type==etFLOAT ) realvalue += rounder;
        /* Normalize realvalue to within 10.0 > realvalue >= 1.0 */
        exp = 0;
        if( realvalue>0.0 ){
          int k = 0;
          while( realvalue>=1e8 && k++<100 ){ realvalue *= 1e-8; exp+=8; }
          while( realvalue>=10.0 && k++<100 ){ realvalue *= 0.1; exp++; }
          while( realvalue<1e-8 && k++<100 ){ realvalue *= 1e8; exp-=8; }
          while( realvalue<1.0 && k++<100 ){ realvalue *= 10.0; exp--; }
          if( k>=100 ){
            bufpt = "NaN";
            length = 3;
            break;
          }
	}
        bufpt = buf;
        /*
        ** If the field type is etGENERIC, then convert to either etEXP
        ** or etFLOAT, as appropriate.
        */
        flag_exp = xtype==etEXP;
        if( xtype!=etFLOAT ){
          realvalue += rounder;
          if( realvalue>=10.0 ){ realvalue *= 0.1; exp++; }
        }
        if( xtype==etGENERIC ){
          flag_rtz = !flag_alternateform;
          if( exp<-4 || exp>precision ){
            xtype = etEXP;
          }else{
            precision = precision - exp;
            xtype = etFLOAT;
          }
	}else{
          flag_rtz = 0;
	}
        /*
        ** The "exp+precision" test causes output to be of type etEXP if
        ** the precision is too large to fit in buf[].
        */
        nsd = 0;
        if( xtype==etFLOAT && exp+precision<etBUFSIZE-30 ){
          flag_dp = (precision>0 || flag_alternateform);
          if( prefix ) *(bufpt++) = prefix;         /* Sign */
          if( exp<0 )  *(bufpt++) = '0';            /* Digits before "." */
          else for(; exp>=0; exp--) *(bufpt++) = et_getdigit(&realvalue,&nsd);
          if( flag_dp ) *(bufpt++) = '.';           /* The decimal point */
          for(exp++; exp<0 && precision>0; precision--, exp++){
            *(bufpt++) = '0';
          }
          while( (precision--)>0 ) *(bufpt++) = et_getdigit(&realvalue,&nsd);
          *(bufpt--) = 0;                           /* Null terminate */
          if( flag_rtz && flag_dp ){     /* Remove trailing zeros and "." */
            while( bufpt>=buf && *bufpt=='0' ) *(bufpt--) = 0;
            if( bufpt>=buf && *bufpt=='.' ) *(bufpt--) = 0;
          }
          bufpt++;                            /* point to next free slot */
	}else{    /* etEXP or etGENERIC */
          flag_dp = (precision>0 || flag_alternateform);
          if( prefix ) *(bufpt++) = prefix;   /* Sign */
          *(bufpt++) = et_getdigit(&realvalue,&nsd);  /* First digit */
          if( flag_dp ) *(bufpt++) = '.';     /* Decimal point */
          while( (precision--)>0 ) *(bufpt++) = et_getdigit(&realvalue,&nsd);
          bufpt--;                            /* point to last digit */
          if( flag_rtz && flag_dp ){          /* Remove tail zeros */
            while( bufpt>=buf && *bufpt=='0' ) *(bufpt--) = 0;
            if( bufpt>=buf && *bufpt=='.' ) *(bufpt--) = 0;
          }
          bufpt++;                            /* point to next free slot */
          if( exp || flag_exp ){
            *(bufpt++) = infop->charset[0];
            if( exp<0 ){ *(bufpt++) = '-'; exp = -exp; } /* sign of exp */
            else       { *(bufpt++) = '+'; }
            if( exp>=100 ){
              *(bufpt++) = (exp/100)+'0';                /* 100's digit */
              exp %= 100;
  	    }
            *(bufpt++) = exp/10+'0';                     /* 10's digit */
            *(bufpt++) = exp%10+'0';                     /* 1's digit */
          }
	}
        /* The converted number is in buf[] and zero terminated. Output it.
        ** Note that the number is in the usual order, not reversed as with
        ** integer conversions. */
        length = (long)bufpt-(long)buf;
        bufpt = buf;

        /* Special case:  Add leading zeros if the flag_zeropad flag is
        ** set and we are not left justified */
        if( flag_zeropad && !flag_leftjustify && length < width){
          int i;
          int nPad = width - length;
          for(i=width; i>=nPad; i--){
            bufpt[i] = bufpt[i-nPad];
          }
          i = prefix!=0;
          while( nPad-- ) bufpt[i++] = '0';
          length = width;
        }
#endif
        break;
      case etSIZE:
        *(va_arg(ap,int*)) = count;
        length = width = 0;
        break;
      case etPERCENT:
        buf[0] = '%';
        bufpt = buf;
        length = 1;
        break;
      case etCHARLIT:
      case etCHARX:
        c = buf[0] = (xtype==etCHARX ? va_arg(ap,int) : *++fmt);
        if( precision>=0 ){
          if( precision>etBUFSIZE-1 ) precision = etBUFSIZE-1; 
          for(idx=1; idx<precision; idx++) buf[idx] = c;
          length = precision;
	}else{
          length =1;
	}
        bufpt = buf;
        break;
      case etSTRING:
        zMem = bufpt = va_arg(ap,char*);
        if( bufpt==0 ) bufpt = "";
        length = strlen(bufpt);
        if( precision>=0 && precision<length ) length = precision;
        break;
      case etDYNAMIC:
        zExtra = zMem = bufpt = va_arg(ap,char*);
        if( bufpt==0 ) bufpt = "";
        length = strlen(bufpt);
        if( precision>=0 && precision<length ) length = precision;
        break;
      case etHTMLIZE:
        zMem = va_arg(ap,char*);
        if( zMem==0 ) zMem = "";
        zExtra = bufpt = htmlize(zMem, -1);
        length = strlen(bufpt);
        if( precision>=0 && precision<length ) length = precision;
        break;
      case etHTTPIZE:
        zMem = va_arg(ap,char*);
        if( zMem==0 ) zMem = "";
        zExtra = bufpt = httpize(zMem, -1);
        length = strlen(bufpt);
        if( precision>=0 && precision<length ) length = precision;
        break;
      case etURLIZE:
        zMem = va_arg(ap,char*);
        if( zMem==0 ) zMem = "";
        zExtra = bufpt = urlize(zMem, -1);
        length = strlen(bufpt);
        if( precision>=0 && precision<length ) length = precision;
        break;
      case etERROR:
        buf[0] = '%';
        buf[1] = c;
        errorflag = 0;
        idx = 1+(c!=0);
        (*func)(arg,"%",idx);
        count += idx;
        if( c==0 ) fmt--;
        break;
    }/* End switch over the format type */
    /*
    ** The text of the conversion is pointed to by "bufpt" and is
    ** "length" characters long.  The field width is "width".  Do
    ** the output.
    */
    if( !flag_leftjustify ){
      register int nspace;
      nspace = width-length;
      if( nspace>0 ){
        if( flag_center ){
          nspace = nspace/2;
          width -= nspace;
          flag_leftjustify = 1;
	}
        count += nspace;
        while( nspace>=etSPACESIZE ){
          (*func)(arg,spaces,etSPACESIZE);
          nspace -= etSPACESIZE;
        }
        if( nspace>0 ) (*func)(arg,spaces,nspace);
      }
    }
    if( length>0 ){
      (*func)(arg,bufpt,length);
      count += length;
    }
    if( flag_leftjustify ){
      register int nspace;
      nspace = width-length;
      if( nspace>0 ){
        count += nspace;
        while( nspace>=etSPACESIZE ){
          (*func)(arg,spaces,etSPACESIZE);
          nspace -= etSPACESIZE;
        }
        if( nspace>0 ) (*func)(arg,spaces,nspace);
      }
    }
    if( zExtra ){
      free(zExtra);
    }
  }/* End for loop over the format string */
  return errorflag ? -1 : count;
} /* End of function */


/* This structure is used to store state information about the
** write to memory that is currently in progress.
*/
struct sgMprintf {
  char *zBase;     /* A base allocation */
  char *zText;     /* The string collected so far */
  int  nChar;      /* Length of the string so far */
  int  nAlloc;     /* Amount of space allocated in zText */
};

/* 
** This function implements the callback from vxprintf. 
**
** This routine add nNewChar characters of text in zNewText to
** the sgMprintf structure pointed to by "arg".
*/
static void mout(void *arg, char *zNewText, int nNewChar){
  struct sgMprintf *pM = (struct sgMprintf*)arg;
  if( pM->nChar + nNewChar + 1 > pM->nAlloc ){
    pM->nAlloc = pM->nChar + nNewChar*2 + 1;
    if( pM->zText==pM->zBase ){
      pM->zText = malloc(pM->nAlloc);
      if( pM->zText && pM->nChar ) memcpy(pM->zText,pM->zBase,pM->nChar);
    }else{
      char *z = realloc(pM->zText, pM->nAlloc);
      if( z==0 ){
        free(pM->zText);
        pM->nChar = 0;
        pM->nAlloc = 0;
      }
      pM->zText = z;
    }
  }
  if( pM->zText ){
    memcpy(&pM->zText[pM->nChar], zNewText, nNewChar);
    pM->nChar += nNewChar;
    pM->zText[pM->nChar] = 0;
  }
}

/*
** mprintf() works like printf(), but allocations memory to hold the
** resulting string and returns a pointer to the allocated memory.  Use
** free() to release the memory allocated.
*/
char *mprintf(const char *zFormat, ...){
  va_list ap;
  struct sgMprintf sMprintf;
  char *zNew;
  char zBuf[200];

  sMprintf.nChar = 0;
  sMprintf.nAlloc = sizeof(zBuf);
  sMprintf.zText = zBuf;
  sMprintf.zBase = zBuf;
  va_start(ap,zFormat);
  vxprintf(mout,&sMprintf,zFormat,ap);
  va_end(ap);
  sMprintf.zText[sMprintf.nChar] = 0;
  if( sMprintf.zText==sMprintf.zBase ){
    zNew = malloc( sMprintf.nChar+1 );
    if( zNew ) memcpy(zNew, zBuf, sMprintf.nChar+1);
  }else{
    zNew = realloc(sMprintf.zText,sMprintf.nChar+1);
    if( zNew==0 ){
      free(sMprintf.zText);
    }
  }
  if( zNew==0 ) exit(1);
  return zNew;
}

/* This is the varargs version of mprintf.  
*/
char *vmprintf(const char *zFormat, va_list ap){
  struct sgMprintf sMprintf;
  char zBuf[200];
  sMprintf.nChar = 0;
  sMprintf.zText = zBuf;
  sMprintf.nAlloc = sizeof(zBuf);
  sMprintf.zBase = zBuf;
  vxprintf(mout,&sMprintf,zFormat,ap);
  sMprintf.zText[sMprintf.nChar] = 0;
  if( sMprintf.zText==sMprintf.zBase ){
    sMprintf.zText = malloc( sMprintf.nChar+1 );
    if( sMprintf.zText ) memcpy(sMprintf.zText, zBuf, sMprintf.nChar+1);
  }else{
    char *z = realloc(sMprintf.zText,sMprintf.nChar+1);
    if( z==0 ){
      free(sMprintf.zText);
    }
    sMprintf.zText = z;
  }
  if( sMprintf.zText==0 ) exit(1);
  return sMprintf.zText;
}

/* 
** This function implements the callback from vxprintf. 
**
** This routine add nNewChar characters of text in zNewText to
** the sgMprintf structure pointed to by "arg". Unlink mout(), it
** doesn't grow the buffer, but truncates the output.
*/
static void bout(void *arg, char *zNewText, int nNewChar){
  struct sgMprintf *pM = (struct sgMprintf*)arg;
  if( pM->nChar + nNewChar + 1 <= pM->nAlloc ){
    memcpy(&pM->zText[pM->nChar], zNewText, nNewChar);
    pM->zText[pM->nChar+nNewChar] = 0;
  }
  pM->nChar += nNewChar;
}

/*
** bprintf() works like snprintf(), but uses the more advanced formatting
** of vxprintf(). Returns the number of bytes needed to write the full
** formatted string. Formatted buffer is always NUL terminated.
*/
int bprintf(char* zBuf, int nBuflen, const char *zFormat, ...){
  va_list ap;
  struct sgMprintf sMprintf;

  if( nBuflen <= 0 ) return -1;

  sMprintf.nChar = 0;
  sMprintf.nAlloc = nBuflen;
  sMprintf.zText = zBuf;

  /* bout() NUL terminates... _if_ it's called */
  sMprintf.zText[0] = 0;

  va_start(ap,zFormat);
  vxprintf(bout,&sMprintf,zFormat,ap);
  va_end(ap);

  return sMprintf.nChar;
}

/*
** appendf() is basically an accumulating version of bprintf. Each call
** will append a NUL terminated to the previous output. If nCurlen is
** NULL, the function will calculate the current length of the buffer itself
** (but it _does_ assume that it's NUL terminated).
*/
int appendf(char* zBuf, int* nCurlen, int nBuflen, const char *zFormat, ...){
  va_list ap;
  struct sgMprintf sMprintf;

  if( nBuflen <= 0 ) return -1;

  sMprintf.nChar = nCurlen ? *nCurlen : strlen(zBuf);
  sMprintf.nAlloc = nBuflen;
  sMprintf.zText = zBuf;

  if( sMprintf.nChar<nBuflen ){
    /* bout() NUL terminates... _if_ it's called */
    sMprintf.zText[sMprintf.nChar] = 0;
  }

  va_start(ap,zFormat);
  vxprintf(bout,&sMprintf,zFormat,ap);
  va_end(ap);

  if( nCurlen ) *nCurlen = sMprintf.nChar;
  return sMprintf.nChar;
}

/* 
** This function implements the callback from vxprintf. 
**
** This routine sends nNewChar characters of text in zNewText to
** CGI reply content buffer.
*/
static void sout(void *NotUsed, char *zNewText, int nNewChar){
  cgi_append_content(zNewText, nNewChar);
}

/*
** This routine works like "printf" except that it has the
** extra formatting capabilities such as %h and %t.
*/
void cgi_printf(const char *zFormat, ...){
  va_list ap;
  va_start(ap,zFormat);
  vxprintf(sout,0,zFormat,ap);
  va_end(ap);
}

/*
** This routine works like "vprintf" except that it has the
** extra formatting capabilities such as %h and %t.
*/
void cgi_vprintf(const char *zFormat, va_list ap){
  vxprintf(sout,0,zFormat,ap);
}

/*
** Make the given string safe for HTML by converting every "<" into "&lt;",
** every ">" into "&gt;" and every "&" into "&amp;".  Return a pointer
** to a new string obtained from malloc().
**
** We also encode " as &quot; so that it can appear as an argument
** to markup.
*/
char *htmlize(const char *zIn, int n){
  int c;
  int i = 0;
  int count = 0;
  char *zOut;

  if( n<0 ) n = strlen(zIn);
  while( i<n && (c = zIn[i])!=0 ){
    switch( c ){
      case '<':   count += 4;       break;
      case '>':   count += 4;       break;
      case '&':   count += 5;       break;
      case '"':   count += 6;       break;
      default:    count++;          break;
    }
    i++;
  }
  i = 0;
  zOut = malloc( count+1 );
  if( zOut==0 ) return 0;
  while( n-->0 && (c = *zIn)!=0 ){
    switch( c ){
      case '<':   
        zOut[i++] = '&';
        zOut[i++] = 'l';
        zOut[i++] = 't';
        zOut[i++] = ';';
        break;
      case '>':   
        zOut[i++] = '&';
        zOut[i++] = 'g';
        zOut[i++] = 't';
        zOut[i++] = ';';
        break;
      case '&':   
        zOut[i++] = '&';
        zOut[i++] = 'a';
        zOut[i++] = 'm';
        zOut[i++] = 'p';
        zOut[i++] = ';';
        break;
      case '"':   
        zOut[i++] = '&';
        zOut[i++] = 'q';
        zOut[i++] = 'u';
        zOut[i++] = 'o';
        zOut[i++] = 't';
        zOut[i++] = ';';
        break;
      default:
        zOut[i++] = c;
        break;
    }
    zIn++;
  }
  zOut[i] = 0;
  return zOut;
}


/*
** Encode a string for HTTP.  This means converting lots of
** characters into the "%HH" where H is a hex digit.  It also
** means converting spaces to "+".
**
** This is the opposite of DeHttpizeString below.
*/
static char *EncodeHttp(const char *zIn, int n, int encodeSlash){
  int c;
  int i = 0;
  int count = 0;
  char *zOut;
  int other;
# define IsSafeChar(X)  \
     (isalnum(X) || (X)=='.' || (X)=='$' || (X)=='-' || (X)=='_' || (X)==other)

  if( zIn==0 ) return 0;
  if( n<0 ) n = strlen(zIn);
  other = encodeSlash ? 'a' : '/';
  while( i<n && (c = zIn[i])!=0 ){
    if( IsSafeChar(c) || c==' ' ){
      count++;
    }else{
      count += 3;
    }
    i++;
  }
  i = 0;
  zOut = malloc( count+1 );
  if( zOut==0 ) return 0;
  while( n-->0 && (c = *zIn)!=0 ){
    if( IsSafeChar(c) ){
      zOut[i++] = c;
    }else if( c==' ' ){
      zOut[i++] = '+';
    }else{
      zOut[i++] = '%';
      zOut[i++] = "0123456789ABCDEF"[(c>>4)&0xf];
      zOut[i++] = "0123456789ABCDEF"[c&0xf];
    }
    zIn++;
  }
  zOut[i] = 0;
  return zOut;
}

/*
** Convert the input string into a form that is suitable for use as
** a token in the HTTP protocol.  Spaces are encoded as '+' and special
** characters are encoded as "%HH" where HH is a two-digit hexidecimal
** representation of the character.  The "/" character is encoded
** as "%2F".
*/
char *httpize(const char *z, int n){
  return EncodeHttp(z, n, 1);
}

/*
** Convert the input string into a form that is suitable for use as
** a token in the HTTP protocol.  Spaces are encoded as '+' and special
** characters are encoded as "%HH" where HH is a two-digit hexidecimal
** representation of the character.  The "/" character is not encoded
** by this routine. 
*/
char *urlize(const char *z, int n){
  return EncodeHttp(z, n, 0);
}

/*
** Convert a single HEX digit to an integer
*/
static int AsciiToHex(int c){
  if( c>='a' && c<='f' ){
    c += 10 - 'a';
  }else if( c>='A' && c<='F' ){
    c += 10 - 'A';
  }else if( c>='0' && c<='9' ){
    c -= '0';
  }else{
    c = 0;
  }
  return c;
}

/*
** Remove the HTTP encodings from a string.  The conversion is done
** in-place.
*/
void dehttpize(char *z){
  int i, j;
  i = j = 0;
  while( z[i] ){
    switch( z[i] ){
      case '%':
        if( z[i+1] && z[i+2] ){
          z[j] = AsciiToHex(z[i+1]) << 4;
          z[j] |= AsciiToHex(z[i+2]);
          i += 2;
        }
        break;
      case '+':
        z[j] = ' ';
        break;
      default:
        z[j] = z[i];
        break;
    }
    i++;
    j++;
  }
  z[j] = 0;
}

/*
** The characters used for HTTP base64 encoding.
*/
static unsigned char zBase[] = 
  "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~";

/*
** Encode a string using a base-64 encoding.
** The encoding can be reversed using the <b>decode64</b> function.
**
** Space to hold the result comes from malloc().
*/
unsigned char *encode64(const unsigned char *zData, int nData){
  unsigned char *z64;
  int i, n;

  if( nData<=0 ){
    nData = strlen((const char *)zData);
  }
  z64 = malloc( (nData*4)/3 + 6 );
  if(z64==0) return 0;
  for(i=n=0; i+2<nData; i+=3){
    z64[n++] = zBase[ (zData[i]>>2) & 0x3f ];
    z64[n++] = zBase[ ((zData[i]<<4) & 0x30) | ((zData[i+1]>>4) & 0x0f) ];
    z64[n++] = zBase[ ((zData[i+1]<<2) & 0x3c) | ((zData[i+2]>>6) & 0x03) ];
    z64[n++] = zBase[ zData[i+2] & 0x3f ];
  }
  if( i+1<nData ){
    z64[n++] = zBase[ (zData[i]>>2) & 0x3f ];
    z64[n++] = zBase[ ((zData[i]<<4) & 0x30) | ((zData[i+1]>>4) & 0x0f) ];
    z64[n++] = zBase[ ((zData[i+1]<<2) & 0x3c) ];
  }else if( i<nData ){
    z64[n++] = zBase[ (zData[i]>>2) & 0x3f ];
    z64[n++] = zBase[ ((zData[i]<<4) & 0x30) ];
  }
  z64[n] = 0;
  return z64;
}

/*
** This function treats its input as a base-64 string and returns the
** decoded value of that string.  Characters of input that are not
** valid base-64 characters (such as spaces and newlines) are ignored.
**
** Space to hold the decoded string is obtained from malloc().
*/
unsigned char *decode64(const unsigned char *z64){
  unsigned char *zData;
  int n64;
  int i, j;
  int a, b, c, d;
  static int isInit = 0;
  static int trans[128];

  if( !isInit ){
    for(i=0; i<128; i++){ trans[i] = 0; }
    for(i=0; zBase[i]; i++){ trans[zBase[i] & 0x7f] = i; }
    isInit = 1;
  }
  n64 = strlen((const char *)z64);
  while( n64>0 && z64[n64-1]=='=' ) n64--;
  zData = malloc( (n64*3)/4 + 4 );
  if( zData==0 ) return 0;
  for(i=j=0; i+3<n64; i+=4){
    a = trans[z64[i] & 0x7f];
    b = trans[z64[i+1] & 0x7f];
    c = trans[z64[i+2] & 0x7f];
    d = trans[z64[i+3] & 0x7f];
    zData[j++] = ((a<<2) & 0xfc) | ((b>>4) & 0x03);
    zData[j++] = ((b<<4) & 0xf0) | ((c>>2) & 0x0f);
    zData[j++] = ((c<<6) & 0xc0) | (d & 0x3f);
  }
  if( i+2<n64 ){
    a = trans[z64[i] & 0x7f];
    b = trans[z64[i+1] & 0x7f];
    c = trans[z64[i+2] & 0x7f];
    zData[j++] = ((a<<2) & 0xfc) | ((b>>4) & 0x03);
    zData[j++] = ((b<<4) & 0xf0) | ((c>>2) & 0x0f);
  }else if( i+1<n64 ){
    a = trans[z64[i] & 0x7f];
    b = trans[z64[i+1] & 0x7f];
    zData[j++] = ((a<<2) & 0xfc) | ((b>>4) & 0x03);
  }
  zData[j] = 0;
  return zData;
}

/*
** Send a reply indicating that the HTTP request was malformed
*/
static void malformed_request(void){
  cgi_set_status(501, "Not Implemented");
  cgi_printf(
    "<html><body>Unrecognized HTTP Request</body></html>\n"
  );
  cgi_reply();
  exit(0);
}

/*
** Remove the first space-delimited token from a string and return
** a pointer to it.  Add a NULL to the string to terminate the token.
** Make *zLeftOver point to the start of the next token.
*/
static char *extract_token(char *zInput, char **zLeftOver){
  char *zResult = 0;
  if( zInput==0 ){
    if( zLeftOver ) *zLeftOver = 0;
    return 0;
  }
  while( isspace(*zInput) ){ zInput++; }
  zResult = zInput;
  while( *zInput && !isspace(*zInput) ){ zInput++; }
  if( *zInput ){
    *zInput = 0;
    zInput++;
    while( isspace(*zInput) ){ zInput++; }
  }
  if( zLeftOver ){ *zLeftOver = zInput; }
  return zResult;
}

/*
** This routine handles a single HTTP request which is coming in on
** standard input and which replies on standard output.
*/
void cgi_handle_http_request(void){
  char *z, *zToken;
  int i;
  struct sockaddr_in remoteName;
  size_t size = sizeof(struct sockaddr_in);
  char zLine[2000];     /* A single line of input. */

  fullHttpReply = 1;
  if( fgets(zLine, sizeof(zLine), stdin)==0 ){
    malformed_request();
  }
  zToken = extract_token(zLine, &z);
  if( zToken==0 ){
    malformed_request();
  }
  if( strcmp(zToken,"GET")!=0 && strcmp(zToken,"POST")!=0
      && strcmp(zToken,"HEAD")!=0 ){
    malformed_request();
  }
  putenv("GATEWAY_INTERFACE=CGI/1.0");
  putenv(mprintf("REQUEST_METHOD=%s",zToken));
  zToken = extract_token(z, &z);
  if( zToken==0 ){
    malformed_request();
  }
  putenv(mprintf("REQUEST_URI=%s", zToken));
  for(i=0; zToken[i] && zToken[i]!='?'; i++){}
  if( zToken[i] ) zToken[i++] = 0;
  putenv(mprintf("PATH_INFO=%s", zToken));
  putenv(mprintf("QUERY_STRING=%s", &zToken[i]));
  if( getpeername(fileno(stdin), (struct sockaddr*)&remoteName, &size)>=0 ){
    putenv(mprintf("REMOTE_ADDR=%s", inet_ntoa(remoteName.sin_addr)));
  }
 
  /* Get all the optional fields that follow the first line.
  */
  while( fgets(zLine,sizeof(zLine),stdin) ){
    char *zFieldName;
    char *zVal;

    zFieldName = extract_token(zLine,&zVal);
    if( zFieldName==0 || *zFieldName==0 ) break;
    while( isspace(*zVal) ){ zVal++; }
    i = strlen(zVal);
    while( i>0 && isspace(zVal[i-1]) ){ i--; }
    zVal[i] = 0;
    for(i=0; zFieldName[i]; i++){ zFieldName[i] = tolower(zFieldName[i]); }
    if( strcmp(zFieldName,"user-agent:")==0 ){
      putenv(mprintf("HTTP_USER_AGENT=%s", zVal));
    }else if( strcmp(zFieldName,"content-length:")==0 ){
      putenv(mprintf("CONTENT_LENGTH=%s", zVal));
    }else if( strcmp(zFieldName,"referer:")==0 ){
      putenv(mprintf("HTTP_REFERER=%s", zVal));
    }else if( strcmp(zFieldName,"host:")==0 ){
      putenv(mprintf("HTTP_HOST=%s", zVal));
    }else if( strcmp(zFieldName,"content-type:")==0 ){
      putenv(mprintf("CONTENT_TYPE=%s", zVal));
    }else if( strcmp(zFieldName,"cookie:")==0 ){
      putenv(mprintf("HTTP_COOKIE=%s", zVal));
    }else if( strcmp(zFieldName,"if-none-match:")==0 ){
      putenv(mprintf("HTTP_IF_NONE_MATCH=%s", zVal));
    }else if( strcmp(zFieldName,"if-modified-since:")==0 ){
      putenv(mprintf("HTTP_IF_MODIFIED_SINCE=%s", zVal));
    }
  }

  cgi_init();
}

/*
** Maximum number of child processes that we can have running
** at one time before we start slowing things down.
*/
#define MAX_PARALLEL 2

/*
** Implement an HTTP server daemon.
*/
void cgi_http_server(int iPort){
  int listener;                /* The server socket */
  int connection;              /* A socket for each individual connection */
  fd_set readfds;              /* Set of file descriptors for select() */
  size_t lenaddr;              /* Length of the inaddr structure */
  int child;                   /* PID of the child process */
  int nchildren = 0;           /* Number of child processes */
  struct timeval delay;        /* How long to wait inside select() */
  struct sockaddr_in inaddr;   /* The socket address */
  int opt = 1;                 /* setsockopt flag */

  memset(&inaddr, 0, sizeof(inaddr));
  inaddr.sin_family = AF_INET;
  inaddr.sin_addr.s_addr = INADDR_ANY;
  inaddr.sin_port = htons(iPort);
  listener = socket(AF_INET, SOCK_STREAM, 0);
  if( listener<0 ){
    fprintf(stderr,"Can't create a socket\n");
    exit(1);
  }

  /* if we can't terminate nicely, at least allow the socket to be reused */
  setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

  if( bind(listener, (struct sockaddr*)&inaddr, sizeof(inaddr))<0 ){
    fprintf(stderr,"Can't bind to port %d\n", iPort);
    exit(1);
  }
  listen(listener,10);
  while( 1 ){
    if( nchildren>MAX_PARALLEL ){
      /* Slow down if connections are arriving too fast */
      sleep( nchildren-MAX_PARALLEL );
    }
    delay.tv_sec = 60;
    delay.tv_usec = 0;
    FD_ZERO(&readfds);
    FD_SET( listener, &readfds);
    if( select( listener+1, &readfds, 0, 0, &delay) ){
      lenaddr = sizeof(inaddr);
      connection = accept(listener, (struct sockaddr*)&inaddr, &lenaddr);
      if( connection>=0 ){
        child = fork();
        if( child!=0 ){
          if( child>0 ) nchildren++;
          close(connection);
        }else{
          close(0);
          dup(connection);
          close(1);
          dup(connection);
          close(2);
          dup(connection);
          close(connection);
          return;
        }
      }
    }
    /* Bury dead children */
    while( waitpid(0, 0, WNOHANG)>0 ){
      nchildren--;
    }
  }
  /* NOT REACHED */  
  exit(1);
}

/*
** Returns an RFC822-formatted time string suitable for HTTP headers, among
** other things.
** Returned timezone is always GMT as required by HTTP/1.1 specification.
**
** See http://www.faqs.org/rfcs/rfc822.html, section 5
** and http://www.faqs.org/rfcs/rfc2616.html, section 3.3.
*/
char *cgi_rfc822_datestamp(time_t now){
  static char *azDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", 0};
  static char *azMonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
                             "Aug", "Sep", "Oct", "Nov", "Dec", 0};
  struct tm *pTm;
  pTm = gmtime(&now);
  if( pTm==0 ) return "";
  return mprintf("%s, %d %s %02d %02d:%02d:%02d GMT",
                 azDays[pTm->tm_wday], pTm->tm_mday, azMonths[pTm->tm_mon],
                 pTm->tm_year+1900, pTm->tm_hour, pTm->tm_min, pTm->tm_sec);
}

/*
** Parse an RFC822-formatted timestamp as we'd expect from HTTP and return
** a Unix epoch time. <= zero is returned on failure.
**
** Note that this won't handle all the _allowed_ HTTP formats, just the
** most popular one (the one generated by cgi_rfc822_datestamp(), actually).
*/
time_t cgi_rfc822_parsedate(const char *zDate){
  static char *azMonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
                             "Aug", "Sep", "Oct", "Nov", "Dec", 0};
  struct tm t;
  char zIgnore[16];
  char zMonth[16];

  memset(&t, 0, sizeof(t));
  if( 7==sscanf(zDate, "%12[A-Za-z,] %d %12[A-Za-z] %d %d:%d:%d", zIgnore,
                       &t.tm_mday, zMonth, &t.tm_year, &t.tm_hour, &t.tm_min,
                       &t.tm_sec)){

    if( t.tm_year > 1900 ) t.tm_year -= 1900;
    for(t.tm_mon=0; azMonths[t.tm_mon]; t.tm_mon++){
      if( !strncasecmp( azMonths[t.tm_mon], zMonth, 3 )){
        return mkgmtime(&t);
      }
    }
  }

  return 0;
}

/*
** Check the objectTime against the If-Modified-Since request header. If the
** object time isn't any newer than the header, we immediately send back
** a 304 reply and exit.
*/
void cgi_modified_since(time_t objectTime){
  const char *zIf = getenv("HTTP_IF_MODIFIED_SINCE");
  if( zIf==0 ) return;
  if( objectTime > cgi_rfc822_parsedate(zIf) ) return;
  cgi_set_status(304,"Not Modified");
  cgi_reset_content();
  cgi_reply();
  exit(0);
}

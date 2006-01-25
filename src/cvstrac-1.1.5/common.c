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
** Routines shared by many pages
*/
#include "config.h"
#include "common.h"

/*
** Output a string with the following substitutions:
**
**     %T      The title of the current page.
**     %N      Project name
**     %V      CVSTrac version number
**     %%      The character '%'
*/
static void output_with_subst(const char *zText, const char *zTitle){
  int i;
  while( zText[0] ){
    for(i=0; zText[i] && zText[i]!='%'; i++){}
    if( i>0 ) cgi_append_content(zText, i);
    if( zText[i]==0 ) break;
    switch( zText[i+1] ){
      case 'T':
        zText += i+2;
        cgi_printf("%h", zTitle);
        break;
      case 'N':
        zText += i+2;
        cgi_printf("%h", g.zName);
        break;
      case 'V':
        zText += i+2;
        cgi_printf("%h", "@VERSION@");
        break;
      case '%':
        zText += i+2;
        cgi_printf("%%");
        break;
      default:
        zText += i+1;
        cgi_printf("%%");
        break;
    }
  }
}

/*
** Read the whole contents of a file into memory obtained from
** malloc().  Return a pointer to the file contents.  Be sure
** the string is null terminated.
**
** A NULL pointer is returned if the file could not be read
** for any reason.
*/
static char *common_readfile(const char *zFilename) {
  FILE *fp;
  char *zContent = NULL;
  size_t n;

  if ((fp = fopen(zFilename, "r")) != NULL) {
    fseek(fp, 0, SEEK_END);
    if ((n = ftell(fp)) > 0) {
      if ((zContent = (char *)malloc(n+1)) == NULL) {
        fclose(fp);
        return NULL;
      }
      fseek(fp, 0, SEEK_SET);
      if ((n = fread(zContent, 1, n, fp)) == 0) {
        free(zContent);
        fclose(fp);
        return NULL;
      }
      zContent[n] = '\0';
    }
    else {
      zContent = strdup("");
    }
    fclose(fp);
  }
  return zContent;
}

/*
** Generate an error message screen.
*/
void common_err(const char *zFormat, ...){
  char *zMsg;

  va_list ap;
  va_start(ap, zFormat);
  zMsg = vmprintf(zFormat, ap);
  va_end(ap);
  cgi_reset_content();
  common_standard_menu(0,0);
  common_header("Oops!");
  @ <p>The following error has occurred:</p>
  @ <blockquote>%h(zMsg)</blockquote>
  if( g.okSetup ){
    @ <p>Query parameters:<p>
    cgi_print_all();
  }
  common_footer();
  cgi_append_header("Pragma: no-cache\r\n");
  cgi_reply();
  exit(0);
}

/*
** The menu on the top bar of every page is defined by the following
** variables.
*/
static const char *azLink[50];
static int nLink = 0;

/*
** Prepopulate the set of menu items with a standard set that includes
** links to all top-level pages except for zOmit.  If zOmit is NULL then
** include all items.
**
** If zSrchUrl is not NULL then use it as the URL for the "Search" menu
** option.
*/
void common_standard_menu(const char *zOmit, const char *zSrchUrl){
  const char *zLimit;
  if( g.okNewTkt ){
    azLink[nLink++] = "tktnew";
    azLink[nLink++] = "Ticket";
  }
  if( g.okCheckout ){
    azLink[nLink++] = "dir";
    azLink[nLink++] = "Browse";
  }
  if( g.okRead ){
    azLink[nLink++] = "reportlist";
    azLink[nLink++] = "Reports";
    azLink[nLink++] = "timeline";
    azLink[nLink++] = "Timeline";
  }
  if( g.okRdWiki ){
    azLink[nLink++] = "wiki";
    azLink[nLink++] = "Wiki";
  }
  if( g.okRdWiki || g.okRead || g.okCheckout ){
    azLink[nLink++] = zSrchUrl ? zSrchUrl : "search";
    azLink[nLink++] = "Search";
  }
  if( g.okCheckin ){
    azLink[nLink++] = "msnew";
    azLink[nLink++] = "Milestone";
  }
  if( g.okWrite && !g.isAnon ){
    azLink[nLink++] = "userlist";
    azLink[nLink++] = "Users";
  }
  if( g.okAdmin ){
    azLink[nLink++] = "setup";
    azLink[nLink++] = "Setup";
  }
  azLink[nLink++] = "login";
  if( g.isAnon ){
    azLink[nLink++] = "Login";
  }else{
    azLink[nLink++] = "Logout";
  }
  if( g.isAnon && (zLimit = db_config("throttle",0))!=0 && atof(zLimit)>0.0 ){
    azLink[nLink++] = "honeypot";
    azLink[nLink++] = "0Honeypot";
  }
  if( nLink>2 ){
    azLink[nLink++] = "index";
    azLink[nLink++] = "Home";
  }
  if( zOmit ){
    int j;
    for(j=0; j<nLink; j+=2){
      if( azLink[j][0]==zOmit[0] && strcmp(zOmit,azLink[j])==0 ){
        azLink[j] = azLink[nLink-2];
        azLink[j+1] = azLink[nLink-1];
        nLink -= 2;
        break;
      }
    }
  }
  azLink[nLink] = 0;
}

/*
** Add a new entry to the menu that will appear at the top of the
** page.  zUrl is the URL that we jump to when the user clicks on
** the link and zName is the text that appears in the link.
*/
void common_add_menu_item(
  const char *zUrl,      /* The URL to be appended */
  const char *zName      /* The menu entry name */
){
  azLink[nLink++] = zUrl;
  azLink[nLink++] = zName;
  azLink[nLink] = 0;
}

/*
** Replace an existing menu item with the new version given here.
*/
void common_replace_menu_item(
  const char *zUrl,      /* The new URL */
  const char *zName      /* The menu entry name to be replaced */
){
  int i;
  for(i=0; i<nLink; i+=2){
    if( strcmp(azLink[i+1],zName)==0 ){
      if( zUrl==0 ){
        azLink[i] = azLink[nLink-2];
        azLink[i+1] = azLink[nLink-1];
        nLink--;
      }else{
        azLink[i] = zUrl;
      }
      break;
    }
  }
}

/*
** Function used for sorting entries in azLinks[]
*/
static int link_compare(const void *a, const void *b){
  const char **pA = (const char **)a;
  const char **pB = (const char **)b;
  return strcmp(pA[1], pB[1]);
}


/*
** Generate an HTML header common to all web pages.  zTitle is the
** title for the page.  azLinks is an array of URI/Name pairs that
** are used to generate quick-links on the title bar.
*/
void common_header(const char *zTitle, ...){
  int i = 0;
  int brk;
  const char *zHeader = 0;
  va_list ap;
  char *zTitleTxt;

  va_start(ap, zTitle);
  zTitleTxt = vmprintf(zTitle, ap);
  va_end(ap);
  zHeader = db_config("header", 0);
  if( zHeader && zHeader[0] ){
    char *z;
    if( zHeader[0]=='/' && (z = common_readfile(zHeader))!=0 ){
      zHeader = z;
    }
    output_with_subst(zHeader, zTitleTxt);
  }else{
    @ <html>
    @ <head>
    @ <title>%h(g.zName): %h(zTitleTxt)</title>
    @ </head>
    @ <body bgcolor="white">
  }
  @ <table width="100%%" cellpadding=2 border=0>
  @ <tr><td bgcolor="%s(BORDER1)" class="border1">
  @ <table width="100%%" border=0 cellpadding=2 cellspacing=0>
  @ <tr bgcolor="%s(BG1)" class="bkgnd1">
  @ <td valign="top" align="left">
  @ <big><b>%h(g.zName) - %h(zTitleTxt)</b></big><br>
  if( !g.isAnon ){
    @ <small>Logged in as <i>%h(g.zUser)</i></small>
  }else{
    @ <a href="honeypot"><small><notatag arg="meaningless"></small></a>
    @ <small>Not logged in</small>
  }
  @ </td>
  @ <td valign="bottom" align="right">
  if( azLink ){
    int j;
    int nChar;
    nLink /= 2;
    qsort(azLink, nLink, 2*sizeof(azLink[0]), link_compare);
    nChar = 0;
    for(i=0; azLink[i]; i+=2){
      nChar += strlen(azLink[i+1]) + 3;
    }
    if( nChar<=60 ){
      brk = nChar;
    }else if( nChar<=120 ){
      brk = nChar/2;
    }else{
      brk = nChar/3;
    }
    nChar = 0;
    @ <nobr>
    for(i=0, j=1; azLink[i] && azLink[i+1]; i+=2, j++){
      char *z = azLink[i+1];
      if( z[0]<'A' ) z++;
      @ [<a href="%h(azLink[i])">%h(z)</a>]&nbsp;
      nChar += strlen(azLink[i+1]) + 3;
      if( nChar>=brk && azLink[i+2] ){
        nChar = 0;
        @ </nobr><br><nobr>
      }
    }
    @ </nobr>
  }
  if( i==0 ){
    @ &nbsp;
  }
  @ </td></tr></table>
  @ </td></tr></table>
  @ <p>
  free(zTitleTxt);
}

/*
** Generate a common footer
*/
void common_footer(void){
  const char *zFooter;
  zFooter = db_config("footer", 0);
  if( zFooter && zFooter[0] ){
    char *z;
    if( zFooter[0]=='/' && (z = common_readfile(zFooter))!=0 ){
      zFooter = z;
    }
    output_with_subst(zFooter, "");
  }else{
    @ <p><small><small>
    @ <a href="about">CVSTrac version @VERSION@</a>
    @ </small></small></p>
    @ </body></html>
  }
}

/*
** Generate an about screen
**
** WEBPAGE: /about
*/
void common_about(void){
  login_check_credentials();
  common_add_menu_item("index", "Home");
  common_header("About This Server", azLink);
  @ <p>This website is implemented using CVSTrac version @VERSION@.</p>
  @
  @ <p>CVSTrac implements a patch-set and bug tracking system for CVS.
  @ For additional information, visit the CVSTrac homepage at</p>
  @ <blockquote>
  @ <a href="http://www.cvstrac.org/">http://www.cvstrac.org/</a>
  @ </blockquote>
  @
  @ <p>Copyright &copy; 2002-2003 <a href="mailto:drh@hwaci.com">
  @ D. Richard Hipp</a>.
  @ The CVSTrac server is released under the terms of the GNU
  @ <a href="http://www.gnu.org/copyleft/gpl.html">
  @ General Public License</a>.</p>
  common_footer(); 
}

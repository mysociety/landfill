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
** Code to generate the index page
*/
#include "config.h"
#include "index.h"

/*
** Return TRUE if the given string contains at least one non-space
** character
*/
static int not_blank(const char *z){
  while( isspace(*z) ){ z++; }
  return *z!=0;
}

/*
** WEBPAGE: /
** WEBPAGE: /index
** WEBPAGE: /index.html
** WEBPAGE: /mainmenu
*/
void index_page(void){
  int cnt = 0;
  login_check_credentials();
  common_standard_menu("index", 0);

  common_add_help_item("CvstracDocumentation");

  /* If the user has wiki read permission and a wiki page named HomePage
  ** exists and is not empty and is locked (meaning that only an 
  ** administrator could have created it), then use that page as the
  ** main menu rather than the built-in main menu.
  **
  ** The built-in main menu is always reachable using the /mainmenu URL
  ** instead of "/index" or "/".
  */
  if( g.okRdWiki && g.zPath[0]!='m' ){
    char *zBody = db_short_query(
        "SELECT text FROM wiki WHERE name='HomePage' AND locked");
    if( zBody && not_blank(zBody) ){
      common_add_nav_item("mainmenu", "Main Menu");
      common_header("Home Page");
      /* menu_sidebar(); */
      output_wiki(zBody, "", "HomePage");
      common_footer();
      return;
    }
  }

  /* Render the built-in main-menu page.
  */
  common_header("Main Index");
  @ <table cellpadding="10">
  @
  if( g.zPath[0]=='m' ){
    @ <tr>
    @ <td valign="top">
    @ <a href="index"><b>Home Page</b></a>
    @ </td>
    @ <td valign="top">
    @ View the Wiki-based homepage for this project.
    @ </td>
    @ </tr>
    @
    cnt++;
  }
  if( g.okNewTkt ){
    @ <tr>
    @ <td valign="top">
    @ <a href="tktnew"><b>Ticket</b></a>
    @ </td>
    @ <td valign="top">
    @ Create a new Ticket with a defect report or enhancement request.
    @ </td>
    @ </tr>
    @
    cnt++;
  }
  if( g.okCheckout ){
    @ <tr>
    @ <td valign="top">
    @ <a href="%h(default_browse_url())"><b>Browse</b></a>
    @ </td>
    @ <td valign="top">
    @ Browse the %s(g.scm.zName) repository tree.
    @ </td>
    @ </tr>
    @
    cnt++;
  }   
  if( g.okRead ){
    @ <tr>
    @ <td valign="top">
    @ <a href="reportlist"><b>Reports</b></a>
    @ </td>
    @ <td valign="top">
    @ View summary reports of Tickets.
    @ </td>
    @ </tr>
    @
    cnt++;
  }
  if( g.okRdWiki || g.okRead || g.okCheckout ){
    @ <tr>
    @ <td valign="top">
    @ <a href="timeline"><b>Timeline</b></a>
    @ </td>
    @ <td valign="top">
    @ View a chronology of Check-Ins and Ticket changes.
    @ </td>
    @ </tr>
    @ 
    cnt++;
  }
  if( g.okRdWiki ){
    @ <tr>
    @ <td valign="top">
    @ <a href="wiki"><b>Wiki</b></a>
    @ </td>
    @ <td valign="top">
    @ View the Wiki documentation pages.
    @ </td>
    @ </tr>
    @
    cnt++;
  }
  if( g.okRead || g.okCheckout || g.okRdWiki ){
    const char *az[5];
    int n=0;
    if( g.okRead ) az[n++] = "Tickets";
    if( g.okCheckout ) az[n++] = "Check-ins";
    if( g.okRdWiki ) az[n++] = "Wiki pages";
    if( g.okCheckout ) az[n++] = "Filenames";
    @ <tr>
    @ <td valign="top">
    @ <a href="search"><b>Search</b></a>
    @ </td>
    @ <td valign="top">
    if( n==4 ){
      @ Search for keywords in %s(az[0]), %s(az[1]), %s(az[2]), and/or %s(az[3])
    }else if( n==3 ){
      @ Search for keywords in %s(az[0]), %s(az[1]), and/or %s(az[2])
    }else if( n==2 ){
      @ Search for keywords in %s(az[0]) and/or %s(az[1])
    }else{
      @ Search for keywords in %s(az[0])
    }
    @ </td>
    @ </tr>
    @
    cnt++;
  }
  if( g.okCheckin ){
    @ <tr>
    @ <td valign="top">
    @ <a href="msnew"><b>Milestones</b></a>
    @ </td>
    @ <td valign="top">
    @ Create new project milestones.
    @ </td>
    @ </tr>
    @ 
    cnt++;
  }
  if( g.okWrite && !g.isAnon ){
    @ <tr>
    @ <td valign="top">
    @ <a href="userlist"><b>User</b></a>
    @ </td>
    @ <td valign="top">
    @ Create, edit, and delete users.
    @ </td>
    @ </tr>
    @ 
    cnt++;
  }
  if( g.okAdmin ){
    @ <tr>
    @ <td valign="top">
    @ <a href="setup"><b>Setup</b></a>
    @ </td>
    @ <td valign="top">
    @ Setup global system parameters.
    @ </td>
    @ </tr>
    @ 
    cnt++;
  }
  if( g.okRdWiki ){
    @ <tr>
    @ <td valign="top">
    @ <a href="wiki?p=CvstracDocumentation"><b>Documentation</b></a>
    @ </td>
    @ <td valign="top">
    @ Read the online manual.
    @ </td>
    @ </tr>
    @
    cnt++;
  }
  if( g.isAnon ){
    if( cnt==0 ){
      login_needed();
      return;
    }
    @ <tr>
    @ <td valign="top">
    @ <a href="login"><b>Login</b></a>
    @ </td>
    @ <td valign="top">
    @ Log in.
    @ </td>
    @ </tr>
    @ 
  }else{
    @ <tr>
    @ <td valign="top">
    @ <a href="logout"><b>Logout</b></a>
    @ </td>
    @ <td valign="top">
    @ Log off or change password.
    @ </td>
    @ </tr>
    @ 
  }
  @ </table>
  @ 
  common_footer();
}

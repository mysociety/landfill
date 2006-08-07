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
** This file contains code for generating the login and logout screens.
*/
#define _XOPEN_SOURCE
#include <unistd.h>
#include "config.h"
#include "login.h"
#include <time.h>

/*
** Return the name of the login cookie
*/
static char *login_cookie_name(void){
  return mprintf("%s_login", g.zName);
}

/*
** WEBPAGE: /login
** WEBPAGE: /logout
**
** Generate the login page
*/
void login_page(void){
  const char *zUsername, *zPasswd, *zGoto;
  const char *zNew1, *zNew2;
  char *zErrMsg = "";
  char *z;

  login_check_credentials();
  zUsername = P("u");
  zPasswd = P("p");
  zGoto = P("g");
  if( P("out")!=0 ){
    const char *zCookieName = login_cookie_name();
    cgi_set_cookie(zCookieName, "", 0, 86400 * 28);
    db_execute("DELETE FROM cookie WHERE cookie='%q'", zCookieName);
    cgi_redirect(PD("nxp","index"));
    return;
  }
  if( !g.isAnon && zPasswd && (zNew1 = P("n1"))!=0 && (zNew2 = P("n2"))!=0 ){
    z = db_short_query("SELECT passwd FROM user WHERE id='%q'", g.zUser);
    if( z==0 || z[0]==0 || strcmp(crypt(zPasswd,z),z)!=0 ){
      sleep(1);
      zErrMsg = 
         @ <p><font color="red">
         @ You entered an incorrect old password while attempting to change
         @ your password.  Your password is unchanged.
         @ </font></p>
      ;
    }else if( strcmp(zNew1,zNew2)!=0 ){
      zErrMsg = 
         @ <p><font color="red">
         @ The two copies of your new passwords do not match.
         @ Your password is unchanged.
         @ </font></p>
      ;
    }else{
      db_execute(
         "UPDATE user SET passwd='%q' WHERE id='%q'",
         crypt(zNew1,zPasswd), g.zUser
      );
      user_cvs_write(0);
      cgi_redirect("index");
      return;
    }
  }
  if( zUsername!=0 && zPasswd!=0 && strcmp(zUsername,"anonymous")!=0 ){
    z = db_short_query("SELECT passwd FROM user WHERE id='%q'", zUsername);
    if( z==0 || z[0]==0 || strcmp(crypt(zPasswd,z),z)!=0 ){
      sleep(1);
      zErrMsg = 
         @ <p><font color="red">
         @ You entered an incorrect username and/or password
         @ </font></p>
      ;
    }else{
      time_t now;
      char *zDigest;
      const char *zAddr;
      const char *zAgent;
      char zHash[200];
      char zRawDigest[16];
      MD5Context ctx;

      time(&now);
      sprintf(zHash,"%d%d%.19s", getpid(), (int)now, zPasswd);
      MD5Init(&ctx);
      MD5Update(&ctx, zHash, strlen(zHash));
      MD5Final(zRawDigest, &ctx);
      zDigest = encode64(zRawDigest, 16);
      zAddr = getenv("REMOTE_ADDR");
      if( zAddr==0 ) zAddr = "0.0.0.0";
      zAgent = getenv("HTTP_USER_AGENT");
      if( zAgent==0 ) zAgent = "Unknown";
      db_execute(
        "BEGIN;"
        "DELETE FROM cookie WHERE expires<=%d;"
        "INSERT INTO cookie(cookie,user,expires,ipaddr,agent)"
        "  VALUES('%q','%q',%d,'%q','%q');"
        "COMMIT;",
        now, zDigest, zUsername, now+28*3600*24, zAddr, zAgent
      );
      cgi_set_cookie(login_cookie_name(), zDigest, 0, 86400 * 28);
      cgi_redirect(PD("nxp","index"));
      return;
    }
    free(z);
  }
  common_standard_menu("login", 0);
  common_header("Login/Logout");
  @ %s(zErrMsg)
  @ <form action="login" method="POST">
  if( P("nxp") ){
    @ <input type="hidden" name="nxp" value="%h(P("nxp"))">
  }
  @ <table align="left" hspace="10">
  @ <tr>
  @   <td align="right">User ID:</td>
  @   <td><input type="text" name="u" value="" size=30></td>
  @ </tr>
  @ <tr>
  @  <td align="right">Password:</td>
  @   <td><input type="password" name="p" value="" size=30></td>
  @ </tr>
  @ <tr>
  @   <td></td>
  @   <td><input type="submit" name="in" value="Login"></td>
  @ </tr>
  @ </table>
  if( g.isAnon ){
    @ <p>To login
  }else{
    @ <p>You are current logged in as <b>%h(g.zUser)</b></p>
    @ <p>To change your login to a different user
  }
  @ enter the user-id and password at the left and press the
  @ "Login" button.  Your user name will be stored in a browser cookie.
  @ You must configure your web browser to accept cookies in order for
  @ the login to take.</p>
  z = db_short_query("SELECT id FROM user WHERE id='anonymous'");
  if( z && z[0] ){
    @ <p>This server is configured to allow limited access to users
    @ who are not logged in.</p>
  }
  if( !g.isAnon ){
    @ <br clear="both"><hr>
    @ <p>To log off the system (and delete your login cookie)
    @  press the following button:<br>
    @ <input type="submit" name="out" value="Logout"></p>
  }
  @ </p>
  @ </form>
  if( !g.isAnon && g.okPassword ){
    @ <br clear="both"><hr>
    @ <p>To change your password, enter your old password and your
    @ your new password twice below then press the "Change Password"
    @ button.</p>
    @ <form action="login" method="POST">
    @ <table>
    @ <tr><td align="right">Old Password:</td>
    @ <td><input type="password" name="p" size=30></td></tr>
    @ <tr><td align="right">New Password:</td>
    @ <td><input type="password" name="n1" size=30></td></tr>
    @ <tr><td align="right">Repeat New Password:</td>
    @ <td><input type="password" name="n2" size=30></td></tr>
    @ <tr><td></td>
    @ <td><input type="submit" value="Change Password"></td></tr>
    @ </table>
  }
  common_footer();
}

/*
** This routine examines the login cookie to see if it exists and
** contains a valid password hash.  If the login cookie checks out,
** it then sets g.zUser to the name of the user and set g.isAnon to 0.
**
** Permission variable are set as appropriate:
**
**   g.okRead        User can read bug reports and change histories
**   g.okCheckout    User can read from the CVS repository
**   g.okWrite       User can change bug reports
**   g.okCheckin     User can checking changes to CVS
**   g.okAdmin       User can add or delete other user and create new reports
**   g.okSetup       User can change CVS repositories
**   g.okPassword    User can change his password
**
**   g.okRdWiki      User can read wiki pages
**   g.okWiki        User and create or modify wiki pages
**
*/
void login_check_credentials(void){
  const char *zCookie;
  time_t now;
  char **azResult, *z;
  int i;
  const char *zUser;
  const char *zPswd;
  const char *zAddr;     /* The IP address of the browser making this request */
  const char *zAgent;    /* The type of browser */

  g.zUser = g.zHumanName = "anonymous";
  g.okPassword = 0;
  g.okRead = 0;
  g.okNewTkt = 0;
  g.okWrite = 0;
  g.okAdmin = 0;
  g.okSetup = 0;
  g.okCheckout = 0;
  g.okCheckin = 0;
  g.okRdWiki = 0;
  g.okWiki = 0;
  g.isAnon = 1;
  time(&now);

  /*
  ** Check to see if there is an anonymous user.  Everybody gets at
  ** least the permissions that anonymous enjoys.
  */
  z = db_short_query("SELECT capabilities FROM user WHERE id='anonymous'");
  if( z && z[0] ){
    for(i=0; z[i]; i++){
      switch( z[i] ){
        case 'i':   g.okCheckin = g.okCheckout = 1;  break;
        case 'j':   g.okRdWiki = 1;                  break;
        case 'k':   g.okWiki = g.okRdWiki = 1;       break;
        case 'n':   g.okNewTkt = 1;                  break;
        case 'o':   g.okCheckout = 1;                break;
        case 'p':   g.okPassword = 1;                break;
        case 'r':   g.okRead = 1;                    break;
        case 'w':   g.okWrite = g.okRead = 1;        break;
      }
    }
  }

  /*
  ** Next check to see if the user specified by "U" and "P" query
  ** parameters or by the login cookie exists
  */  
  if( (zUser = P("U"))!=0 && (zPswd = P("P"))!=0 ){
    char *z = db_short_query("SELECT passwd FROM user WHERE id='%q'", zUser);
    if( z==0 || z[0]==0 || strcmp(crypt(zPswd,z),z)!=0 ){
      return;
    }
  }else if( (zCookie = P(login_cookie_name()))!=0 && zCookie[0]!=0 ){
    zAddr = getenv("REMOTE_ADDR");
    if( zAddr==0 ) zAddr = "0.0.0.0";
    zAgent = getenv("HTTP_USER_AGENT");
    if( zAgent==0 ) zAgent = "Unknown";
    zUser = db_short_query(
      "SELECT user FROM cookie "
      "WHERE cookie='%q' "
      /* "  AND ipaddr='%q' " */ /* disabled ip address check; doesn't work due to our SSH proxying and squid stuff */
      "  AND agent='%q' "
      "  AND expires>%d",
      zCookie, /*zAddr,*/ zAgent, now);
    if( zUser==0 ){
      return;
    }
  }else{
    return;
  }

  /* If we reach here, it means that the user named "zUser" checks out.
  ** Set up appropriate permissions.
  */
  azResult = db_query(
    "SELECT name, capabilities FROM user "
    "WHERE id='%q'", zUser
  );
  if( azResult[0]==0 ){
    return;  /* Should never happen... */
  }
  g.isAnon = 0;
  g.zHumanName = azResult[0];
  g.zUser = zUser;
  cgi_logfile(0, g.zUser);
  for(i=0; azResult[1][i]; i++){
    switch( azResult[1][i] ){
      case 's':   g.okSetup = 1;
      case 'a':   g.okAdmin = g.okRead = g.okWrite = 
                              g.okNewTkt = g.okPassword = 1;
      case 'i':   g.okCheckin = g.okCheckout = 1;  break;
      case 'j':   g.okRdWiki = 1;                  break;
      case 'k':   g.okWiki = g.okRdWiki = 1;       break;
      case 'n':   g.okNewTkt = 1;                  break;
      case 'o':   g.okCheckout = 1;                break;
      case 'p':   g.okPassword = 1;                break;
      case 'r':   g.okRead = 1;                    break;
      case 'w':   g.okWrite = g.okRead = 1;        break;
    }
  }
}

/*
** Call this routine when the credential check fails.  It causes
** a redirect to the "login" page.
*/
void login_needed(void){
  const char *zUrl = getenv("REQUEST_URI");
  if( zUrl==0 ) zUrl = "index";
  cgi_redirect(mprintf("login?nxp=%T", zUrl));
}

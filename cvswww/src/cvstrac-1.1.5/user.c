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
** Routines for handling user account
*/
#define _XOPEN_SOURCE
#include <unistd.h>
#include "config.h"
#include "user.h"
#include <sys/types.h>

/*
** WEBPAGE: /userlist
*/
void user_list(void){
  char **azResult;
  int i;

  login_check_credentials();
  if( !g.okWrite && g.isAnon ){
    login_needed();
    return;
  }
  common_standard_menu("userlist", 0);
  common_add_menu_item("useredit", "Add User");
  common_header("User List");
  @ <table cellspacing=0 cellpadding=0 border=0>
  @ <tr>
  @   <th align="right"><nobr>User ID</nobr></th>
  @   <th>&nbsp;&nbsp;&nbsp;Permissions&nbsp;&nbsp;&nbsp;</th>
  @   <th><nobr>In Real Life</nobr></th>
  @ </tr>
  azResult = db_query(
    "SELECT id, name, email, capabilities FROM user ORDER BY id");
  for(i=0; azResult[i]; i+= 4){
    @ <tr>
    @ <td align="right">
    if( g.okAdmin ){
      @ <a href="useredit?id=%t(azResult[i])">
    }
    @ <nobr>%h(azResult[i])</nobr>
    if( g.okAdmin ){
      @ </a>
    }
    @ </td>
    @ <td align="center">%s(azResult[i+3])</td>
    if( azResult[i+2] && azResult[i+2][0] ){
      char *zE = azResult[i+2];
      @ <td align="left"><nobr>%h(azResult[i+1])
      @    (<a href="mailto:%h(zE)">%h(zE)</a>)</nobr></td>
    } else {
      @ <td align="left"><nobr>%h(azResult[i+1])</nobr></td>
    }
    @ </tr>
  }
  @ </table>
  @ <p><hr>
  @ <b>Notes:</b>
  @ <ol>
  @ <li><p>The permission flags are as follows:</p>
  @ <table>
  @ <tr><td>a</td><td width="10"></td>
  @     <td>Admin: Create or delete users and ticket report formats</td></tr>
  @ <tr><td>i</td><td></td>
  @     <td>Check-in: Add new code to the CVS repository</td></tr>
  @ <tr><td>j</td><td></td><td>Read-Wiki: View wiki pages</td></tr>
  @ <tr><td>k</td><td></td><td>Wiki: Create or modify wiki pages</td></tr>
  @ <tr><td>n</td><td></td><td>New: Create new tickets</td></tr>
  @ <tr><td>o</td><td></td>
  @     <td>Check-out: Read code out of the CVS repository</td></tr>
  @ <tr><td>p</td><td></td><td>Password: Change password</td></tr>
  @ <tr><td>r</td><td></td><td>Read: View tickets and change histories</td></tr>
  @ <tr><td>s</td><td></td><td>Setup: Change the CVS repository</td></tr>
  @ <tr><td>w</td><td></td><td>Write: Edit tickets</td></tr>
  @ </table>
  @ </p></li>
  @
  @ <li><p>
  @ If a user named "<b>anonymous</b>" exists, then anyone can access
  @ the server without having to log in.  The permissions on the
  @ anonymous user determine the access rights for anyone who is not
  @ logged in.
  @ </p></li>
  @
  @ <li><p>
  @ You must be using CVS version 1.11 or later in order to give users
  @ read-only access to the repository.
  @ With earlier versions of CVS, all users with check-out
  @ privileges also automatically get check-in privileges.
  @ </p></li>
  @
  @ <li><p>
  @ Changing a users ID or password modifies the <b>CVSROOT/passwd</b>,
  @ <b>CVSROOT/readers</b>, and <b>CVSROOT/writers</b> files in the CVS
  @ repository, if those files have write permission turned on.  Users
  @ IDs in <b>CVSROOT/passwd</b> that are unknown to CVSTrac are preserved.
  if( g.okSetup ){
    @ Use the "Import CVS Users" button on the 
    @ <a href="setup_user">user setup</a> page
    @ to import CVS users into CVSTrac.
  }
  @ </p></li>
  @ </ol>
  common_footer();
}

/*
** WEBPAGE: /useredit
*/
void user_edit(void){
  char **azResult;
  const char *zId, *zName, *zEMail, *zCap;
  char *oaa, *oas, *oar, *oaw, *oan, *oai, *oaj, *oao, *oap ;
  char *oak;
  int doWrite;
  int higherUser = 0;  /* True if user being edited is SETUP and the */
                       /* user doing the editing is ADMIN.  Disallow editing */

  /* Must have ADMIN privleges to access this page
  */
  login_check_credentials();
  if( !g.okAdmin ){ login_needed(); return; }

  /* Check to see if an ADMIN user is trying to edit a SETUP account.
  ** Don't allow that.
  */
  zId = P("id");
  if( zId && !g.okSetup ){
    char *zOldCaps;
    zOldCaps = db_short_query(
       "SELECT capabilities FROM user WHERE id='%q'",zId);
    higherUser = zOldCaps && strchr(zOldCaps,'s');
  }

  /* If we have all the necessary information, write the new or
  ** modified user record.  After writing the user record, redirect
  ** to the page the displays a list of users.
  */
  doWrite = zId && cgi_all("nm","em","pw") && !higherUser;
  if( doWrite ){
    const char *zOldPw;
    char zCap[20];
    int i = 0;
    int aa = P("aa")!=0;
    int ai = P("ai")!=0;
    int aj = P("aj")!=0;
    int ak = P("ak")!=0;
    int an = P("an")!=0;
    int ao = P("ao")!=0;
    int ap = P("ap")!=0;
    int ar = P("ar")!=0;
    int as = g.okSetup && P("as")!=0;
    int aw = P("aw")!=0;
    if( as ) aa = 1;
    if( aa ) ai = aw = ap = 1;
    if( aw ) an = ar = 1;
    if( ai ) ao = 1;
    if( ak ) aj = 1;
    if( aa ){ zCap[i++] = 'a'; }
    if( ai ){ zCap[i++] = 'i'; }
    if( aj ){ zCap[i++] = 'j'; }
    if( ak ){ zCap[i++] = 'k'; }
    if( an ){ zCap[i++] = 'n'; }
    if( ao ){ zCap[i++] = 'o'; }
    if( ap ){ zCap[i++] = 'p'; }
    if( ar ){ zCap[i++] = 'r'; }
    if( as ){ zCap[i++] = 's'; }
    if( aw ){ zCap[i++] = 'w'; }
    zCap[i] = 0;
    zOldPw = db_short_query("SELECT passwd FROM user WHERE id='%q'", zId);
    db_execute("DELETE FROM user WHERE id='%q'", zId);
    if( !P("delete") ){
      const char *zPw = P("pw");
      char zBuf[3];
      if( zOldPw==0 ){
        char zSeed[100];
        const char *z;
        sprintf(zSeed,"%d%.20s",getpid(),zId);
        z = crypt(zSeed, "aa");
        zBuf[0] = z[2];
        zBuf[1] = z[3];
        zBuf[2] = 0;
        zOldPw = zBuf;
      }
      db_execute(
         "INSERT INTO user(id,name,email,passwd,capabilities) "
         "VALUES('%q','%q','%q','%q','%s')",
         zId, P("nm"), P("em"), zPw[0] ? crypt(zPw, zOldPw) : zOldPw, zCap
      );
    }
    user_cvs_write(P("delete")!=0 ? zId : 0);
    cgi_redirect("userlist");
    return;
  }

  /* Load the existing information about the user, if any
  */
  zName = "";
  zEMail = "";
  zCap = "";
  oaa = oai = oaj = oak = oan = oao = oap = oar = oas = oaw = "";
  if( zId ){
    azResult = db_query(
      "SELECT name, email, capabilities FROM user WHERE id='%q'", zId
    );
    if( azResult && azResult[0] ){
      zName = azResult[0];
      zEMail = azResult[1];
      zCap = azResult[2];
      if( strchr(zCap, 'a') ) oaa = " checked";
      if( strchr(zCap, 'i') ) oai = " checked";
      if( strchr(zCap, 'j') ) oaj = " checked";
      if( strchr(zCap, 'k') ) oak = " checked";
      if( strchr(zCap, 'n') ) oan = " checked";
      if( strchr(zCap, 'o') ) oao = " checked";
      if( strchr(zCap, 'p') ) oap = " checked";
      if( strchr(zCap, 'r') ) oar = " checked";
      if( strchr(zCap, 's') ) oas = " checked";
      if( strchr(zCap, 'w') ) oaw = " checked";
    }else{
      zId = 0;
    }
  }

  /* Begin generating the page
  */
  common_standard_menu(0,0);
  if( zId ){
    common_header("Edit User %s", zId);
  }else{
    common_header("Add New User");
  }
  @ <form action="%s(g.zPath)" method="POST">
  @ <table align="left" hspace=10 vspace=10>
  @ <tr>
  @   <td align="right"><nobr>User ID:</nobr></td>
  if( zId ){
    @   <td>%h(zId) <input type="hidden" name="id" value="%h(zId)"></td>
  }else{
    @   <td><input type="text" name="id" size=10></td>
  }
  @ </tr>
  @ <tr>
  @   <td align="right"><nobr>Full Name:</nobr></td>
  @   <td><input type="text" name="nm" value="%h(zName)"></td>
  @ </tr>
  @ <tr>
  @   <td align="right"><nobr>E-Mail:</nobr></td>
  @   <td><input type="text" name="em" value="%h(zEMail)"></td>
  @ </tr>
  @ <tr>
  @   <td align="right" valign="top">Capabilities:</td>
  @   <td>
  @     <input type="checkbox" name="aa"%s(oaa)>Admin</input><br>
  @     <input type="checkbox" name="ai"%s(oai)>Check-In</input><br>
  @     <input type="checkbox" name="aj"%s(oaj)>Read Wiki</input><br>
  @     <input type="checkbox" name="ak"%s(oak)>Write Wiki</input><br>
  @     <input type="checkbox" name="an"%s(oan)>New Tkt</input><br>
  @     <input type="checkbox" name="ao"%s(oao)>Check-Out</input><br>
  @     <input type="checkbox" name="ap"%s(oap)>Password</input><br>
  @     <input type="checkbox" name="ar"%s(oar)>Read</input><br>
  if( g.okSetup ){
    @     <input type="checkbox" name="as"%s(oas)>Setup</input><br>
  }
  @     <input type="checkbox" name="aw"%s(oaw)>Write</input>
  @   </td>
  @ </tr>
  @ <tr>
  @   <td align="right">Password:</td>
  @   <td><input type="password" name="pw" value=""></td>
  @ </tr>
  if( !higherUser ){
    @ <tr>
    @   <td>&nbsp</td>
    @   <td><input type="submit" name="submit" value="Apply Changes">
    @       &nbsp;&nbsp;&nbsp;
    @       <input type="submit" name="delete" value="Delete User"></td>
    @ </tr>
  }
  @ </table>
  @ <p><b>Notes:</b></p>
  @ <ol>
  if( higherUser ){
    @ <li><p>
    @ User %h(zId) has Setup privileges and you only have Admin privileges
    @ so you are not permitted to make changes to %h(zId).
    @ </p></li>
    @
  }
  @ <li><p>
  @ If the Check-out capability is specified then
  @ the password entered here will be used to regenerate the
  @ <b>CVSROOT/passwd</b> file and will thus become the CVS password
  @ as well as the password for this server.
  @ </p></li>
  @
  @ <li><p>
  @ The <b>Check-in</b> capability means that the user ID will be written
  @ into the <b>CVSROOT/writers</b> file and thus allow write access to
  @ the CVS repository.
  @ </p></li>
  @
  @ <li><p>
  @ The <b>Read</b> and <b>Write</b> privileges give the user the ability
  @ to read and write tickets.  The <b>New Tkt</b> capability means that
  @ the user is able to create new tickets.
  @ </p></li>
  @
  @ <li><p>
  @ An <b>Admin</b> user can add other users, create new ticket report
  @ formats, and change system defaults.  But only the <b>Setup</b> user
  @ is able to change the CVS repository to which this program is linked.
  @ </p></li>
  @
  if( zId==0 || strcmp(zId,"anonymous")==0 ){
    @ <li><p>
    @ No login is required for user "<b>anonymous</b>".  The capabilities
    @ of this user are available to anyone without supplying a username or
    @ password.  To disable anonymous access, make sure there is no user
    @ with an ID of <b>anonymous</b>.
    @ </p></li>
    @
    @ <li><p>
    @ The password for the "<b>anonymous</b>" user is used for anonymous
    @ CVS access.  The recommended value for the anonymous password
    @ is "anonymous".
    @ </p></li>
  }
  @ </form>
  common_footer();
}

/*
** Remove the newline from the end of a string.
*/
static void remove_newline(char *z){
  while( *z && *z!='\n' && *z!='\r' ){ z++; }
  if( *z ){ *z = 0; }
}

/*
** Load the names of all users listed in CVSROOT/passwd into a temporary
** table "tuser".  Load the names of readonly users into a temporary table
** "treadonly".
*/
static void read_cvs_passwd_files(const char *zCvsRoot){
  FILE *f;
  char *zFile;
  char *zPswd, *zSysId;
  int i;
  char zLine[2000];
  

  db_execute(
    "CREATE TEMP TABLE tuser(id UNIQUE ON CONFLICT IGNORE,pswd,sysid,cap);"
    "CREATE TEMP TABLE treadonly(id UNIQUE ON CONFLICT IGNORE);"
  );
  if( zCvsRoot==0 ){
    zCvsRoot = db_config("cvsroot","");
  }
  zFile = mprintf("%s/CVSROOT/passwd", zCvsRoot);
  f = fopen(zFile, "r");
  free(zFile);
  if( f ){
    while( fgets(zLine, sizeof(zLine), f) ){
      remove_newline(zLine);
      for(i=0; zLine[i] && zLine[i]!=':'; i++){}
      if( zLine[i]==0 ) continue;
      zLine[i++] = 0;
      zPswd = &zLine[i];
      while( zLine[i] && zLine[i]!=':' ){ i++; }
      if( zLine[i]==0 ){
        zSysId = zLine;
      }else{
        zLine[i++] = 0;
        zSysId = &zLine[i];
      }
      db_execute("INSERT INTO tuser VALUES('%q','%q','%q','io');",
         zLine, zPswd, zSysId);
    }
    fclose(f);
  }
  zFile = mprintf("%s/CVSROOT/readers", zCvsRoot);
  f = fopen(zFile, "r");
  free(zFile);
  if( f ){
    while( fgets(zLine, sizeof(zLine), f) ){
      remove_newline(zLine);
      db_execute("INSERT INTO treadonly VALUES('%q');", zLine);
    }
    fclose(f);
  }
  zFile = mprintf("%s/CVSROOT/writers", zCvsRoot);
  f = fopen(zFile, "r");
  free(zFile);
  if( f ){
    db_execute("INSERT INTO treadonly SELECT id FROM tuser");
    while( fgets(zLine, sizeof(zLine), f) ){
      remove_newline(zLine);
      db_execute("DELETE FROM treadonly WHERE id='%q';", zLine);
    }
    fclose(f);
  }
  db_execute(
    "UPDATE tuser SET cap='o' WHERE id IN (SELECT id FROM treadonly);"
  );
}

/*
** Read the CVSROOT/passwd, CVSROOT/reader, and CVSROOT/write files.
** record infomation gleaned from those files in the local database.
*/
void user_cvs_read(void){
  char *zAnonCap;
  db_add_functions();
  db_execute("BEGIN");
  zAnonCap = 
    db_short_query("SELECT capabilities FROM user WHERE id='anonymous'");
  if( zAnonCap==0 ) zAnonCap = "";
  read_cvs_passwd_files(0);
  db_execute(
    "REPLACE INTO user(id,name,email,passwd,capabilities) "
    "  SELECT n.id, o.name, o.email, n.pswd, "
    "      cap_or(cap_and(o.capabilities, 'aknrsw'), n.cap) "
    "  FROM tuser as n, user as o WHERE n.id=o.id;"
    "INSERT OR IGNORE INTO user(id,name,email,passwd,capabilities) "
    "  SELECT id, id, '', pswd,"
    "      cap_or((SELECT capabilities FROM user WHERE id='anonymous'),cap) "
    "  FROM tuser;"
    "COMMIT;"
  );
}

/*
** Write the CVSROOT/passwd and CVSROOT/writer files based on the current
** state of the database.
*/
int user_cvs_write(const char *zOmit){
  FILE *pswd;
  FILE *wrtr;
  FILE *rdr;
  char **az;
  int i;
  char *zFile;
  const char *zCvsRoot;
  const char *zUser = 0;
  const char *zWriteEnable;

  db_execute("BEGIN");
  if( access("/etc/passwd",R_OK)==0 ){
    db_execute("CREATE TEMP TABLE etcpw(name,pw,uid,gid,ex,home,shell)");
    db_execute("COPY etcpw FROM '/etc/passwd' USING DELIMITERS ':'");
    if( geteuid()!=0 ){
      zUser = db_short_query("SELECT name FROM etcpw WHERE uid=%d", geteuid());
    }else if( getuid()!=0 ){
      zUser = db_short_query("SELECT name FROM etcpw WHERE uid=%d", getuid());
    }
  }
  if( zUser==0 ) zUser = db_config("cvs_user_id","nobody");

  /* If the "write_cvs_passwd" configuration option exists and is "no"
  ** (or at least begins with an 'n') then disallow writing to the
  ** CVSROOT/passwd file.
  */
  zWriteEnable = db_config("write_cvs_passwd","yes");
  if( zWriteEnable[0]=='n' ){
    db_execute("COMMIT");
    return 1;
  }

  zCvsRoot = db_config("cvsroot","");
  read_cvs_passwd_files(zCvsRoot);
  db_execute("COMMIT");
  zFile = mprintf("%s/CVSROOT/passwd", zCvsRoot);
  pswd = fopen(zFile, "w");
  free(zFile);
  if( pswd==0 ) return 0;
  zFile = mprintf("%s/CVSROOT/writers", zCvsRoot);
  wrtr = fopen(zFile, "w");
  free(zFile);
  zFile = mprintf("%s/CVSROOT/readers", zCvsRoot);
  rdr = fopen(zFile, "w");
  free(zFile);
  az = db_query(
      "SELECT id, passwd, '%q', capabilities FROM user "
      "UNION ALL "
      "SELECT id, pswd, sysid, cap FROM tuser "
      "  WHERE id NOT IN (SELECT id FROM user) "
      "ORDER BY id", zUser);
  for(i=0; az[i]; i+=4){
    if( strchr(az[i+3],'o')==0 ) continue;
    if( zOmit && strcmp(az[i],zOmit)==0 ) continue;
    fprintf(pswd, "%s:%s:%s\n", az[i], az[i+1], az[i+2]);
    if( strchr(az[i+3],'i')!=0 ){
      if( wrtr!=0 ) fprintf(wrtr,"%s\n", az[i]);
    }else{
      if( rdr!=0 ) fprintf(rdr,"%s\n", az[i]);
    }
  }
  db_query_free(az);
  fclose(pswd);
  if( wrtr ) fclose(wrtr);
  if( rdr ) fclose(rdr);
  return 1;
}

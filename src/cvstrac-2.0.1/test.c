/*
** Generate a test pages of HTML.
**
** To disable these tests change the WEBPAGE in the comment headers
** to WEB*PAGE and comment out the code.
*/
#include "config.h"
#include "test.h"

/*
** Generate an output page that contains information about the
** current environment.
*/
extern char **environ;
void test_cgi_vardump(void){
  int i;
  char zPwd[1000];
  getcwd(zPwd, sizeof(zPwd));
  @ <html>
  @ <head><title>Test Page</title></head>
  @ <body bgcolor="white">
  @ <h1>Test Page</h1>
  cgi_print_all();
  @ <h2>Environment variables:</h2>
  @ <p>
  for(i=0; environ[i]; i++){
    @ %h(environ[i]) <br />
  }
  @ </p>
  @ <h2>Working directory and user id</h2>
  @ <p>
  @ pwd = %s(zPwd)<br />
  @ uid = %d(getpid())<br />
  @ euid = %d(geteuid())<br />
  @ argc = %d(g.argc)<br />
  for(i=0; i<g.argc; i++){
    @ argv[%d(i)] = %s(g.argv[i])<br />
  }
  @ </p>
  @ </body></html>
}

/*
** WEBPAGE: /test
*/
void test_page(void){
  login_check_credentials();
  if( !g.okSetup ){ login_needed(); return; }
  test_cgi_vardump();
}

/*
** WEBPAGE: /formtest1
*/
void form_test_1(void){
  login_check_credentials();
  if( !g.okSetup ){ login_needed(); return; }
  @ <html>
  @ <body bgcolor="white">
  @ <h1>Parameters:</h1>
  cgi_print_all();
  @ <h1>New Form:</h1>
  @ <form action="formtest1" method="POST" enctype="multipart/form-data">
  @ X = <input type="text" name="x" size=30><br>
  @ F1 = <input type="file" name="f1"><br>
  @ Y = <input type="text" name="y" size=30><br>
  @ F2 = <input type="file" name="f2"><br>
  @ <input type="submit" value="Submit">
  @ </form>
}

/*
** WEBPAGE: /test2
*/
void test_page_2(void){
  cgi_redirect("test");
}

/*
** WEBPAGE: /endlessloop
*/
void endlessloop_page(void){
  while(1){sleep(5);}
}

/*
** WEBPAGE: /setcookie
*/
void set_cookie_page(void){
  const char *zName = PD("name","C_NAME");
  const char *zVal = PD("value","C_VALUE");
  login_check_credentials();
  if( !g.okSetup ){ login_needed(); return; }
  cgi_set_cookie(zName, zVal, 0, 0);
  cgi_redirect("");
}

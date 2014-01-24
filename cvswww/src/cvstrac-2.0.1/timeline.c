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
** This file contains code used to generate the "timeline"
** family of reports
*/
#include <time.h>
#include <assert.h>
#include "config.h"
#include "timeline.h"

/*
** Parse a text time description.  The format can be 
**
**      YYYY-BB-DD  HH:MM:SS
**      YYYY-bbbb-DD   HH:MM:SS
**
** Where BB is the numerical month number (between 01 and 12) and
** bbbb is the month name.  The ":SS" or the "HH:MM:SS" part may
** be omitted.  All times are local.
**
** Zero (0) is returned if the time does not parse.
*/
time_t parse_time(const char *zTime){
  char zMonth[30];
  struct tm sTm;
  int y, d, h, m, s;
  int i, n;
  static struct {
    char *zName;
    int iMon;
  } aMName[] = {
    { "january",    1 },
    { "february",   2 },
    { "march",      3 },
    { "april",      4 },
    { "may",        5 },
    { "june",       6 },
    { "july",       7 },
    { "jly",        7 },
    { "august",     8 },
    { "sepember",   9 },
    { "october",   10 },
    { "november",  11 },
    { "december",  12 },
  };

  y = d = h = m = s = 0;
  n = sscanf(zTime, "%d-%20[^-]-%d %d:%d:%d", &y, zMonth, &d, &h, &m, &s);
  if( n<3 || n==4 ) return 0;
  for(i=0; zMonth[i]; i++){
    if( isupper(zMonth[i]) ) zMonth[i] = tolower(zMonth[i]);
  }
  n = strlen(zMonth);
  memset(&sTm, 0, sizeof(sTm));
  if( n<=2 && isdigit(zMonth[0]) && (n==1 || isdigit(zMonth[1]))
      && (sTm.tm_mon = atoi(zMonth))>=1 && sTm.tm_mon<=12 ){
    sTm.tm_mon--;
  }else{
    for(i=0; i<sizeof(aMName)/sizeof(aMName[0]); i++){
      if( strncmp(zMonth, aMName[i].zName, n)==0 ){
        sTm.tm_mon = aMName[i].iMon - 1;
        break;
      }
    }
#if CVSTRAC_I18N
    if( i>=sizeof(aMName)/sizeof(aMName[0]) ){
      for(i=0; i<12; i++){
        if( strncasecmp(zMonth, nl_langinfo(ABMON_1+i), n)==0
                 || strncasecmp(zMonth, nl_langinfo(MON_1+i),n )==0 ){
          sTm.tm_mon = i;
          break;
        }
      }
    }
#endif
  }
  sTm.tm_year = y - 1900;
  sTm.tm_mday = d;
  sTm.tm_hour = h;
  sTm.tm_min = m;
  sTm.tm_sec = s;
  sTm.tm_isdst = -1;
  return mktime(&sTm);
}

/*
** zState is a string that defines the state of a ticket.  It is one
** of "new", "review", "defer", "active", "tested", "fixed", or "closed".
** Return true if the string is either active or new.
*/
static int isActiveState(const char *zState){
  return zState[0]=='n' || zState[0]=='a';
}

/*
** zState is a string that defines the state of a ticket.  It is one
** of "new", "review", "defer", "active", "tested", "fixed", or "closed".
** Return true if the string is either fixed, tested, or closed.
*/
static int isFixedState(const char *zState){
  return zState[0]=='f' || zState[0]=='t' || zState[0]=='c';
}

/*
** Converts time_t to day number. This is used only to calculate
** day boundaries, it doesn't make any sense calendar-wise.
*/
int timet_to_day(time_t thisDate){
  struct tm *pTm;
  
  pTm = localtime(&thisDate);
  return (pTm->tm_year+1900)*1000 + pTm->tm_yday;
}


int same_day(time_t date1, time_t date2){
  return timet_to_day(date1)==timet_to_day(date2);
}

/*
** WEBPAGE: /timeline
*/
void timeline_page(void){
  const char *zEnd;       /* Day at which timeline ends */
  time_t begin, end;      /* Beginning and ending times for the timeline */
  char **az;  
  time_t thisDate;
  int thisDay, lastDay;
  int inTable = 0;
  int i;
  struct tm *pTm;
  int len = 0;
  int days = 30;
  int showM;       /* Show milestones if 1.  Do not show if 0 */
  int showC;       /* Show trunk-only if 3, all checkins if 2,
                   ** branch checkins if 1, none if 0 */
  int showS;       /* 0: no status changes 1: active & new
                   ** 2: all status chnganges 3: everything
                   ** 9: show no tickets*/
  int showA;       /* Show assignment changes if 1 */
  int showW;       /* Show changes to Wiki pages if 1 */
  int showT;       /* Show attachment additions */
  int divM;        /* Divide timeline by milestones if 1 */
  int divT;        /* Divide timeline by days if 1 */
  const char *zCkinPrfx;   /* Only show checkins of files with this prefix */
  char zDate[200];
  char zSQL[4000];
  char *zTimelineCookieName = mprintf("%t_timeline",g.zName);

  login_check_credentials();
  if( !g.okRead && !g.okCheckout ){ login_needed(); return; }
  throttle(1,0);
  history_update(0);
  if( P("x") || P(zTimelineCookieName) ){

    /* load in the contents of the cookie first, if possible. This gives us a
    ** "blend" of user-specific defaults and things in the query string. x=1 is
    ** set not only for form submission, but links from other pages (i.e.
    ** /dir's "Activity" link), so it's important that we combine them nicely.
    */
    if( P("x")==0 && P(zTimelineCookieName) ){
      char *zName;
      char *zValue;
      char *z = strdup(PD(zTimelineCookieName,""));

      /* The cookie is formatted as a standard CGI query string. What we can
      ** do is parse through it at set values as parameters. This code is
      ** basically add_param_list() from cgi.c except that we don't override
      ** existing parameters since we don't want to stomp on values in the
      ** query string.
      */
      while( *z ){
        while( isspace(*z) ){ z++; }
        zName = z;
        while( *z && *z!='=' && *z!='&' ){ z++; }
        if( *z=='=' ){
          *z = 0;
          z++;
          zValue = z;
          while( *z && *z!='&' ){ z++; }
          if( *z ){
            *z = 0;
            z++;
          }
          dehttpize(zValue);
          if( P(zName)==0 ) cgi_set_parameter(zName, zValue);
        }else{
          if( *z ){ *z++ = 0; }
          if( P(zName)==0 ) cgi_set_parameter(zName, "");
        }
      }
    }

    /* If form is submited, store all timeline settings in cookie and format 
    ** timeline according to values from submited form. 
    */
    if( P("x") ){
      /* The "timeline_cookie_life" value is in days, and we'll use 90 by
      ** default.  Setting it to zero will, of course, disable timeline
      ** cookies.
      */
      int nCookieLife = 86400*atoi(db_config("timeline_cookie_life","90"));
      if( nCookieLife ){
        char *zCookie = mprintf(
          "d=%d&m=%d&c=%d&s=%d&a=%d&w=%d&t=%d&dm=%d&dt=%d",
          atoi(PD("d","30")), atoi(PD("m","0")), atoi(PD("c","0")),
          atoi(PD("s","0")),  atoi(PD("a","0")), atoi(PD("w","0")),
          atoi(PD("t","0")), atoi(PD("dm","0")), atoi(PD("dt","0")));
        cgi_set_cookie(zTimelineCookieName, zCookie, 0, nCookieLife);
      }
    }

    showM = PD("m","0")[0] - '0';
    showC = PD("c","0")[0] - '0';
    showS = PD("s","0")[0] - '0';
    showA = PD("a","0")[0] - '0';
    showW = PD("w","0")[0] - '0';
    showT = PD("t","0")[0] - '0';
    divM = PD("dm","0")[0] - '0';
    divT = PD("dt","0")[0] - '0';

    if( P("set") && P("d") && g.okAdmin ){
      char zVal[12];
      zVal[0] = showA + '0';
      zVal[1] = showC + '0';
      zVal[2] = showM + '0';
      zVal[3] = showS + '0';
      zVal[4] = showW + showT*2 + '0';
      zVal[5] = divM + divT*2 + '0';
      zVal[6] = 0;
      db_execute("REPLACE INTO config(name,value) "
                 "VALUES('timeline_format','%q%q')", zVal, P("d"));
      db_config(0,0);
    }
  }else{
    const char *zFormat;
    zFormat = db_config("timeline_format", 0);
    if( zFormat && zFormat[0] ){
      showA = zFormat[0]-'0';
      showC = zFormat[1]-'0';
      showM = zFormat[2]-'0';
      showS = zFormat[3]-'0';
      showW = zFormat[4]-'0';
      showT = (showW>>1)&1;
      showW &= 1;
      divM = zFormat[5]-'0';
      divT = divM/2;
      divM %= 2;
      days = atoi(&zFormat[6]);
    }else{
      showM = showS = 1;
      showC = 2;
      showA = showW = showT = 0;
      divM = 0;
      divT = 1;
    }
  }
  zCkinPrfx = P("px");
  if( zCkinPrfx==0 || zCkinPrfx[0]==0 || showC==0 ){
    zCkinPrfx = 0;
  }else{
    zCkinPrfx = sqlite3_mprintf("%q", zCkinPrfx);
  }
  if( !g.okRead ){
    showS = showA = 0;
  }
  zEnd = P("e");
  if( zEnd==0 || strcmp(zEnd,"today")==0 || (end = parse_time(zEnd))==0 ){
    time(&end);
  }
  pTm = localtime(&end);
  pTm->tm_hour = 0;
  pTm->tm_min = 0;
  pTm->tm_sec = 0;
  end = mktime(pTm);
  i = atoi(PD("d","0"));
  if( i>0 ) days = i;
  begin = end - 3600*24*days;
  end += 3600*24 - 1;
  if( showM || (showC && zCkinPrfx==0) || divM ){
    appendf(zSQL,&len,sizeof(zSQL),
       "SELECT "
       "  date AS 'time', 1 AS 'type', user, milestone, branch, cn, message "
       "FROM chng WHERE date<=%d AND date>=%d",
       (int)end, (int)begin);
    if( showM==0 && divM==0 ){
      appendf(zSQL,&len,sizeof(zSQL)," AND NOT milestone");
    }
    if( showC==0 || zCkinPrfx  ){
      appendf(zSQL,&len,sizeof(zSQL)," AND milestone%s", showM ? "" : "==1");
    }else if( showC!=2 ){
      appendf(zSQL,&len,sizeof(zSQL)," AND (milestone%s OR branch%c='')",
              showM ? "":"==1", (showC==3) ? '=' : '!');
    }else if( showM==0 && divM ){
      /* we need to know where releases are, but we won't show them. We also
      ** want checkins since showC!=0 at this point.
      */
      appendf(zSQL,&len,sizeof(zSQL)," AND milestone<=1");
    }
    if( showC && zCkinPrfx==0 ){
      appendf(zSQL,&len,sizeof(zSQL),
         " UNION ALL "
         "SELECT "
         "  inspect.inspecttime AS 'time',"
         "  8 AS type,"
         "  inspect.inspector,"
         "  0 AS milestone,"
         "  chng.branch,"
         "  inspect.cn,"
         "  inspect.result "
         "FROM inspect, chng "
         "WHERE inspect.cn=chng.cn "
           "AND inspect.inspecttime<=%d AND inspect.inspecttime>=%d ",
         (int)end, (int)begin);
      if( showC!=2 ){
        appendf(zSQL,&len,sizeof(zSQL)," AND branch%c=''",
                (showC==3) ? '=' : '!');
      }
    }
  }
  if( zCkinPrfx ){
    if( len>0 ){
      appendf(zSQL,&len,sizeof(zSQL)," UNION ALL ");
    }
    appendf(zSQL,&len,sizeof(zSQL),
       "SELECT DISTINCT "
       "  chng.date AS 'time',"
       "  1 AS type,"
       "  chng.user,"
       "  0 AS milestone,"
       "  chng.branch,"
       "  chng.cn,"
       "  chng.message "
       "FROM chng, filechng "
       "WHERE filechng.cn=chng.cn AND filechng.filename LIKE '%.100s%%'"
       "  AND chng.date<=%d AND chng.date>=%d ",
       zCkinPrfx, (int)end, (int)begin);

    appendf(zSQL,&len,sizeof(zSQL),
       " UNION ALL "
       "SELECT DISTINCT "
       "  inspect.inspecttime AS 'time',"
       "  8 AS type,"
       "  inspect.inspector,"
       "  0 AS milestone,"
       "  chng.branch,"
       "  inspect.cn,"
       "  inspect.result "
       "FROM inspect, chng, filechng "
       "WHERE inspect.cn=chng.cn AND filechng.cn=chng.cn "
         "AND filechng.filename LIKE '%.100s%%' "
         "AND inspect.inspecttime<=%d AND inspect.inspecttime>=%d ",
       zCkinPrfx, (int)end, (int)begin);
  }
  if( (showS || showA) && showS!=9 ){
    if( len>0 ){
      appendf(zSQL,&len,sizeof(zSQL)," UNION ALL ");
    }
    appendf(zSQL,&len,sizeof(zSQL),
       "SELECT origtime AS 'time', 2 AS 'type', owner, type, NULL, tn, title "
       "FROM ticket WHERE origtime<=%d AND origtime>=%d",
       (int)end, (int)begin);
    if( showS ){
      appendf(zSQL,&len,sizeof(zSQL),
         " UNION ALL "
         "SELECT chngtime AS 'time', 3 AS 'type', user, oldval AS 'aux1', "
         "       newval AS 'aux2', tn AS 'ref', NULL as 'text' "
         "FROM tktchng "
         "WHERE fieldid='status' AND chngtime<=%d AND chngtime>=%d ",
         (int)end, (int)begin);
      if( showS==1 ){
        appendf(zSQL,&len,sizeof(zSQL),
          " AND ("
          "(newval IN ('new','active') AND oldval NOT IN ('new','active')) OR"
          "(newval NOT IN ('new','active') AND oldval IN ('new','active')))");
      }
    }
    if( showS==3 ){
      appendf(zSQL,&len,sizeof(zSQL),
         " UNION ALL "
         "SELECT chngtime AS 'time', 6 AS 'type', user, '' AS 'aux1', "
         "       '' AS 'aux2', tn AS 'ref', NULL as 'text' "
         "FROM tktchng "
         "WHERE fieldid!='status' AND fieldid!='assignedto'"
         "  AND chngtime<=%d AND chngtime>=%d ",
         (int)end, (int)begin);
    }
    if( showA ){
      appendf(zSQL,&len,sizeof(zSQL),
         " UNION ALL "
         "SELECT chngtime AS 'time', 4 AS 'type', user, oldval AS 'aux1', "
         "       newval AS 'aux2', tn AS 'ref', NULL as 'text' "
         "FROM tktchng "
         "WHERE fieldid='assignedto' AND chngtime<=%d AND chngtime>=%d",
         (int)end, (int)begin);
    }
  }
  if( showW ){
    if( len>0 ){
      appendf(zSQL,&len,sizeof(zSQL)," UNION ALL ");
    }
    appendf(zSQL,&len,sizeof(zSQL),
       "SELECT -invtime AS 'time', 5 AS 'type', who, NULL, NULL, "
       "       name as 'ref', NULL as 'text' "
       "FROM wiki "
       "WHERE invtime>=%d AND invtime<=%d",
       -(int)end, -(int)begin);
  }
  if( showT ){
    if( len>0 ){
      appendf(zSQL,&len,sizeof(zSQL)," UNION ALL ");
    }
    appendf(zSQL,&len,sizeof(zSQL),
       "SELECT date AS 'time', 7 AS 'type', user, tn, size, "
       "       fname as 'ref', atn as 'text' "
       "FROM attachment "
       "WHERE date>=%d AND date<=%d AND tn>0",
       (int)begin, (int)end);
  }

  if( len==0 ){
    static char *azDummy[] = { 0 };
    az = azDummy;
  }else{
    appendf(zSQL,&len,sizeof(zSQL), " ORDER BY 1 DESC, 2");
    az = db_query("%s",zSQL);
  }

  lastDay = 0;
  common_standard_menu("timeline", "search?t=1&c=1");

  common_add_action_item(
      mprintf( "timeline.rss?d=%d&px=%T", days, zCkinPrfx),
      "RSS"
  );
 
  common_add_help_item("CvstracTimeline");

  if( zCkinPrfx && zCkinPrfx[0]
      && db_exists("SELECT dir FROM file WHERE dir='%q'", zCkinPrfx)
  ){
    common_add_action_item( mprintf("%s?d=%h",default_browse_url(),zCkinPrfx),
                            "Directory" );
  }

  common_header("Timeline");
  if( P("debug1") ){
    @ <p>%h(zSQL)</p><hr>
  }
  for(i=0; az[i]; i+=7){
    char *zIcon = 0;
    char *zBg = "";
    char *zFg = 0;
    char *zMsg = 0; /* plain message */
    char *zWiki = 0;  /* wiki-formatted message */
    char *zAttach = 0;
    char zPrefix[1400];
    char zSuffix[1200];
    int nEdits, nLastEdit;
    
    thisDate = atoi(az[i]);
    pTm = localtime(&thisDate);
    thisDay = timet_to_day(thisDate);
    if( !inTable ){
      @ <table cellspacing=0 border=0 cellpadding=0>
      inTable = 1;
    }
    if( thisDay!=lastDay && divT ){
      strftime(zDate, sizeof(zDate), "%A, %Y-%b-%d", pTm);
      @ <tr><td colspan=3>
      @ <table cellpadding=2 border=0>
      @ <tr><td bgcolor="%s(BORDER1)" class="border1">
      @ <table cellpadding=2 cellspacing=0 border=0><tr>
      @   <td bgcolor="%s(BG1)" class="bkgnd1">%s(zDate)</td>
      @ </tr></table>
      @ </td></tr></table>
      @ </td></tr>
      lastDay = thisDay;
    }
    if( divM && az[i+1][0]=='1' && az[i+3][0] && az[i+3][0]=='1' ){
      @ <tr><td colspan=3>
      @ <table cellpadding=2 border=0>
      @ <tr><td bgcolor="%s(BORDER2)" class="border2">
      @ <table cellpadding=2 cellspacing=0 border=0><tr>
      if( az[i+4] && az[i+4][0] ){
        @   <td bgcolor="%s(BG2)" class="bkgnd2">%h(az[i+6])
        @       (<i>%h(az[i+4])</i>)</td>
      } else {
        @   <td bgcolor="%s(BG2)" class="bkgnd2">%h(az[i+6])</td>
      }
      @ </tr></table>
      @ </td></tr></table>
      @ </td></tr>
      if( !divT ) lastDay = thisDay-1;
    }
    if( az[i+1][0]=='1' && !showM && az[i+3][0] && az[i+3][0]!='0' ) continue;
    if( divT || thisDay==lastDay ){
      strftime(zDate, sizeof(zDate), "%H:%M", pTm);
    }else{
      strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M", pTm);
      lastDay = thisDay;
    }
    @ <tr>
    @ <td valign="top" align="right" width=%d(160-divT*100)>%s(zDate)</td>
    zPrefix[0] = 0;
    zSuffix[0] = 0;
    zAttach = 0;
    switch( atoi(az[i+1]) ){
      case 1: { /* A check-in or milestone */
        zWiki = az[i+6];
        if( az[i+3][0] && az[i+3][0]!='0' ){
          zIcon = "box";
          bprintf(zPrefix, sizeof(zPrefix), "Milestone [%.20s]: ", az[i+5]);
        }else{
          zIcon = "dot";
          if( az[i+4][0] ){
            bprintf(zPrefix, sizeof(zPrefix),
                    "Check-in [%.20s] on branch %.50s: ",
                    az[i+5], az[i+4]);
            if( showC==2 ) zBg = " bgcolor=\"" BG4 "\" class=\"bkgnd5\"";
          }else{
            bprintf(zPrefix, sizeof(zPrefix), "Check-in [%.20s]: ", az[i+5]);
          }
          bprintf(zSuffix, sizeof(zSuffix), " (By %z)", format_user(az[i+2]));
        }
        break;
      }
      case 2: {  /* A new ticket was created */
        zMsg = az[i+6];
        bprintf(zPrefix, sizeof(zPrefix), "Create ticket #%.20s: ", az[i+5]);
        bprintf(zSuffix, sizeof(zSuffix), " (By %z)", format_user(az[i+2]));
        zIcon = strcmp(az[i+3],"code")==0 ? "x" : "ptr1";
        break;
      }
      case 3: {  /* The status field of a ticket changed */
        char zType[50];
        bprintf(zType,sizeof(zType),"%.30s",az[i+4]);
        if( islower(zType[0]) ) zType[0] = toupper(zType[0]);
        bprintf(zPrefix, sizeof(zPrefix), "%.30s ticket #%.20s, was %.20s.",
             zType, az[i+5], az[i+3]);
        bprintf(zSuffix, sizeof(zSuffix), " (By %z)", format_user(az[i+2]));
        if( isActiveState(az[i+4]) ){
          zIcon = "x";
        }else if( isFixedState(az[i+4]) ){
          zIcon = "ck";
        }else{
          zIcon = "dia";
        }
        if( az[i+7] && atoi(az[i+8])==4 && strcmp(az[i],az[i+7])==0
            && strcmp(az[i+5],az[i+12])==0 ){
          i += 7;
          if( az[i+4][0]==0 ){
            appendf(zPrefix,0,sizeof(zPrefix), " Unassign from %.50s.", az[i+3]);
          }else if( az[i+3][0]==0 ){
            appendf(zPrefix,0,sizeof(zPrefix), " Assign to %.50s.", az[i+4]);
          }else{
            appendf(zPrefix,0,sizeof(zPrefix), " Reassign from %.50s to %.50s",
                    az[i+3], az[i+4]); 
          }
        }
        if( az[i+7] && atoi(az[i+8])==6 && strcmp(az[i],az[i+7])==0
            && strcmp(az[i+5],az[i+12])==0 ){
          i += 7;
          appendf(zPrefix,0,sizeof(zPrefix), " Plus other changes.");
          while( az[i+7] && atoi(az[i+8])==6 && strcmp(az[i],az[i+7])==0
                 && strcmp(az[i+5],az[i+12])==0 ){
            i += 7;
          }
        }
        break;
      }
      case 4: {  /* The assigned-to field of a ticket changed */
        if( az[i+4][0]==0 ){
          bprintf(zPrefix, sizeof(zPrefix), "Unassign ticket #%.20s from %.50s.",
             az[i+5], az[i+3]);
        }else if( az[i+3][0]==0 ){
          bprintf(zPrefix, sizeof(zPrefix), "Assign ticket #%.20s to %.50s.",
             az[i+5], az[i+4]);
        }else{
          bprintf(zPrefix, sizeof(zPrefix),
             "Reassign ticket #%.20s from %.50s to %.50s",
             az[i+5], az[i+3], az[i+4]); 
        }
        bprintf(zSuffix, sizeof(zSuffix), " (By %z)", format_user(az[i+2]));
        zIcon = "arrow";
        break;
      }
      case 5: {   /* Changes to a Wiki page */
        bprintf(zPrefix, sizeof(zPrefix), "Wiki page {wiki:%s} ", az[i+5]);
        zIcon = "star";
        /* Skip over subsequent lines of same text and display number 
        ** of edits if > 1. Only collapse items from same day.
        */
        nEdits = 1;
        while( az[i+7] && atoi(az[i+8])==5 && strcmp(az[i+5],az[i+12])==0
               && strcmp(az[i+2],az[i+9])==0 ){
          if( divT && !same_day(atoi(az[i]), atoi(az[i+7])) ){
            break;
          }
          i += 7;
          nEdits++;
        }
        if( nEdits>1 ){
          bprintf(zSuffix, sizeof(zSuffix), "edited %d times by %z", nEdits, 
                  format_user(az[i+2]));
        }else{
          bprintf(zSuffix, sizeof(zSuffix), "edited by %z", format_user(az[i+2]));
        }
        appendf(zSuffix,0,sizeof(zSuffix),
                " <a href=\"wiki?p=%T&t=%d&diff=1\">(diff)</a>",
                az[i+5], thisDate);
        break;
      }
      case 6: {  /* Changes to a ticket other than status or assignment */
        bprintf(zSuffix, sizeof(zSuffix), " (By %z)", format_user(az[i+2]));
        zIcon = "arrow";
        /* Skip over subsequent lines of same text and display number 
        ** of edits if > 1. Only collapse items from same day.
        */
        nEdits = 1;
        nLastEdit = atoi(az[i]);
        while( az[i+7] && atoi(az[i+8])==6 && strcmp(az[i+5],az[i+12])==0 
               && strcmp(az[i+2],az[i+9])==0 ){
          if( divT && !same_day(atoi(az[i]), atoi(az[i+7])) ){
            break;
          }
          if( atoi(az[i+7])!=nLastEdit ){
            nLastEdit = atoi(az[i+7]);
            nEdits++;
          }
          i += 7;
        }
        if( nEdits>1 ){
          bprintf(zPrefix, sizeof(zPrefix), "%d changes to ticket #%.20s",
                  nEdits, az[i+5]);
        }else{
          bprintf(zPrefix, sizeof(zPrefix), "Changes to ticket #%.20s", az[i+5]);
        }
        break;
      }
      case 7: { /* Attachments */
        if( isdigit(az[i+3][0]) ){
          bprintf(zPrefix, sizeof(zPrefix), "Attachment to ticket #%.20s: ",
                  az[i+3]);
        }else{
          bprintf(zPrefix, sizeof(zPrefix), "Attachment to {wiki:%.100s}: ",
                  az[i+3]);
        }
        zAttach = mprintf(
            "%h bytes <a href=\"attach_get/%T/%T\">%h</a>",
            az[i+4], az[i+6], az[i+5], az[i+5]);
        bprintf(zSuffix, sizeof(zSuffix), "(By %z)", format_user(az[i+2]));
        zIcon = "arrow";
        break;
      }
      case 8: { /* An inspection */
        zMsg = az[i+6];
        zIcon = "dot";
        if( az[i+4][0] ){
          bprintf(zPrefix, sizeof(zPrefix),
                  "Inspection of [%.20s] on branch %.50s: ",
                  az[i+5], az[i+4]);
          if( showC==2 ) zBg = " bgcolor=\"" BG4 "\" class=\"bkgnd5\"";
        }else{
          bprintf(zPrefix, sizeof(zPrefix), "Inspection of [%.20s]: ", az[i+5]);
        }
        bprintf(zSuffix, sizeof(zSuffix), " (By %z)", format_user(az[i+2]));
        break;
      }
      default:
        /* Cannot happen */
        break;
    }
    @ <td valign="top" align="center" width=30>
    if( zIcon ){
      common_icon(zIcon);
    }
    @ </td>
    @ <td valign="top"%s(zBg)>
    if( zFg ){
      @ <font color="%s(zFg)">
    }
    assert( strlen(zPrefix)<sizeof(zPrefix)-1000 );
    output_formatted(zPrefix, 0);
    if( zWiki ){
      if( output_trim_message(zWiki, MN_CKIN_MSG, MX_CKIN_MSG) ){
        output_formatted(zWiki, 0);
        @ &nbsp;[...]
      }else{
        output_formatted(zWiki, 0);
      }
    }else if(zMsg){
      @ %h(zMsg)
    }
    if( zAttach ){
      @ %s(zAttach)
      free(zAttach);
      zAttach = 0;
    }
    cgi_printf("%s", zSuffix);  /* suffix is always user info */
    if( zFg ){
      @ </font>
    }
    @ </td></tr>
  }
  if( inTable ){
    @ </table>
    inTable = 0;
  }
  @ <hr>
  @ <form method="GET" action="timeline">
  @ <table cellpadding=3>
  @ <tr><td colspan=2>
  @ Show a timeline of 
  cgi_text("d", 0, 0, 0, 0, 5, 8, 0, mprintf("%d",days), 0);
  @ days going backwards from
  pTm = localtime(&end);
  strftime(zDate, sizeof(zDate), "%Y-%b-%d", pTm);
  cgi_text("e", 0, 0, 0, 0, 14, 50, 0, mprintf("%s",zDate), 0);
  @ </td></tr>
  @ <tr>
  zCkinPrfx = P("px");
  if( g.okCheckout ){
    @ <td align="left" valign="top">
    @ <fieldset><legend>Check-in options</legend>
    cgi_radio_fieldset(0, "c", 0, 0, mprintf("%d",showC),
      "2", 0, "Show all check-ins<br>",
      "3", 0, "Show only trunk check-ins<br>",
      "1", 0, "Show only branch check-ins<br>",
      "0", 0, "Show no check-ins<br>",
      NULL
    );
    @ Only show check-ins of files with this prefix:
    cgi_text("px", 0, 0, 0, 0, 25, 0, 0,
      mprintf("%s",zCkinPrfx?zCkinPrfx:""), 0);
    @ </fieldset>
    @ </td>
  }else{
    cgi_hidden("c", 0, mprintf("%d",showC));
    cgi_hidden("px", 0, mprintf("%s",zCkinPrfx?zCkinPrfx:""));
  }
  if( g.okRead ){
    @ <td valign="top">
    cgi_radio_fieldset("Ticket options", "s", 0, 0, mprintf("%d",showS),
      "3", 'A', "Show all ticket changes of any kind<br>",
      "2", 'S', "Show all ticket status changes<br>",
      "1", 'N', "Show only &quot;active&quot; and &quot;new&quot; "
                "status changes<br>",
      "0", 'U', "Show no ticket status changes<br>",
      "9", 'H', "Show no tickets at all<br>",
      NULL
    );
    @ </td>
  }else{
    cgi_hidden("s", 0, mprintf("%d",showS));
  }
  @ </tr>
  @ <tr><td valign="top">
  cgi_checkbox_fieldset("Timeline options", 0, 0,
    "dm", "1", 0, mprintf("%d",divM), "Divide timeline by milestones<br>",
    "dt", "1", 0, mprintf("%d",divT), "Divide timeline by days<br>",
    "debug1", "1", 0, mprintf("%s",P("debug1")),
      "Show the SQL used to generate the timeline",
    NULL
  );
  @ <br>
  cgi_hidden("x", 0, "1");
  cgi_submit(0, 0, 0, 0, 0, "Show Timeline");
  if( g.okAdmin ){
    @ &nbsp;&nbsp;&nbsp;
    cgi_submit("set", 0, 0, 0, 0, "Make Default");
  }
  @ </td><td valign="top">
  @ <fieldset><legend>Other options</legend>
  if( g.okRead ){
    cgi_checkbox("a", 0, 0, 0, 0, 0, "1", mprintf("%d",showA),
      "Show assignment changes<br>");
  }else{
    cgi_hidden("a", 0, mprintf("%d",showA));
  }
  if( g.okCheckout ){
    cgi_checkbox("m", 0, 0, 0, 0, 0, "1", mprintf("%d",showM),
      "Show milestones<br>");
  }else{
    cgi_hidden("m", 0, mprintf("%d",showM));
  }
  if( g.okRdWiki ){
    cgi_checkbox("w", 0, 0, 0, 0, 0, "1", mprintf("%d",showW),
      "Show Wiki edits<br>");
  }else{
    cgi_hidden("w", 0, mprintf("%d",showW));
  }
  if( g.okRdWiki || g.okRead ){
    cgi_checkbox("t", 0, 0, 0, 0, 0, "1", mprintf("%d",showT),
      "Show attachments<br>");
  }else{
    cgi_hidden("t", 0, mprintf("%d",showT));
  }
  @ </fieldset>
  @ </td></tr>
  @ </table>
  @ </form>
  common_footer();
}

/*
** If the string is NULL or contains an single-quote of backslash
** return a pointer to an empty string.  If no unauthorized
** characters are found in the string, return the string itself.
**
** This routine is used to make sure that an argument can be safely
** quoted into a command to be executed by popen().
*/
const char *quotable_string(const char *z){
  int c, i;
  if( z==0 ){
    return "";
  }
  for(i=0; (c=z[i])!=0; i++){
    if( c=='\'' || c=='\\' ){
      return "";
    }
  }
  return z;
}

/*
** Make a file revision number printable. Mostly needed for things like
** git where object identifiers are insanely long.
*/
char *printable_vers(const char *zVers){
  if( zVers ){
    int nLen = strlen(zVers);
    if( nLen<=10 ){
      return strdup(zVers);
    }
    return mprintf("%4.4s..%4.4s",zVers,&zVers[nLen-4]);
  }else{
    return strdup("");
  }
}

/*
** Adds all appropriate action bar links for check-in/milestone tools
*/
static void add_chng_tools(
  const char *zExcept,
  int cn,
  int ms
){
  int i;
  char *zLink;
  char **azTools;
  db_add_functions();
  azTools = db_query("SELECT tool.name FROM tool,user "
                     "WHERE tool.object='%s' AND user.id='%q' "
                     "      AND cap_and(tool.perms,user.capabilities)!=''",
                     ms?"ms":"chng", g.zUser);

  for(i=0; azTools[i]; i++){
    if( zExcept && 0==strcmp(zExcept,azTools[i]) ) continue;

    zLink = mprintf("chngtool?t=%T&cn=%d", azTools[i], cn);
    common_add_action_item(zLink, azTools[i]);
  }
}

/*
** WEBPAGE: /chngtool
**
** Execute an external tool on a given check-in/milestone
*/
void chngtool(void){
  int cn = atoi(PD("cn","0"));
  const char *zTool = P("t");
  char *zAction;
  const char *azSubst[32];
  int n = 0;
  char **az;

  if( cn==0 || zTool==0 ) cgi_redirect("index");

  login_check_credentials();
  if( !g.okRead ){ login_needed(); return; }
  throttle(1,0);
  history_update(0);

  az = db_query("SELECT date, branch, milestone, user, message, directory "
                "FROM chng WHERE cn=%d", cn);
  if( az==0 || az[0]==0 ){
    cgi_redirect(mprintf("chngview?cn=%d",cn));
  }

  zAction = db_short_query("SELECT command FROM tool "
                           "WHERE name='%q' AND object='%s'",
                           zTool, atoi(az[2])?"ms":"chng");
  if( zAction==0 || zAction[0]==0 ){
    cgi_redirect(mprintf("chngview?cn=%d",cn));
  }

  common_standard_menu(0, "search?c=1");
  common_add_action_item(mprintf("chngview?cn=%d", cn), "Back");
  add_chng_tools(zTool,cn,atoi(az[2]));

  common_header("%h: %d", zTool, cn);

  if( atoi(az[2]) ){
    azSubst[n++] = "MS";
  }else{
    azSubst[n++] = "CN";
  }
  azSubst[n++] = mprintf("%d",cn);
  azSubst[n++] = 0;

  n = execute_tool(zTool,zAction,az[4],azSubst);
  free(zAction);
  if( n<=0 ){
    cgi_redirect(mprintf("chngview?cn=%d", cn));
  }
  common_footer();
}

/*
** Generate a page that shows specifics of a particular checkin.
**
** WEBPAGE: /chngview
*/
void checkin_view(void){
  char **az, **azFile, **azTkt, **azInspect;
  int cn;
  time_t tx;
  struct tm *pTm;
  int i;
  int cnt;
  char zDate[200];
  char zDateUTC[200];
  int nMs = 0;

  login_check_credentials();
  if( !g.okRead ){ login_needed(); return; }
  throttle(1,0);
  cn = atoi(PD("cn","0"));
  az = db_query("SELECT date, branch, milestone, user, message, directory "
                "FROM chng WHERE cn=%d", cn);
  if( az[0]==0 ){ cgi_redirect("index"); return; }
  azFile = db_query("SELECT filename, vers, nins, ndel, chngtype, prevvers "
                    "FROM filechng WHERE cn=%d ORDER BY +filename", cn);
  azTkt = db_query(
     "SELECT ticket.tn, ticket.title FROM xref, ticket "
     "WHERE xref.cn=%d AND xref.tn=ticket.tn "
     "ORDER BY ticket.tn", cn);
  azInspect = db_query(
     "SELECT inspecttime, inspector, result FROM inspect "
     "WHERE cn=%d ORDER BY inspecttime", cn);
  common_standard_menu("chngview", "search?c=1&f=1");

  nMs = atoi(az[2]);
  if( nMs==0 && g.okWrite && g.okCheckin ){
    common_add_action_item(mprintf("inspect?cn=%d",cn), "Inspection");
  }
  
  /* Show Patchset link only for check-ins.*/
  if( nMs==0 && g.okCheckout ){
    common_add_action_item(mprintf("patchset?cn=%d",cn), "Patchset");
  }
  if( g.okWrite && g.okCheckin ){
    common_add_action_item(mprintf("chngedit?cn=%d",cn), "Edit");
  }
  if( !strcmp(g.scm.zSCM,"cvs") && g.okRead ){
    common_add_action_item(mprintf("taghints?cn=%d",cn), "Tagging/Branching");
  }
  add_chng_tools(0,cn,nMs);
  if( nMs==0 ){
    common_add_help_item("CvstracCheckin");
    common_header("Check-in [%d]", cn);
  }else{
    common_add_help_item("CvstracMilestone");
    common_header("Milestone [%d]", cn);
  }

  tx = (time_t)atoi(az[0]);
  pTm = localtime(&tx);
  strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M:%S", pTm);
  pTm = gmtime(&tx);
  strftime(zDateUTC, sizeof(zDateUTC), "%Y-%b-%d %H:%M:%S", pTm);

  @ <table cellpadding=1 cellspacing=0 width="100%%">
  @ <tr><td align="right" valign="top">
  if( nMs==0 ){
    @ Check-in Number:
  }else{
    @ Milestone Number:
  }
  @ </td>
  @   <td>%d(cn)</td><td width="30%%"></td></tr>
  @ <tr><td align="right" valign="top">Date:</td>
  @   <td>%s(zDate) (local)
  @   <br>%s(zDateUTC) (UTC)</td></tr>
  @ <tr><td align="right">User:</td><td>%z(format_user(az[3]))</td></tr>
  @ <tr><td align="right">Branch:</td><td>%h(az[1])</td></tr>
  if( nMs && az[5] && az[5][0] ){
    @ <tr><td align="right">Directory:</td>
    @   <td><a href="%h(default_browse_url())?d=%h(az[5])">%h(az[5])</a></td>
    @ </tr>
  }
  if( nMs ){
    @ <tr><td align="right">Type:</td>
    @ <td>%h((nMs==1)?"Release":"Event")</td></tr>
  }
  @ <tr><td align="right" valign="top">Comment:</td><td colspan=2>
  output_formatted(az[4], 0);
  if( g.okWrite && g.okCheckin ){
    @
    @ <a href="chngedit?cn=%d(cn)">(edit)</a>
  }
  @ </td></tr>
  @ <tr><td align="right" valign="top">Tickets:</td><td colspan=2>
  if( azTkt[0]!=0 ){
    @ <table cellpadding=0 cellspacing=0 border=0>
    for(i=0; azTkt[i]; i+=2 ){
      char *z = mprintf("#%s",azTkt[i]);
      @ <tr><td align="right" valign="top">
      output_formatted(z,0);
      @ </td><td width=8></td>
      @ <td>%h(azTkt[i+1])</td></tr>
    }
    @ </table></td></tr>
  }
  @ <tr><td align="right" valign="top">Inspections:</td><td colspan=2>
  for(i=0; azInspect[i]; i+=3){
    tx = (time_t)atoi(azInspect[i]);
    pTm = gmtime(&tx);
    strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M:%S (UTC)", pTm);
    @ %s(zDate) by %h(azInspect[i+1]): %h(azInspect[i+2])<br>
  }
  @ </td></tr>
  @ <tr><td align="right" valign="top">Files:</td><td colspan=2>
  @ <table cellpadding=0 cellspacing=0 border=0>
  for(cnt=i=0; azFile[i]; i+=6, cnt++){
    int chngtype = atoi(azFile[i+4]);
    const char* zVers = azFile[i+1];
    const char* zPrev = azFile[i+5];
    char *zCurP = printable_vers(zVers);
    char *zPrevP = printable_vers(zPrev);
    const char* zF = azFile[i];

    if( g.okCheckout ){
      @ <tr><td><a href="rlog?f=%T(azFile[i])">%h(azFile[i])</a>
    }else{
      @ <tr><td>%h(azFile[i])
    }
    @ &nbsp;&nbsp;&nbsp;&nbsp;</td>
    if( chngtype==1 ){
      @ <td>
      @   added-&gt;<a href="fileview?f=%T(zF)&v=%T(zVers)">
      @   %h(zCurP)</a>
      @ </td>
    }else if( chngtype==2 ){
      @ <td>
      @   <a href="fileview?f=%T(zF)&v=%T(zPrev)">
      @   %h(zPrevP)</a>-&gt;removed
      @ </td>
    }else{
      @ <td>
      @   <a href="fileview?f=%T(zF)&v=%T(zPrev)">%h(zPrevP)</a>
      @   <a href="filediff?f=%T(zF)&v1=%T(zPrev)&v2=%T(zVers)">-&gt;</a>
      @   <a href="fileview?f=%T(zF)&v=%T(zVers)">%h(zCurP)</a>
      @ </td>
      if( azFile[i+2][0] && azFile[i+3][0] ){
        @ <td>&nbsp;&nbsp;&nbsp
        if( atoi(azFile[i+2]) || atoi(azFile[i+3]) ){
          @ %h(azFile[i+2]) inserted, %h(azFile[i+3]) deleted
        }
        @ </td>
      }
    }
    free(zCurP);
    free(zPrevP);
    @ </tr>
  }
  @ </table>
  @ </td></tr></table>
  if( cnt>0 && g.okCheckout ){
    diff_chng(cn,0);
  }
  common_footer();
}

/*
** Generate a full diff for a given changeset.
**
** WEBPAGE: /patchset
*/
void patchset_view(void){
  int cn = atoi(PD("cn","0"));
  char *z;

  login_check_credentials();
  if( !g.okCheckout ){ login_needed(); return; }
  throttle(1,0);

  cgi_set_content_type("text/plain");

  z = db_short_query("SELECT date FROM chng WHERE cn=%d",cn);
  if( z ) cgi_modified_since(atoi(z));

  cgi_append_header(mprintf("Content-disposition: attachment; "
        "filename=\"%T_patchset_%d.diff\"\r\n", g.zName, cn));

  /* patchsets are raw output based one what are supposed to be unchanging file
  ** revisions.
  */
  g.isConst = 1;

  if( cn>0 ) diff_chng(cn,1);
}

/*
** WEBPAGE: /chngedit
*/
void change_edit(void){
  char *zCancel;
  int cn;
  const char *zMsg, *zOldMsg;
  char **az;

  login_check_credentials();
  if( !g.okWrite || !g.okCheckin ){
    login_needed();
    return;
  }
  throttle(1,1);
  cn = atoi(PD("cn","0"));
  if( cn<=0 ){
    cgi_redirect("index");
    return;
  }
  zCancel = mprintf("chngview?cn=%d", cn);
  if( P("can") ){
    cgi_redirect(zCancel);
    return;
  }
  zMsg = P("m");
  if( zMsg ){
    /* Update message in CHNG table and update XREF table in case some 
    ** tickets were removed or added in new message. 
    ** Preserve manually added associations by pre-deleteing only 
    ** those XREF rows that are present in old message.
    */
    db_execute("BEGIN");
    zOldMsg = db_short_query("SELECT message FROM chng WHERE cn=%d", cn);
    db_execute("UPDATE chng SET message='%q' WHERE cn=%d", zMsg, cn);
    xref_remove_checkin_comment(cn, zOldMsg);
    xref_add_checkin_comment(cn, zMsg);
    db_execute("COMMIT");
    cgi_redirect(zCancel);
    return;
  }
  az = db_query("SELECT message, milestone FROM chng WHERE cn=%d", cn);
  zMsg = az[0];
  if( zMsg==0 ) zMsg = "";
  if( az[1][0] && atoi(az[1]) ){
    milestone_edit();
    return;
  }
  common_add_action_item(zCancel, "Cancel");
  common_add_help_item("CvstracCheckin");
  common_header("Edit Check-in [%d]", cn);
  @ <form action="chngedit" method="POST">
  @ <input type="hidden" name="cn" value="%d(cn)">
  @ Edit the change message and press "OK":<br>
  @ <textarea name="m" rows="8" cols="80" wrap="virtual">
  @ %h(zMsg)
  @ </textarea>
  @ <blockquote>
  @ <input type="submit" name="ok" value="OK">
  @ <input type="submit" name="can" value="Cancel">
  @ </blockquote>
  @ </form>
  common_footer();
}

/*
** WEBPAGE: /inspect
*/
void add_inspection(void){
  char *zCancel;
  int cn;
  const char *zResult;

  login_check_credentials();
  if( !g.okWrite || !g.okCheckin ){
    login_needed();
    return;
  }
  throttle(1,1);
  cn = atoi(PD("cn","0"));
  if( cn<=0 ){
    cgi_redirect("index");
    return;
  }
  zCancel = mprintf("chngview?cn=%d", cn);
  if( P("can") ){
    cgi_redirect(zCancel);
    return;
  }
  zResult = P("r");
  if( zResult && P("ok") ){
    time_t now;
    time(&now);
    db_execute("INSERT INTO inspect(cn,inspecttime,inspector,result) "
       "VALUES(%d,%d,'%q','%q')",
       cn, now, g.zUser, zResult);
    cgi_redirect(zCancel);
    return;
  }
  common_add_help_item("CvstracInspection");
  common_header("Inspection Report");
  @ <form action="inspect" method="POST">
  @ <input type="hidden" name="cn" value="%d(cn)">
  @ Inspection results:
  @ <input type="text" name="r" size="40">
  @ <p><input type="submit" name="ok" value="OK">
  @ <input type="submit" name="can" value="Cancel">
  @ </form>
  common_footer();
}


/*
** Generate a page that shows how to create a new tag or branch
**
** WEBPAGE: /taghints
*/
void tag_hints(void){
  char **az;
  int cn;
  time_t tx;
  struct tm *pTm;
  char zDateUTC[200];

  login_check_credentials();
  if( !g.okRead ){ login_needed(); return; }
  throttle(1,0);
  cn = atoi(PD("cn","0"));
  az = db_query("SELECT date, branch, milestone, user, message "
                "FROM chng WHERE cn=%d", cn);
  if( az[0]==0 ){ cgi_redirect("index"); return; }
  common_add_action_item( mprintf("chngview?cn=%d",cn), "Back");
  add_chng_tools(0,cn,atoi(az[2]));
  common_header("Tagging And Branching Hints");
  tx = (time_t)atoi(az[0])-1;
  pTm = gmtime(&tx);
  strftime(zDateUTC, sizeof(zDateUTC), "%Y-%m-%d %H:%M:%S UTC", pTm);
  @ <p>To create a tag that occurs <i>before</i> 
  @ check-in <a href="chngview?cn=%d(cn)">[%d(cn)]</a>, go to the root of
  @ the source tree and enter the following CVS command:</p>
  @ <blockquote>
  @ <tt>cvs rtag -D '%s(zDateUTC)' </tt><i>&lt;tag-name&gt;</i><tt> .</tt>
  @ </blockquote>
  @
  @ <p>Be careful to include the dot (".") at the end of the line!
  @ To create a branch that occurs before the check-in, enter this 
  @ command:</p>
  @ <blockquote>
  @ <tt>cvs rtag -b -D '%s(zDateUTC)' </tt><i>&lt;branch-name&gt;</i><tt> .</tt>
  @ </blockquote>
  common_footer();
}

/*
** A webpage to create a new milestone
**
** WEBPAGE: /msnew
** WEBPAGE: /msedit
*/
void milestone_edit(void){
  const char *zTime;
  const char *zMsg, *zOldMsg;
  time_t tm;
  int cn;
  int mtype;
  const char *zMType;
  const char *zBr;
  char **az;
  char **azAllBr;
  struct tm *pTm;
  char zDate[200];

  login_check_credentials();
  if( !g.okWrite || !g.okCheckin ){ login_needed(); return; }
  throttle(1,1);
  zTime = P("t");
  if( zTime==0 || (tm = parse_time(zTime))==0 ){
    zTime = "";
  }
  cn = atoi(PD("cn","0"));
  zMsg = remove_blank_lines(PD("m",""));
  zMType = PD("y","0");
  zBr = PD("br","");
  mtype = atoi(zMType);

  if( cn>0 && P("del1") ){
    common_add_action_item(mprintf("msedit?cn=%d",cn), "Cancel");
    common_header("Are You Sure?");
    @ <form action="msedit" method="POST">
    @ <p>You are about to delete all traces of milestone [%d(cn)] from
    @ the database.  This is an irreversible operation.</p>
    @
    @ <input type="hidden" name="cn" value="%d(cn)">
    @ <input type="submit" name="del2" value="Delete The Milestone">
    @ <input type="submit" name="can" value="Cancel">
    @ </form>
    common_footer();
    return;
  }else if( cn>0 && P("del2") ){
    db_execute("BEGIN");
    db_execute("DELETE FROM chng WHERE cn=%d AND milestone", cn);
    db_execute("DELETE FROM xref WHERE cn=%d", cn);
    db_execute("COMMIT");
    cgi_redirect("index");
  }

  if( tm>0 && zTime[0] && zMsg[0] && mtype>0 ){
    if( cn>0 ){
      /*
      ** Preserve manually added associations by pre-deleteing only 
      ** those XREF rows that are present in old message.
      */
      db_execute("BEGIN");
      zOldMsg = db_short_query("SELECT message FROM chng WHERE cn=%d", cn);
      db_execute("UPDATE chng SET date=%d, user='%q', message='%q',"
                 "milestone=%d, branch='%q' "
                 "WHERE cn=%d",
         tm, g.zUser, zMsg, mtype, zBr, cn);
      xref_remove_checkin_comment(cn, zOldMsg);
      xref_add_checkin_comment(cn, zMsg);
      db_execute("COMMIT");
    }else{
      db_execute("BEGIN");
      db_execute("INSERT INTO chng(date,milestone,user,message,branch) "
         "VALUES(%d,%d,'%q','%q','%q')",
         tm, mtype, g.zUser, zMsg, zBr);
      zBr = db_short_query("SELECT max(cn) FROM chng");
      cn = zBr ? atoi(zBr) : 0;
      if( cn>0 ){
        xref_add_checkin_comment(cn, zMsg);
      }
      db_execute("COMMIT");
    }
    cgi_redirect(mprintf("chngview?cn=%d", cn));
    return;
  }
  az = cn<=0 ? 0 :
    db_query("SELECT date, milestone, message, branch FROM chng "
             "WHERE cn=%d AND milestone", cn);
  if( az && az[0] ){
    tm = atoi(az[0]);
    zMType = az[1];
    mtype = atoi(zMType);
    zMsg = az[2];
    zBr = az[3];
  }else{
    time(&tm);
    zMsg = "";
    cn = 0;
    zMType = "1";
    mtype = 1;
    zBr = "";
  }
  azAllBr = db_query("SELECT DISTINCT branch FROM chng WHERE branch NOT NULL "
                     "UNION SELECT '' ORDER BY 1");

  common_add_help_item("CvstracMilestone");
  if( cn>0 ){
    common_standard_menu("msedit", 0);
    common_add_action_item(mprintf("chngview?cn=%d",cn), "Cancel");
    common_add_action_item(mprintf("msedit?cn=%d&del1=1",cn), "Delete");
    common_header("Edit Milestone");
  }else{
    common_standard_menu("msnew", 0);
    common_add_action_item("index", "Cancel");
    common_header("Create New Milestone");
  }
  @ <form action="msedit" method="POST">
  if( cn>0 ){
    @ <input type="hidden" name="cn" value="%d(cn)">
  }
  pTm = localtime(&tm);
  strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M:%S", pTm);
  @ <table>
  @ <tr><td align="right">Date &amp; Time:</td>
  @    <td><input type="text" name="t" value="%s(zDate)" size=26></td>
  @ <td width=20>&nbsp;</td>
  @ <td>Type: 
  cgi_optionmenu(2,"y", zMType, "Release", "1", "Event", "2", NULL);
  @ </td></tr>
  @ </td></tr>
  @ <tr><td align="right">Branch:</td><td>
  cgi_v_optionmenu(2, "br", zBr, (const char**)azAllBr);
  @ </td></tr>
  @ <tr><td align="right" valign="top">Comment:</td>
  @   <td colspan=3>
  @     <textarea name="m" cols="50" rows="10">%h(zMsg)</textarea></td></tr>
  @ <tr><td colspan=4 align="center">
  @   <input type="submit" value="Submit">
  if( cn>0 ){
    @   &nbsp;&nbsp;&nbsp;
    @   <input type="submit" value="Delete" name="del1">
  }
  @ </td></tr></table>
  @ </form>
  common_footer();
}

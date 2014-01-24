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
  int showC;       /* Show all checkins if 2, branch checkis if 1, none if 0 */
  int showS;       /* 0: no status changes 1: active & new  2: everything */
  int showA;       /* Show assignment changes if 1 */
  int showW;       /* Show changes to Wiki pages if 1 */
  int showT;       /* Show attachment additions */
  int divM;        /* Divide timeline by milestones if 1 */
  int divT;        /* Divide timeline by days if 1 */
  const char *zCkinPrfx;   /* Only show checkins of files with this prefix */
  char zDate[200];
  char zSQL[3000];

  login_check_credentials();
  if( !g.okRead && !g.okCheckout ){ login_needed(); return; }
  throttle(1);
  history_update(0);
  if( P("x") ){
    showM = PD("m","0")[0] - '0';
    showC = PD("c","0")[0] - '0';
    showS = PD("s","0")[0] - '0';
    showA = PD("a","0")[0] - '0';
    showW = PD("w","0")[0] - '0';
    showT = PD("t","0")[0] - '0';
    divM = PD("dm","0")[0] - '0';
    divT = PD("dt","0")[0] - '0';
    if( P("set") && P("d") && g.okWrite ){
      char zVal[12];
      zVal[0] = showA + '0';
      zVal[1] = showC + '0';
      zVal[2] = showM + '0';
      zVal[3] = showS + '0';
      zVal[4] = showW + showT*2 + '0';
      zVal[5] = divM + divT*2 + '0';
      zVal[6] = 0;
      db_execute("REPLACE INTO config(name,value) "
                 "VALUES('timeline_format','%s%s')", zVal, P("d"));
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
    zCkinPrfx = sqlite_mprintf("%q", zCkinPrfx);
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
    sprintf(zSQL,
       "SELECT "
       "  date AS 'time', 1 AS 'type', user, milestone, branch, cn, message "
       "FROM chng WHERE date<=%d AND date>=%d",
       (int)end, (int)begin);
    len = strlen(zSQL);
    if( showM==0 && divM==0 ){
      sprintf(&zSQL[len]," AND NOT milestone");
      len += strlen(&zSQL[len]);
    }
    if( showC==0 || zCkinPrfx  ){
      sprintf(&zSQL[len]," AND milestone%s", showM ? "" : "==1");
      len += strlen(&zSQL[len]);
    }else if( showC==1 ){
      sprintf(&zSQL[len]," AND (milestone%s OR branch!='')", showM ? "":"==1");
      len += strlen(&zSQL[len]);
    }
  }
  if( zCkinPrfx ){
    if( len>0 ){
      sprintf(&zSQL[len]," UNION ALL ");
      len += strlen(&zSQL[len]);
    }
    sprintf(&zSQL[len],
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
    len += strlen(&zSQL[len]);
  }
  if( showS || showA ){
    if( len>0 ){
      sprintf(&zSQL[len]," UNION ALL ");
      len += strlen(&zSQL[len]);
    }
    sprintf(&zSQL[len],
       "SELECT origtime AS 'time', 2 AS 'type', owner, type, NULL, tn, title "
       "FROM ticket WHERE origtime<=%d AND origtime>=%d",
       (int)end, (int)begin);
    len += strlen(&zSQL[len]);
    if( showS ){
      sprintf(&zSQL[len],
         " UNION ALL "
         "SELECT chngtime AS 'time', 3 AS 'type', user, oldval AS 'aux1', "
         "       newval AS 'aux2', tn AS 'ref', NULL as 'text' "
         "FROM tktchng "
         "WHERE fieldid='status' AND chngtime<=%d AND chngtime>=%d ",
         (int)end, (int)begin);
      len += strlen(&zSQL[len]);
      if( showS==1 ){
        sprintf(&zSQL[len],
          " AND ("
          "(newval IN ('new','active') AND oldval NOT IN ('new','active')) OR"
          "(newval NOT IN ('new','active') AND oldval IN ('new','active')))");
        len += strlen(&zSQL[len]);
      }
    }
    if( showS==3 ){
      sprintf(&zSQL[len],
         " UNION ALL "
         "SELECT chngtime AS 'time', 6 AS 'type', user, '' AS 'aux1', "
         "       '' AS 'aux2', tn AS 'ref', NULL as 'text' "
         "FROM tktchng "
         "WHERE fieldid!='status' AND fieldid!='assignedto'"
         "  AND chngtime<=%d AND chngtime>=%d ",
         (int)end, (int)begin);
      len += strlen(&zSQL[len]);
    }
    if( showA ){
      sprintf(&zSQL[len],
         " UNION ALL "
         "SELECT chngtime AS 'time', 4 AS 'type', user, oldval AS 'aux1', "
         "       newval AS 'aux2', tn AS 'ref', NULL as 'text' "
         "FROM tktchng "
         "WHERE fieldid='assignedto' AND chngtime<=%d AND chngtime>=%d",
         (int)end, (int)begin);
      len += strlen(&zSQL[len]);
    }
  }
  if( showW ){
    if( len>0 ){
      sprintf(&zSQL[len]," UNION ALL ");
      len += strlen(&zSQL[len]);
    }
    sprintf(&zSQL[len],
       "SELECT -invtime AS 'time', 5 AS 'type', who, NULL, NULL, "
       "       name as 'ref', NULL as 'text' "
       "FROM wiki "
       "WHERE invtime>=%d AND invtime<=%d",
       -(int)end, -(int)begin);
    len += strlen(&zSQL[len]);
  }
  if( showT ){
    if( len>0 ){
      sprintf(&zSQL[len]," UNION ALL ");
      len += strlen(&zSQL[len]);
    }
    sprintf(&zSQL[len],
       "SELECT date AS 'time', 7 AS 'type', user, tn, size, "
       "       fname as 'ref', atn as 'text' "
       "FROM attachment "
       "WHERE date>=%d AND date<=%d",
       (int)begin, (int)end);
    len += strlen(&zSQL[len]);
  }

  if( len==0 ){
    static char *azDummy[] = { 0 };
    az = azDummy;
  }else{
    sprintf(&zSQL[len], " ORDER BY 1 DESC, 2");
    az = db_query("%s",zSQL);
  }
  lastDay = 0;
  common_standard_menu("timeline", "search?t=1&c=1");
  common_header("Timeline");
  if( P("debug1") ){
    @ <p>%h(zSQL)</p><hr>
  }
  for(i=0; az[i]; i+=7){
    char *zIcon = 0;
    int iconWidth = 0;
    char *zBg = "";
    char *zFg = 0;
    char *zMsg;
    char zPrefix[400];
    char zSuffix[200];
    char zAttach[500];
    char zMsgBuf[5];

    zMsgBuf[0] = ' ';
    zMsgBuf[1] = 0;
    zMsg = zMsgBuf;
    thisDate = atoi(az[i]);
    pTm = localtime(&thisDate);
    thisDay = (pTm->tm_year+1900)*1000 + pTm->tm_yday;
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
    zAttach[0] = 0;
    switch( atoi(az[i+1]) ){
      case 1: { /* A check-in or milestone */
        zMsg = az[i+6];
        iconWidth = 9;
        if( az[i+3][0] && az[i+3][0]!='0' ){
          zIcon = "box.gif";
          sprintf(zPrefix, "Milestone [%.20s]: ", az[i+5]);
          /* zBg = " bgcolor=\"#e3ffe3\""; */
          /* zFg = "#009100"; */
        }else{
          zIcon = "dot.gif";
          if( az[i+4][0] ){
            sprintf(zPrefix, "Check-in [%.20s] on branch %.50s: ",
                    az[i+5], az[i+4]);
            if( showC==2 ) zBg = " bgcolor=\"#dddddd\"";
            /* zFg = "#909090"; */
          }else{
            sprintf(zPrefix, "Check-in [%.20s]: ", az[i+5]);
          }
          sprintf(zSuffix, " (By %.30s)", az[i+2]);
        }
        break;
      }
      case 2: {  /* A new ticket was created */
        zMsg = az[i+6];
        sprintf(zPrefix, "Create ticket #%.20s: ", az[i+5]);
        sprintf(zSuffix, " (By %.30s)", az[i+2]);
        zIcon = strcmp(az[i+3],"code")==0 ? "x.gif" : "ptr1.gif";
        iconWidth = 9;
        break;
      }
      case 3: {  /* The status field of a ticket changed */
        char zType[50];
        sprintf(zType,"%.30s",az[i+4]);
        if( islower(zType[0]) ) zType[0] = toupper(zType[0]);
        sprintf(zPrefix, "%.30s ticket #%.20s, was %.20s.",
             zType, az[i+5], az[i+3]);
        sprintf(zSuffix, " (By %.30s)", az[i+2]);
        if( isActiveState(az[i+4]) ){
          zIcon = "x.gif";
        }else if( isFixedState(az[i+4]) ){
          zIcon = "ck.gif";
        }else{
          zIcon = "dia.gif";
        }
        iconWidth = 9;
        if( az[i+7] && atoi(az[i+8])==4 && strcmp(az[i],az[i+7])==0
            && strcmp(az[i+5],az[i+12])==0 ){
          char *zTail = &zPrefix[strlen(zPrefix)];
          i += 7;
          if( az[i+4][0]==0 ){
            sprintf(zTail, " Unassign from %.50s.", az[i+3]);
          }else if( az[i+3][0]==0 ){
            sprintf(zTail, " Assign to %.50s.", az[i+4]);
          }else{
            sprintf(zTail, " Reassign from %.50s to %.50s", az[i+3], az[i+4]); 
          }
        }
        if( az[i+7] && atoi(az[i+8])==6 && strcmp(az[i],az[i+7])==0
            && strcmp(az[i+5],az[i+12])==0 ){
          char *zTail = &zPrefix[strlen(zPrefix)];
          i += 7;
          sprintf(zTail, " Plus other changes.");
          while( az[i+7] && atoi(az[i+8])==6 && strcmp(az[i],az[i+7])==0
                 && strcmp(az[i+5],az[i+12])==0 ){
            i += 7;
          }
        }
        break;
      }
      case 4: {  /* The assigned-to field of a ticket changed */
        if( az[i+4][0]==0 ){
          sprintf(zPrefix, "Unassign ticket #%.20s from %.50s.",
             az[i+5], az[i+3]);
        }else if( az[i+3][0]==0 ){
          sprintf(zPrefix, "Assign ticket #%.20s to %.50s.",
             az[i+5], az[i+4]);
        }else{
          sprintf(zPrefix, "Reassign ticket #%.20s from %.50s to %.50s",
             az[i+5], az[i+3], az[i+4]); 
        }
        sprintf(zSuffix, " (By %.30s)", az[i+2]);
        zIcon = "arrow.gif";
        iconWidth = 9;
        break;
      }
      case 5: {   /* Changes to a Wiki page */
        sprintf(zPrefix, "Wiki page %s ", az[i+5]);
        sprintf(zSuffix, "edited by %.30s", az[i+2]);
        zIcon = "star.gif";
        iconWidth = 9;
        /* Skip over subsequent lines of the same text */
        while( az[i+7] && atoi(az[i+8])==5 && strcmp(az[i+5],az[i+12])==0
               && strcmp(az[i+2],az[i+9])==0 ){
          i += 7;
        }
        break;
      }
      case 6: {  /* Changes to a ticket other than status or assignment */
        sprintf(zPrefix, "Changes to ticket #%.20s", az[i+5]);
        sprintf(zSuffix, " (By %.30s)", az[i+2]);
        zIcon = "arrow.gif";
        iconWidth = 9;
        /* Skip over subsequent lines of the same text */
        while( az[i+7] && atoi(az[i+8])==6 && strcmp(az[i],az[i+7])==0
               && strcmp(az[i+5],az[i+12])==0 ){
          i += 7;
        }
        break;
      }
      case 7: { /* Attachments */
        if( isdigit(az[i+3][0]) ){
          sprintf(zPrefix, "Attachment to ticket #%.20s: ", az[i+3]);
        }else{
          sprintf(zPrefix, "Attachment to %.100s: ", az[i+3]);
        }
        sprintf(zAttach, 
            "%.20s bytes <a href=\"attach_get/%.20s/%.100s\">%.100s</a>",
            az[i+4], az[i+6], az[i+5], az[i+5]);
        sprintf(zSuffix, "(by %.30s)", az[i+2]);
        zIcon = "arrow.gif";
        iconWidth = 9;         
        break;
      }
      default:
        /* Cannot happen */
        break;
    }
    @ <td valign="top" align="center" width=30>
    if( zIcon ){
      int w = iconWidth;
      @   <img src="%s(zIcon)" width=%d(w) height=9 align="bottom" alt="*">
    }
    @ </td>
    @ <td valign="top"%s(zBg)>
    if( zFg ){
      @ <font color="%s(zFg)">
    }
    output_formatted(zPrefix, 0);
    if( output_trim_message(zMsg, MN_CKIN_MSG, MX_CKIN_MSG) ){
      output_formatted(zMsg, 0);
      @ &nbsp;[...]
    }else{
      output_formatted(zMsg, 0);
    }
    if( zAttach[0] ){
      @ %s(zAttach)
    }
    output_formatted(zSuffix, 0);
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
  @ <table>
  @ <tr><td colspan=3>
  @ Show a timeline of
  @ <input type="text" name="d" value="%d(days)" size=5> days
  @ going backwards from
  pTm = localtime(&end);
  strftime(zDate, sizeof(zDate), "%Y-%b-%d", pTm);
  @ <input type="text" name="e" value="%s(zDate)" size=14>
  @ </td></tr>
  @ <tr>
  if( g.okCheckout ){
    zCkinPrfx = P("px");
    @ <td align="left" valign="top">
    @ <input type="radio" name="c"%s(showC==2?" checked":"") value="2">
    @   Show all check-ins<br>
    @ <input type="radio" name="c"%s(showC==1?" checked":"") value="1">
    @   Show only branch check-ins<br>
    @ <input type="radio" name="c"%s(showC==0?" checked":"") value="0">
    @   Show no check-ins<br>
    @ Only show check-ins of files with this prefix:<br>
    @ <input type="text" name="px" value="%h(zCkinPrfx?zCkinPrfx:"")" size="40">
    @ </td><td width=50>&nbsp;</td>
  }
  if( g.okRead ){
    @ <td valign="top">
    @ <input type="radio" name="s"%s(showS==3?" checked":"") value="3">
    @   Show all ticket changes of any kind<br>
    @ <input type="radio" name="s"%s(showS==2?" checked":"") value="2">
    @   Show all ticket status changes<br>
    @ <input type="radio" name="s"%s(showS==1?" checked":"") value="1">
    @   Show only "active" and "new" status changes<br>
    @ <input type="radio" name="s"%s(showS==0?" checked":"") value="0">
    @   Show no ticket status changes<br>
    @ </td></tr>
  }
  @ <tr><td valign="top">
  @ <input type="checkbox" name="dm"%s(divM?" checked":"") value="1">
  @   Divide timeline by milestones<br>
  @ <input type="checkbox" name="dt"%s(divT?" checked":"") value="1">
  @   Divide timeline by days<br>
  @ <input type="checkbox" name="debug1"%s(P("debug1")?" checked":"")
  @  value="1"> Show the SQL used to generate the timeline.<br>
  @ <input type="hidden" name="x" value="1">
  @ <input type="submit" value="Show Timeline">
  if( g.okAdmin ){
    @ &nbsp;&nbsp;&nbsp;
    @ <input type="submit" name="set" value="Make Default">
  }
  @ </td><td width=50>&nbsp;</td><td valign="top">
  if( g.okRead ){
    @ <input type="checkbox" name="a"%s(showA?" checked":"") value="1">
    @   Show assignment changes<br>
  }
  if( g.okCheckout ){
    @ <input type="checkbox" name="m"%s(showM?" checked":"") value="1">
    @   Show milestones<br>
  }
  @ <input type="checkbox" name="w"%s(showW?" checked":"") value="1">
  @   Show Wiki edits<br>
  @ <input type="checkbox" name="t"%s(showT?" checked":"") value="1">
  @   Show attachments
  @ </td></tr>
  @ </table>
  @ </form>
  common_footer();
}

/*
** Given a file version number, compute the previous version number.
** The new version number overwrites the old one.
**
** Examples:  "1.12" becomes "1.11".  "1.22.2.1" becomes "1.22".
**
** The special case of "1.1" becomes "".
*/
void previous_version(char *zVers){
  int j, x;
  int n = strlen(zVers);
  for(j=n-2; j>=0 && zVers[j]!='.'; j--){}
  j++;
  x = atoi(&zVers[j]);
  if( x>1 ){
    sprintf(&zVers[j],"%d",x-1);
  }else{
    for(j=j-2; j>0 && zVers[j]!='.'; j--){}
    zVers[j] = 0;
  }
}

/*
** Check a repository file for the presence of the -kb option.
*/
int has_binary_keyword(const char* filename){
  FILE* in;
  char line[80];
  int has_binary=0;

  in = fopen(filename, "r");
  if( in==0 ) return 2;

  while( fgets(line, sizeof(line), in) ){
    /* End of header? */
    if( line[0]=='\n' || line[0]=='\r' ){
      break;
    }

    /* Is this the "expand" field? */
#define EXPAND "expand"
    if( strncmp(line, EXPAND, strlen(EXPAND))==0 ){
      /* Does its value contain 'b'? */
      if( strchr(line+strlen(EXPAND), 'b') ){
        has_binary=1;
      }
      break;
    }
  }

  fclose(in);
  return has_binary;
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
** Perform the following substitutions on the input string zInCmd and
** write the result into a new string obtained from malloc.  Return the
** result.
**
**      %V1      First version number  (if first version is not NULL)
**      %V2      Second version number (if first version is not NULL)
**      %V       Second version number (if first version is NULL)
**      %F       Filename
**      %%       "%"
*/
static char *subst(const char *zIn, const char **azSubst){
  int i, k;
  char *zOut;
  int nByte = 1;

  /* Sanitize the substitutions
  */
  for(i=0; azSubst[i]; i+=2){
    azSubst[i+1] = quotable_string(azSubst[i+1]);
  }

  /* Figure out how must space is required to hold the result.
  */
  nByte = 1;  /* For the null terminator */
  for(i=0; zIn[i]; i++){
    if( zIn[i]=='%' ){
      int c = zIn[++i];
      if( c=='%' ){
        nByte++;
      }else{
        int j, len = 0;
        for(j=0; azSubst[j]; j+=2){
          if( azSubst[j][0]!=c ) continue;
          len = strlen(azSubst[j]);
          if( strncmp(&zIn[i], azSubst[j], len)!=0 ) continue;
          break;
        }
        if( azSubst[j]==0 ){
          nByte += 2;
        }else{
          nByte += strlen(azSubst[j+1]);
          i += len - 1;
        }
      }
    }else{
      nByte++;
    }
  }

  /* Allocate a string to hold the result
  */
  zOut = malloc( nByte );
  if( zOut==0 ) exit(1);

  /* Do the substituion
  */
  for(i=k=0; zIn[i]; i++){
    if( zIn[i]=='%' ){
      int c = zIn[++i];
      if( c=='%' ){
        zOut[k++] = '%';
      }else{
        int j, len = 0;
        for(j=0; azSubst[j]; j+=2){
          if( azSubst[j][0]!=c ) continue;
          len = strlen(azSubst[j]);
          if( strncmp(&zIn[i], azSubst[j], len)!=0 ) continue;
          break;
        }
        if( azSubst[j]==0 ){
          zOut[k++] = '%';
          zOut[k++] = c;
        }else{
          strcpy(&zOut[k], azSubst[j+1]);
          k += strlen(&zOut[k]);
          i += len - 1;
        }
      }
    }else{
      zOut[k++] = zIn[i];
    }
  }
  zOut[k] = 0;
  assert( k==nByte-1 );
  return zOut;
}

/*
** Diff two versions of a file, handling all exceptions.
**
** If oldVersion is NULL, then this function will output the
** text of version newVersion of the file instead of doing
** a diff.
*/
void diff_versions(
  const char *oldVersion,
  const char *newVersion,
  const char *file
){
  const char *zTemplate;
  char *zCmd;
  FILE *in;
  int i;
  const char *zFormat;
  const char *azSubst[10];
  char zLine[1000];

  if( file==0 ) return;

  /* Check file for binary keyword */
  if( has_binary_keyword(file) ){
    @ <p>
    @ %h(file) is a binary file
    @ </p>
    return; /* Don't attempt to compare binaries */
  }

  /* Find the command used to compute the file difference.
  */
  azSubst[0] = "F";
  azSubst[1] = file;
  if( oldVersion[0]==0 ){
    zTemplate = db_config("filelist","co -q -p'%V' '%F' | diff -c /dev/null -");
    azSubst[2] = "V";
    azSubst[3] = newVersion;
    azSubst[4] = 0;
  }else{
    zTemplate = db_config("filediff","rcsdiff -q '-r%V1' '-r%V2' -u '%F'");
    azSubst[2] = "V1";
    azSubst[3] = oldVersion;
    azSubst[4] = "V2";
    azSubst[5] = newVersion;
    azSubst[6] = 0;
  }
  zCmd = subst(zTemplate, azSubst);
  in = popen(zCmd, "r");
  free(zCmd);
  if( in==0 ) return;

  /* Output the result of the command.  If the first non-whitespace
  ** character is "<" then assume the command is giving us HTML.  In
  ** that case, do no translation.  If the first non-whitespace character
  ** is anything other than "<" then assume the output is plain text.
  ** Convert this text into HTML.
  */
  zFormat = 0;
  while( fgets(zLine, sizeof(zLine), in) ){
    for(i=0; isspace(zLine[i]); i++){}
    if( zLine[i]==0 ) continue;
    if( zLine[i]=='<' ){
      zFormat = "%s";
    }else{
      @ <pre>
      zFormat = "%h";
    }
    cgi_printf(zFormat, zLine);
    break;
  }
  while( fgets(zLine, sizeof(zLine), in) ){
    cgi_printf(zFormat, zLine);
  }
  if( zFormat && zFormat[1]=='h' ){
    @ </pre>
  }
  pclose(in);
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

  login_check_credentials();
  if( !g.okRead ){ login_needed(); return; }
  throttle(1);
  cn = atoi(PD("cn","0"));
  az = db_query("SELECT date, branch, milestone, user, message "
                "FROM chng WHERE cn=%d", cn);
  if( az[0]==0 ){ cgi_redirect("index"); return; }
  azFile = db_query("SELECT filename, vers, nins, ndel "
                    "FROM filechng WHERE cn=%d ORDER BY filename", cn);
  azTkt = db_query(
     "SELECT ticket.tn, ticket.title FROM xref, ticket "
     "WHERE xref.cn=%d AND xref.tn=ticket.tn "
     "ORDER BY ticket.tn", cn);
  azInspect = db_query(
     "SELECT inspecttime, inspector, result FROM inspect "
     "WHERE cn=%d ORDER BY inspecttime", cn);
  common_standard_menu("chngview", "search?c=1");
  common_header("Check-in [%d]", cn);
  tx = (time_t)atoi(az[0]);
  pTm = localtime(&tx);
  strftime(zDate, sizeof(zDate), "%Y-%b-%d %H:%M:%S", pTm);
  pTm = gmtime(&tx);
  strftime(zDateUTC, sizeof(zDateUTC), "%Y-%b-%d %H:%M:%S", pTm);
  @ <table cellpadding=1 cellspacing=0 width="100%%">
  @ <tr><td align="right" valign="top">Check-in Number:</td><td>%d(cn)</td>
  @   <td align="right" rowspan=4 valign="top">
  @    <a href="taghints?cn=%d(cn)">Tagging and branching hints</a><br>
  @    <a href="inspect?cn=%d(cn)">Add a new inspection report</a></td></tr>
  @ <tr><td align="right" valign="top">Date:</td>
  @   <td>%s(zDate) (local)
  @   <br>%s(zDateUTC) (UTC)</td></tr>
  @ <tr><td align="right">User:</td><td>%h(az[3])</td></tr>
  @ <tr><td align="right">Branch:</td><td>%h(az[1])</td></tr>
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
      @ <tr><td align="right" valign="top">
      @ <a href="tktview?tn=%h(azTkt[i])">#%h(azTkt[i])</a></td>
      @ <td width=8></td>
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
  cnt = 0;
  for(i=0; azFile[i]; i+=4){
    char zPVers[100];
    sprintf(zPVers, "%.90s", azFile[i+1]);
    previous_version(zPVers);
    if( g.okCheckout ){
      @ <tr><td><a href="rlog?f=%T(azFile[i])">%h(azFile[i])</a>
    }else{
      @ <tr><td>%h(azFile[i])
    }
    @ &nbsp;&nbsp;&nbsp;&nbsp;</td>
    if( strcmp(azFile[i+1],"1.1")==0 ){
      cnt++;
      @ <td>New File</td>
    }else{
      cnt++;
      @ <td>%h(zPVers)->%h(azFile[i+1])</td>
      if( azFile[i+2][0] && azFile[i+3][0] ){
        @ <td>&nbsp;&nbsp;&nbsp
        @ %h(azFile[i+2]) inserted, %h(azFile[i+3]) deleted </td>
      }
    }
    @ </tr>
  }
  @ </table>
  @ </td></tr></table>
  if( cnt>0 && g.okCheckout ){
    const char *zRoot = db_config("cvsroot", 0);
    if( zRoot==0 || access(zRoot,0) ) azFile[0] = 0;
    for(i=0; azFile[i]; i+=4){
      char *zFile;
      char zPVers[100];
      sprintf(zPVers, "%.90s", azFile[i+1]);
      previous_version(zPVers);
      zFile = find_repository_file(zRoot, azFile[i]);
      if( zFile==0 ) continue;
      @ <hr>
      @ %h(azFile[i]) &nbsp;&nbsp;&nbsp; %h(zPVers) -> %h(azFile[i+1])<br>
      diff_versions(zPVers, azFile[i+1], zFile);
      free(zFile);
    }
  }
  common_footer();
}

/*
** WEBPAGE: /chngedit
*/
void change_edit(void){
  char zCancel[200];
  int cn;
  const char *zMsg;
  char **az;

  login_check_credentials();
  if( !g.okWrite || !g.okCheckin ){
    login_needed();
    return;
  }
  throttle(1);
  cn = atoi(PD("cn","0"));
  if( cn<=0 ){
    cgi_redirect("index");
    return;
  }
  sprintf(zCancel,"chngview?cn=%d", cn);
  if( P("can") ){
    cgi_redirect(zCancel);
    return;
  }
  zMsg = P("m");
  if( zMsg ){
    db_execute("UPDATE chng SET message='%q' WHERE cn=%d", zMsg, cn);
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
  common_add_menu_item(zCancel, "Cancel");
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
  char zCancel[200];
  int cn;
  const char *zResult;

  login_check_credentials();
  if( !g.okWrite || !g.okCheckin ){
    login_needed();
    return;
  }
  throttle(1);
  cn = atoi(PD("cn","0"));
  if( cn<=0 ){
    cgi_redirect("index");
    return;
  }
  sprintf(zCancel,"chngview?cn=%d", cn);
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
  char zLink[200];

  login_check_credentials();
  if( !g.okRead ){ login_needed(); return; }
  throttle(1);
  cn = atoi(PD("cn","0"));
  az = db_query("SELECT date, branch, milestone, user, message "
                "FROM chng WHERE cn=%d", cn);
  if( az[0]==0 ){ cgi_redirect("index"); return; }
  sprintf(zLink,"chngview?cn=%d",cn);
  common_add_menu_item( zLink, "Back");
  common_header("Tagging And Branching Hints");
  tx = (time_t)atoi(az[0])-1;
  pTm = gmtime(&tx);
  strftime(zDateUTC, sizeof(zDateUTC), "%Y-%m-%d %H:%M:%S UTC", pTm);
  @ <p>To create a tag that occurs <i>before</i> 
  @ check-in <a href="%s(zLink)">[%d(cn)]</a>, go to the root of
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
  const char *zMsg;
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
  throttle(1);
  zTime = P("t");
  if( zTime==0 || (tm = parse_time(zTime))==0 ){
    zTime = "";
  }
  cn = atoi(PD("cn","0"));
  zMsg = remove_blank_lines(PD("m",""));
  zMType = PD("y","0");
  zBr = PD("br","");
  mtype = atoi(zMType);
  if( tm>0 && zTime[0] && zMsg[0] && mtype>0 ){
    if( P("del") && cn>0 ){
      db_execute("DELETE FROM chng WHERE cn=%d AND milestone", cn);
    }else if( cn>0 ){
      db_execute("UPDATE chng SET date=%d, user='%q', message='%q',"
                 "milestone=%d, branch='%q' "
                 "WHERE cn=%d",
         tm, g.zUser, zMsg, mtype, zBr, cn);
    }else{
      db_execute("INSERT INTO chng(date,milestone,user,message,branch) "
         "VALUES(%d,%d,'%q','%q','%q')",
         tm, mtype, g.zUser, zMsg, zBr);
    }
    cgi_redirect("index");
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
  common_standard_menu("msnew", 0);
  if( cn>0 ){
    common_header("Edit Milestone");
  }else{
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
  cgi_optionmenu(2,"y", zMType, "Release", "1", "Event", "2", 0);
  @ </td></tr>
  @ </td></tr>
  @ <tr><td align="right">Branch:</td><td>
  cgi_v_optionmenu(2, "br", zBr, (const char**)azAllBr);
  @ </td></tr>
  @ <tr><td align="right">Comment:</td>
  @    <td colspan=3>
  @    <input type="text" name="m" value="%h(zMsg)" size=70>
  @ <tr><td colspan=4 align="center">
  @   <input type="submit" value="Submit">
  if( cn>0 ){
    @   &nbsp;&nbsp;&nbsp;
    @   <input type="submit" value="Delete" name="del">
  }
  @ </td></tr></table>
  @ </form>
  common_footer();
}

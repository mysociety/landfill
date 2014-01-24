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
** This file contains code used to generate the "timeline.rss"
*/
#include <time.h>
#include <assert.h>
#include "config.h"
#include "rss.h"

/*
** Generate a common RSS header
*/

void common_rss_header( char *zTitle, char *zDescription ) {
  cgi_set_content_type("text/xml");
  @ <?xml version="1.0" encoding="ISO-8859-1"?>
  @ <!-- RSS generated by CVSTrac @VERSION@ -->
  @ <rss version="0.91">
  @ <channel>
  @ <title>%h(g.zName) - %h(zTitle)</title>
  @ <link></link>
  @ <description>%h(zDescription)</description>
  @ <generator>CVSTrac @VERSION@</generator>
  @ <ttl>30</ttl>
}

void common_rss_footer( void ) {
  @ </channel>
  @ </rss>
}

/*
** WEBPAGE: /index.rss
*/
void index_rss(void){
  common_rss_header( "Unauthorized", "No content available" );
  common_rss_footer();
}

/*
** WEBPAGE: /timeline.rss
*/
void timeline_rss(void){
  const char *zEnd;       /* Day at which timeline ends */
  time_t begin, end;      /* Beginning and ending times for the timeline */
  char **az;  
  time_t thisDate;
  int thisDay, lastDay;
  int i;
  struct tm *pTm;
  int len = 0;
  int days = 30;
  int showM;       /* Show milestones if 1.  Do not show if 0 */
  int showC;       /* Show all checkins if 2, branch checkis if 1, none if 0 */
  int showS;       /* 0: no status changes 1: active & new  2: everything */
  int showA;       /* Show assignment changes if 1 */
  int showW;       /* Show changes to Wiki pages if 1 */
  int divM;        /* Divide timeline by milestones if 1 */
  int divT;        /* Divide timeline by days if 1 */
  const char *zCkinPrfx;   /* Only show checkins of files with this prefix */
  const char *zFormat;
  char zSQL[3000];

  login_check_credentials();
  if( !g.okRead && !g.okCheckout ){ cgi_redirect("index.rss"); return; }
  history_update(0);
  zFormat = db_config("timeline_format", 0);
  if( zFormat && zFormat[0] ){
    showA = zFormat[0]-'0';
    showC = zFormat[1]-'0';
    showM = zFormat[2]-'0';
    showS = zFormat[3]-'0';
    showW = zFormat[4]-'0';
    divM = zFormat[5]-'0';
    divT = divM/2;
    divM %= 2;
    days = atoi(&zFormat[6]);
  }else{
    showM = showS = 1;
    showC = 2;
    showA = showW = 0;
    divM = 0;
    divT = 1;
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
      sprintf(&zSQL[len]," AND milestone");
      len += strlen(&zSQL[len]);
    }else if( showC==1 ){
      sprintf(&zSQL[len]," AND (milestone OR branch!='')");
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
       "  chng.message, "
       "FROM chng, filechng "
       "WHERE filechng.cn=chng.cn AND filechng.filename LIKE '%s%%'"
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
  if( len==0 ){
    static char *azDummy[] = { 0 };
    az = azDummy;
  }else{
    sprintf(&zSQL[len], " ORDER BY time DESC, type");
    az = db_query("%s",zSQL);
  }
  lastDay = 0;
  common_rss_header("Timeline", "Changes");
  for(i=0; az[i]; i+=7){
    char *zMsg;
    char zPrefix[400];
    char zSuffix[200];
    char zMsgBuf[5];

    zMsgBuf[0] = ' ';
    zMsgBuf[1] = 0;
    zMsg = zMsgBuf;
    thisDate = atoi(az[i]);
    pTm = localtime(&thisDate);
    thisDay = (pTm->tm_year+1900)*1000 + pTm->tm_yday;
    zPrefix[0] = 0;
    zSuffix[0] = 0;
    switch( atoi(az[i+1]) ){
      case 1: { /* A check-in or milestone */
        zMsg = az[i+6];
        if( az[i+3][0] && az[i+3][0]!='0' ){
          sprintf(zPrefix, "Milestone [%.20s]: ", az[i+5]);
        }else{
          if( az[i+4][0] ){
            sprintf(zPrefix, "Check-in [%.20s] on branch %.50s: ",
                    az[i+5], az[i+4]);
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
        break;
      }
      case 3: {  /* The status field of a ticket changed */
        char zType[50];
        sprintf(zType,"%.30s",az[i+4]);
        if( islower(zType[0]) ) zType[0] = toupper(zType[0]);
        sprintf(zPrefix, "%.30s ticket #%.20s, was %.20s.",
             zType, az[i+5], az[i+3]);
        sprintf(zSuffix, " (By %.30s)", az[i+2]);
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
        break;
      }
      case 5: {   /* Changes to a Wiki page */
        sprintf(zPrefix, "Wiki page %s ", az[i+5]);
        sprintf(zSuffix, "edited by %.30s", az[i+2]);
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
        /* Skip over subsequent lines of the same text */
        while( az[i+7] && atoi(az[i+8])==6 && strcmp(az[i],az[i+7])==0
               && strcmp(az[i+5],az[i+12])==0 ){
          i += 7;
        }
        break;
      }
      default:
        /* Cannot happen */
        break;
    }
    @ <item>
    @ <title>%h(zPrefix) %h(zSuffix)</title>
    if( output_trim_message(zMsg, MN_CKIN_MSG, MX_CKIN_MSG) ){
      @ <description>%h(zMsg) [...]</description>
    }else{
      @ <description>%h(zMsg)</description>
    }
    @ </item>
  }
  common_rss_footer();
}
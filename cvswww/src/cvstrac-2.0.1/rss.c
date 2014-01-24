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
static void common_rss_header(char *zTitle, char *zDescription, int nBuildDate){
  int nTTL = atoi(db_config("rss_ttl", "60"));
  cgi_set_content_type("text/xml");
  g.zLinkURL = g.zBaseURL;  /* formatting for output links... */
#if CVSTRAC_I18N
  @ <?xml version="1.0" encoding="%h(nl_langinfo(CODESET))"?>
#else
  @ <?xml version="1.0" encoding="ISO-8859-1"?>
#endif
  @ <rss version="2.0">
  @ <channel>
  @ <title>%h(g.zName) - %h(zTitle)</title>
  @ <link>%s(g.zBaseURL)/timeline</link>
  @ <description>%h(zDescription)</description>
  @ <language>en</language>
  @ <pubDate>%h(cgi_rfc822_datestamp( time(0) ))</pubDate>
  if( nBuildDate>0 ){
    const char* zBD = cgi_rfc822_datestamp(nBuildDate);
    @ <lastBuildDate>%h(zBD)</lastBuildDate>
    cgi_append_header(mprintf("Last-Modified: %h\r\n",zBD));
  }
  @ <generator>CVSTrac @VERSION@</generator>
  @ <ttl>%d(nTTL)</ttl>
}

void common_rss_footer( void ) {
  @ </channel>
  @ </rss>
  g.zLinkURL = 0;
}

/*
** WEBPAGE: /index.rss
*/
void index_rss(void){
  common_rss_header("Unauthorized", "No content available", 0);
  common_rss_footer();
}

/*
** Format wiki output and return it in a buffer.
*/
char *format_formatted(const char *zText){
  /* we'll need to restore this later */
  int n=0, n2=0;
  char *zFormatted;
  char *zContent = cgi_extract_content(&n);

  output_formatted(zText,0);
  zFormatted = cgi_extract_content(&n2);

  /* restore the original buffer */
  cgi_append_content(zContent,n);
  if( zContent ) free(zContent);

  return zFormatted;
}

static char *get_ticket_title(int tn){
  char *z = db_short_query("SELECT title FROM ticket WHERE tn=%d", tn);
  return ( z && z[0] ) ? z : mprintf("");
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
  int showC;       /* Show only trunk checkins if 3, all checkins if 2,
                   ** branch checkis if 1, none if 0 */
  int showS;       /* 0: no status changes 1: active & new  2: everything */
  int showA;       /* Show assignment changes if 1 */
  int showW;       /* Show changes to Wiki pages if 1 */
  int showT;       /* Show attachment additions */
  int divM;        /* Divide timeline by milestones if 1 */
  int divT;        /* Divide timeline by days if 1 */
  const char *zCkinPrfx;   /* Only show checkins of files with this prefix */
  const char *zFormat;
  char zSQL[4000];
  int nLastBuildDate;
  int rssDetail = atoi(db_config("rss_detail_level","5"));

  /*
  ** Allow a user to decrease the desired detail level, but never increase
  ** past what the admin has configured.
  */
  if( (i=atoi(PD("rss","100")))<rssDetail && i>=0 ){
    rssDetail = i;
  }

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
    showT = (showW>>1)&1;
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
      appendf(zSQL,&len,sizeof(zSQL)," AND milestone");
    }else if( showC!=2 ){
      appendf(zSQL,&len,sizeof(zSQL)," AND (milestone OR branch%c='')",
              (showC==3) ? '=' : '!');
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
         "  AND inspect.inspecttime<=%d AND inspect.inspecttime>=%d ",
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
       "WHERE filechng.cn=chng.cn AND filechng.filename LIKE '%s%%'"
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
  if( showS || showA ){
    if( len>0 ){
      appendf(zSQL,&len,sizeof(zSQL)," UNION ALL ");
    }
    appendf(zSQL,&len,sizeof(zSQL),
       "SELECT origtime AS 'time', 2 AS 'type', owner, type, description, "
       "       tn, title "
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
         "SELECT chngtime AS 'time', 6 AS 'type', user, oldval AS 'aux1', "
         "       newval AS 'aux2', tn AS 'ref', fieldid as 'text' "
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
    appendf(zSQL,&len,sizeof(zSQL), " ORDER BY time DESC, type");
    az = db_query("%s",zSQL);
  }
  
  /* If there is no data, just send empty RSS file.
  */
  if( az==0 || az[0]==0 ){
    common_rss_header("Timeline", "Changes", 0);
    common_rss_footer();
    return;
  }
  
  /* nLastBuildDate - last time the content of the channel changed.
  ** That is basically time of our most recent item in timeline.
  */
  nLastBuildDate = atoi(az[0]);

  /* We don't even need to build a response if nothing new has happened. */
  cgi_modified_since(nLastBuildDate);

  lastDay = 0;
  common_rss_header("Timeline", "Changes", nLastBuildDate);
  for(i=0; az[i]; i+=7){
    char *zMsg = 0;      /* HTML text for description */
    char *zWiki = 0;     /* Wiki text for description */
    char zLink[400];
    char zPrefix[1000];
    char zSuffix[200];
    int nEdits, nLastEdit;
    
    thisDate = atoi(az[i]);
    pTm = localtime(&thisDate);
    thisDay = (pTm->tm_year+1900)*1000 + pTm->tm_yday;
    zPrefix[0] = 0;
    zSuffix[0] = 0;
    zLink[0] = 0;
    switch( atoi(az[i+1]) ){
      case 1: { /* A check-in or milestone */
        if( rssDetail>=5 ) zWiki = az[i+6];  /* comment is wiki markup */
        if( az[i+3][0] && az[i+3][0]!='0' ){
          bprintf(zPrefix, sizeof(zPrefix), "Milestone [%.20s]: ", az[i+5]);
        }else{
          if( az[i+4][0] ){
            bprintf(zPrefix, sizeof(zPrefix),
                    "Check-in [%.20s] on branch %.50s: ",
                    az[i+5], az[i+4]);
          }else{
            bprintf(zPrefix, sizeof(zPrefix), "Check-in [%.20s]: ", az[i+5]);
          }
          bprintf(zSuffix, sizeof(zSuffix), " (By %.30s)", az[i+2]);
        }
        if( g.okCheckout ){
          bprintf(zLink,sizeof(zLink),"chngview?cn=%.20s",az[i+5]);
        }
        break;
      }
      case 2: {  /* A new ticket was created */
        bprintf(zPrefix, sizeof(zPrefix), "Create ticket #%.20s: ", az[i+5]);
        bprintf(zSuffix, sizeof(zSuffix), " (By %.30s)", az[i+2]);
        bprintf(zLink,sizeof(zLink), "tktview?tn=%.20s",az[i+5]);

        /*
        ** Ticket title is not wiki markup, but the description _is_.
        ** Include both of them.
        */
        zMsg = mprintf("Created #%.20s <i>%s</i>:", az[i+5], az[i+6]);
        if( rssDetail>=9 ) zWiki = az[i+4];
        break;
      }
      case 3: {  /* The status field of a ticket changed */
        char zType[50];
        if( rssDetail>=5 ) zMsg = get_ticket_title( atoi(az[i+5]) );
        bprintf(zType,sizeof(zType),"%.30s",az[i+4]);
        if( islower(zType[0]) ) zType[0] = toupper(zType[0]);
        bprintf(zPrefix, sizeof(zPrefix),"%.30s ticket #%.20s, was %.20s.",
             zType, az[i+5], az[i+3]);
        bprintf(zSuffix, sizeof(zSuffix), " (By %.30s)", az[i+2]);
        if( az[i+7] && atoi(az[i+8])==4 && strcmp(az[i],az[i+7])==0
            && strcmp(az[i+5],az[i+12])==0 ){
          i += 7;
          if( az[i+4][0]==0 ){
            appendf(zPrefix,0,sizeof(zPrefix),
                    " Unassign from %.50s.", az[i+3]);
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
        bprintf(zLink,sizeof(zLink),"tktview?tn=%.20s",az[i+5]);
        break;
      }
      case 4: {  /* The assigned-to field of a ticket changed */
        if( rssDetail>=5 ) zMsg = get_ticket_title( atoi(az[i+5]) );
        if( az[i+4][0]==0 ){
          bprintf(zPrefix, sizeof(zPrefix),
                  "Unassign ticket #%.20s from %.50s.",
                  az[i+5], az[i+3]);
        }else if( az[i+3][0]==0 ){
          bprintf(zPrefix, sizeof(zPrefix), "Assign ticket #%.20s to %.50s.",
                  az[i+5], az[i+4]);
        }else{
          bprintf(zPrefix, sizeof(zPrefix),
                  "Reassign ticket #%.20s from %.50s to %.50s",
                  az[i+5], az[i+3], az[i+4]); 
        }
        bprintf(zSuffix, sizeof(zSuffix), " (By %.30s)", az[i+2]);
        bprintf(zLink,sizeof(zLink),"tktview?tn=%.20s",az[i+5]);
        break;
      }
      case 5: {   /* Changes to a Wiki page */
        bprintf(zPrefix, sizeof(zPrefix), "Wiki page %.300s ", az[i+5]);
        /* Skip over subsequent lines of the same text and display 
        ** number of edits if greater then 1
        */
        nEdits = 1;
        while( az[i+7] && atoi(az[i+8])==5 && strcmp(az[i+5],az[i+12])==0
               && strcmp(az[i+2],az[i+9])==0 ){
          i += 7;
          nEdits++;
        }
        if( nEdits>1 ){
          bprintf(zSuffix, sizeof(zSuffix), "edited %d times by %.30s",
                  nEdits, az[i+2]);
        }else{
          bprintf(zSuffix, sizeof(zSuffix), "edited by %.30s", az[i+2]);
        }
        if( g.okRdWiki ){
          bprintf(zLink,sizeof(zLink),"wiki?p=%.300s",az[i+5]);
        }
        break;
      }
      case 6: {  /* Changes to a ticket other than status or assignment */
        bprintf(zSuffix, sizeof(zSuffix), " (By %.30s)", az[i+2]);
        /* Skip over subsequent lines of the same text and display 
        ** number of edits if greater then 1
        */
        nEdits = 1;
        if( rssDetail>=5 ) zMsg = get_ticket_title( atoi(az[i+5]) );
        if( 0==strcmp(az[i+6],"remarks") ){
          /*
          ** append remarks...
          */
          int len1 = strlen(az[i+3]);

          if( len1==0 ){
            zMsg = mprintf("Added to #%h <i>%h</i>:",
                           az[i+5], zMsg ? zMsg : "");
            if( rssDetail>=9 ) zWiki = az[i+4];
          }else if( strlen(az[i+4])>len1+5
                    && strncmp(az[i+3],az[i+4],len1)==0 ){
            zMsg = mprintf("Appended to #%h <i>%h</i>:",
                           az[i+5], zMsg ? zMsg : "");
            if( rssDetail>=9 ) zWiki = &(az[i+4])[len1];
          }
        }else{
          nLastEdit = atoi(az[i]);
          while( az[i+7] && atoi(az[i+8])==6 && strcmp(az[i+5],az[i+12])==0 
                 && strcmp(az[i+2],az[i+9])==0
                 && strcmp(az[i+13],"remarks")!=0 ){
            if( atoi(az[i+7])!=nLastEdit ){
              nLastEdit = atoi(az[i+7]);
              nEdits++;
            }
            i += 7;
          }
        }
        if( nEdits>1 ){
          bprintf(zPrefix, sizeof(zPrefix), "%d changes to ticket #%.20s",
                  nEdits, az[i+5]);
        }else{
          bprintf(zPrefix, sizeof(zPrefix), "Changes to ticket #%.20s",
              az[i+5]);
        }
        bprintf(zLink,sizeof(zLink),"tktview?tn=%.20s",az[i+5]);
        break;
      }
      case 7: { /* Attachments */
        if( isdigit(az[i+3][0]) ){
          bprintf(zPrefix, sizeof(zPrefix), "Attachment to ticket #%.20s: ",
                  az[i+3]);
          zMsg = mprintf(
              "Attachment to ticket "
              "<a href=\"%s/tktview?tn=%.20t\">#%.20s</a>: "
              "%h bytes <a href=\"%s/attach_get/%T/%T\">%h</a>",
              g.zBaseURL, az[i+3], az[i+3], az[i+4], g.zBaseURL, az[i+6],
              az[i+5], az[i+5]);
          if( g.okRead ){
            bprintf(zLink,sizeof(zLink),"tktview?tn=%.20t",az[i+3]);
          }
        }else{
          bprintf(zPrefix, sizeof(zPrefix),
                  "Attachment to %.300s: %h bytes %h", 
                  az[i+3], az[i+4], az[i+5]);
          zMsg = mprintf(
              "Attachment to <a href=\"%s/wiki?p=%.300t\">%.300s</a>: "
              "%h bytes <a href=\"%s/attach_get/%T/%T\">%h</a>",
              g.zBaseURL, az[i+3], az[i+3], az[i+4], g.zBaseURL,
              az[i+6], az[i+5], az[i+5]);
          if( g.okRdWiki ){
            bprintf(zLink,sizeof(zLink),"wiki?p=%.300t",az[i+3]);
          }
        }
        if( rssDetail>=9 ){
          zWiki = db_short_query("SELECT description FROM attachment "
                                 "WHERE atn=%d", atoi(az[i+6]));
        }
        bprintf(zSuffix, sizeof(zSuffix), "(by %.30s)", az[i+2]);
        break;
      }
      case 8: { /* An inspection */
        zMsg = az[i+6]; /* result is not wiki markup */
        if( az[i+4][0] ){
          bprintf(zPrefix, sizeof(zPrefix),
                  "Inspection of [%.20s] on branch %.50s: ",
                  az[i+5], az[i+4]);
        }else{
          bprintf(zPrefix, sizeof(zPrefix), "Inspection of [%.20s]: ", az[i+5]);
        }
        bprintf(zSuffix, sizeof(zSuffix), " (By %.30s)", az[i+2]);
        if( g.okCheckout ){
          bprintf(zLink,sizeof(zLink),"chngview?cn=%.20s",az[i+5]);
        }
        break;
      }
      default:
        /* Cannot happen */
        break;
    }
    @ <item>
    if( zLink[0] ){
      @ <link>%s(g.zBaseURL)/%s(zLink)</link>
    }
    @ <title>%h(zPrefix) %h(zSuffix)</title>

    @ <description>\

    if(zMsg){
      if( rssDetail<9 && output_trim_message(zMsg, MN_CKIN_MSG, MX_CKIN_MSG) ){
        @ %h(zMsg) [...]\
      }else{
        @ %h(zMsg)\
      }
      cgi_printf("%h","<br>");
    }

    if( zWiki ){
      if( rssDetail<9 && output_trim_message(zWiki, MN_CKIN_MSG, MX_CKIN_MSG) ){
        zWiki = format_formatted(zWiki);
        @ %h(zWiki) [...]\
      }else{
        zWiki = format_formatted(zWiki);
        @ %h(zWiki)\
      }
      free(zWiki);  /* format_formatted(), no longer points to az[?] */
    }
    
    if( rssDetail>=5 && zWiki==0 && zMsg==0 ){
      @ %h(zPrefix) %h(zSuffix)\
    }
    @ </description>

    @ <pubDate>%h(cgi_rfc822_datestamp(thisDate))</pubDate>
    @ </item>
  }
  common_rss_footer();
}

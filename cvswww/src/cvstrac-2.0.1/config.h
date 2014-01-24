/*
** System header files used by all modules
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <sqlite3.h>
#include <assert.h>
#if defined(__linux__) || defined(__sun__)
#include <crypt.h>
#endif

/*
** Standard colors.  These colors can also be changed using a stylesheet.
*/

/* A blue border and background.  Used for the title bar and for dates
** in a timeline.
*/
#define BORDER1       "#a0b5f4"      /* Stylesheet class: border1 */
#define BG1           "#d0d9f4"      /* Stylesheet class: bkgnd1 */

/* A red border and background.  Use for releases in the timeline.
*/
#define BORDER2       "#ec9898"      /* Stylesheet class: border2 */
#define BG2           "#f7c0c0"      /* Stylesheet class: bkgnd2 */

/* A gray background.  Used for column headers in the Wiki Table of Contents
** and to highlight ticket properties.
*/
#define BG3           "#d0d0d0"      /* Stylesheet class: bkgnd3 */

/* A light-gray background.  Used for title bar, menus, and rlog alternation
*/
#define BG4           "#f0f0f0"      /* Stylesheet class: bkgnd4 */

/* A deeper gray background.  Used for branches
*/
#define BG5           "#dddddd"      /* Stylesheet class: bkgnd5 */

/* Default HTML page header */
#define HEADER "<html>\n" \
               "<head>\n" \
               "<link rel=\"alternate\" type=\"application/rss+xml\"\n" \
               "   title=\"%N Timeline Feed\" href=\"%B/timeline.rss\">\n" \
               "<link rel=\"index\" title=\"Index\" href=\"%B/index\">\n" \
               "<link rel=\"search\" title=\"Search\" href=\"%B/search\">\n" \
               "<link rel=\"help\" title=\"Help\"\n" \
               "   href=\"%B/wiki?p=CvstracDocumentation\">\n" \
               "<title>%N: %T</title>\n</head>\n" \
               "<body bgcolor=\"white\">"

/* Default HTML page footer */
#define FOOTER "<div id=\"footer\"><small><small>\n" \
               "<a href=\"about\">CVSTrac version %V</a>\n" \
               "</small></small></div>\n" \
               "</body></html>\n"

/* In the timeline, check-in messages are truncated at the first space
** that is more than MX_CKIN_MSG from the beginning, or at the first
** paragraph break that is more than MN_CKIN_MSG from the beginning.
*/
#define MN_CKIN_MSG   100
#define MX_CKIN_MSG   300

/* Maximum number of seconds for any HTTP or CGI handler to live. This
** prevents denials of service caused by bad queries, endless loops, or
** other possibilties.
*/
#define MX_CHILD_LIFETIME 300

/* If defined, causes the query_authorizer() function to return SQLITE_DENY on
** invalid calls rather than just SQLITE_IGNORE. This is not recommended for
** production use since it's basically a denial of service waiting to happen,
** but CVSTrac developers _should_ enable it to catch incorrect use of
** db_query calls (i.e. using them for something other than SELECT).
*/
/* #define USE_STRICT_AUTHORIZER 1 */

/* Unset the following to disable internationalization code. */
#ifndef CVSTRAC_I18N
# define CVSTRAC_I18N 1
#endif

#if CVSTRAC_I18N
# include <locale.h>
# include <langinfo.h>
#endif
#ifndef CODESET
# undef CVSTRAC_I18N
# define CVSTRAC_I18N 0
#endif

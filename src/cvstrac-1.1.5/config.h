/*
** System header files used by all modules
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <sqlite.h>
#include <assert.h>

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

/* A light-gray background.  Used for title bar and menus.
*/
#define BG4           "#e0e0e0"      /* Stylesheet class: bkgnd4 */

/* In the timeline, check-in messages are truncated at the first space
** that is more than MX_CKIN_MSG from the beginning, or at the first
** paragraph break that is more than MN_CKIN_MSG from the beginning.
*/
#define MN_CKIN_MSG   100
#define MX_CKIN_MSG   300

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

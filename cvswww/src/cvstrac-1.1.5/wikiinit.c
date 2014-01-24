/*** AUTOMATICALLY GENERATED FILE - DO NOT EDIT ****
**
** This file was generated automatically by the makewikiinit.c
** program.  See the sources to that program for additional
** information.
**
** This file contains code used to initialize the wiki for a new
** CVSTrac database.
*/
#include "config.h"
#include "wikiinit.h"
#include <time.h>

static const char zCamelCase[] =
@ *CamelCase* is the capitalization technique used to identify hyperlinks
@ in wiki. Many wiki servers (including this one) use {quote: CamelCase}, 
@ but others use more complex hyperlinking syntax.
@ 
@ See also: WikiPageNames, WhatIsWiki.
;
static const char zChrootJailForCvstrac[] =
@ *Launching CVSTrac Into A Chroot Jail*
@ 
@ CVSTrac will automatically put itself into a chroot jail if its
@ first argument is _chroot_ and it is started as root.  After the
@ *chroot* argument, the next two arguments are the directory which
@ should be the new root directory and the user that the program
@ should run as after it is chrooted.  (CVSTrac always drops any superuser
@ privileges before doing any real work, regardless of whether or not
@ you use the *chroot* option.)  After the _chroot_ argument and its
@ two parameters, the usual *cgi* or *http* keyword and its arguments
@ appear.
@ 
@ For the http://cvs.hwaci.com:2080/cvstrac/ site, CVSTrac is run from
@ inetd.  The inetd.conf configuration line looks like this:
@ 
@   2080 steam tcp nowait.1000 root /usr/bin/cvstrac \
@        cvstrac chroot /home/cvs cvs http /
@ 
@ The three arguments *chroot /home/cvs cvs* tell the server to put
@ itself into a chroot jail located at */home/cvs* and drop superuser
@ privilege and become user *cvs* before continuing.  The first three
@ arguments are then removed and processing continues as if the
@ command had been launched as
@ 
@   cvstrac http /
@ 
@ Notice that the directory argument to the _http_ directive, the argument
@ that tells CVSTrac where to look for its database, is specified 
@ relative to the chroot jail, not to the regular filesystem.
@ 
@ *Configuring The Jail*
@ 
@ CVSTrac does a popen() of a few commands for some of its operations.
@ It uses the following external programs: *rlog rcsdiff co.*
@ The popen() procedure uses /bin/sh and rcsdiff uses diff.
@ All of these external programs most be available inside the chroot
@ jail.  In addition, CVSTrac needs to access a stripped-down version
@ of /etc/passwd at one point.  It also needs access to the /tmp
@ directory and to the special file /dev/null.  The /etc/localhost
@ file is optional, but without it, all times are shown in UTC.
@ 
@ The following listing shows all the files and directories in the chroot jail
@ for the canonical CVSTrac installation:
@ 
@   bin
@   bin/sh
@   bin/bash
@   dev
@   dev/null
@   etc
@   etc/localtime
@   etc/passwd
@   lib
@   lib/libc.so.6
@   lib/ld-linux.so.2
@   lib/libtermcap.so.2
@   tmp
@   usr
@   usr/bin
@   usr/bin/rcsdiff
@   usr/bin/co
@   usr/bin/rlog
@   usr/bin/diff
@ 
@ A similar set of files will be required in any chroot jail for
@ CVSTrac, though the details may vary.  For example, the required
@ libraries might change.  Or you might use a different shell.
@ (The bin/sh above is a hard link to bin/bash)
@ 
@ *Setup Changes*
@ 
@ After you get CVSTrac running inside a chroot jail, you'll need to log in
@ as the "setup" user, go to the "setup" page, and change the path to the
@ CVS repository and the log file so that they are relative to the chroot
@ jail not the regular filesystem.  Other than that, though, no additional
@ setup changes are required.
;
static const char zCreatingNewWiki[] =
@ These are the steps to create a new wiki pages:
@ 
@ 1: Choose a name for your new page.  Use the established
@    naming rules for selecting new WikiPageNames.
@ 
@ 2: Edit an existing wiki page to add a hyperlink to your
@    new page.  Save your edits.
@ 
@ 3: Click on the hyperlink you just created to take you to
@    the new page.  The screen will say that this page has
@    never been created.
@ 
@ 4: Click the "Edit" button to enter edit mode and begin adding
@    content to your new page.
@ 
@ In this wiki implementation, only an administrator
@ can DeleteWiki.
;
static const char zCvsRepositorySplitting[] =
@ Suppose you have two projects, named "one" and "two", in a single
@ CVS repository named "common".  Your directory structure looks something
@ like this:
@ 
@     /home --- cvs --- common --- CVSROOT
@                               |- one
@                               `- two
@ 
@ In other words, the CVS repository is rooted at /home/cvs/common.
@ All the source files are in /home/cvs/common/one and /home/cvs/common/two
@ and the administrative files are in /home/cvs/common/CVSROOT.  
@ Our objective is to split this one repository into two, as follows:
@ 
@     /home --- cvs --- one --- CVSROOT
@                    |       `- one
@                    |
@                    `- two --- CVSROOT
@                            `- two
@ 
@ In the new setup, the first CVS repository is at /home/cvs/one and
@ the second CVS repository is at /home/cvs/two.  Each has its own
@ subdirectory for content and for administrative files.
@ 
@ The first step is to create the new directories and copy in the
@ appropriate files.
@ 
@     mkdir /home/cvs/one
@     mv /home/cvs/common/one /home/cvs/one
@     mkdir /home/cvs/one/CVSROOT
@     cp /home/cvs/common/CVSROOT/* /home/cvs/one/CVSROOT
@     mkdir /home/cvs/two
@     mv /home/cvs/common/two /home/cvs/two
@     mv /home/cvs/common/CVSROOT /home/cvs/two
@     rmdir /home/cvs/common
@ 
@ Notice that the content files for each project were moved into
@ their appropriate CVS repository but that the administrative
@ files were copied into both of the new repositories.  The next
@ step is to modify the CVSROOT/history files in each of the new
@ repositories so that they only contain information about the
@ files in their respective repositories.
@ 
@     cd /home/cvs/one/CVSROOT
@     mv history history.orig
@     grep 'one/' history.orig >history
@     cd /home/cvs/two/CVSROOT
@     mv history history.orig
@     grep 'two/' history.orig >history
@ 
@ We're done.  Now each project is in its own CVS repository.  Now
@ you can create a separate CVSTrac database for each project.
;
static const char zCvstracDocumentation[] =
@ The following topics are available:
@ 
@ *: ChrootJailForCvstrac
@ *: LocalizationOfCvstrac
;
static const char zDeleteWiki[] =
@ CVSTrac allows wiki pages to be deleted only by
@ users with administrator privileges.  When an administrator
@ looks at a wiki page, one of the buttons on the upper right-hand
@ corner of the page will be [Delete].  The administrator just
@ has to click on this hyperlink to delete the page.
@ (Actually, a confirmation page appears first, to help prevent
@ accidental page deletions.)
@ 
@ The administrator can also delete historical copies of a page.
@ To do this, put the page in [History] mode and select the version
@ of the page to be deleted.  Then click on the [Delete] hyperlink
@ as before.  The confirmation screen that appears gives the
@ administrator there options:
@ 
@ 1: Delete the page completely.
@ 
@ 2: Delete the version of the page being viewed and
@    all versions.
@ 
@ 3: Delete just the version being viewed.
@ 
@ CVSTrac stores the complete text of every version of a wiki page
@ in its database.  If a single page has many edits, it can begin
@ to take up a lot of space in the database, especially if the page
@ is big.  Administrators may want to periodically go through and
@ delete all but the most recent two or three versions of large,
@ frequently edited pages.
@ 
@ All deletions are permanent.
;
static const char zFormattingWikiPages[] =
@ **Paragraphs**
@ 
@ Use one or more blank lines to separate paragraphs.  If the
@ first line of a paragraph begins with a tab or with two or 
@ more spaces, then that whole paragraph is shown verbatim 
@ (that is, within <pre>...</pre> markup in HTML.)  The 
@ formatting rules below do not apply to verbatim paragraphs.
@ 
@ **Bold Or Italic Fonts**
@ 
@ Text contained between asterisks is rendered in a *bold font.*
@ If you use two or three asterisks in a row, instead of just
@ one, the bold text is also shown in a larger font.
@ Text between underscores is render in an _italic font._
@ All font markers must start at the beginning of a word and must
@ finish at the end of a word within the same paragraph.
@ 
@ **Lists**
@ 
@ If a line begins with the characters "*:" followed by a space or
@ tab, then that line becomes an item in a bullet list.  Similarly,
@ if the line begins with "N:" (where N is any number including a
@ multi-digit number) then the line becomes an item in an
@ enumeration list. Enumeration items are automatically renumbered
@ so the values of N do not need to be in accending order.
@ 
@ Lines that begin with "_:" are indented like a
@ bullet list but do not display the bullet.  
@ 
@ Make nested lists by adding colons.  For example, to make a
@ second level bullet, begin the line with "*::".
@ 
@ **Hyperlinks**
@ 
@ Links to other pages are created automatically whenever the name
@ of another wiki page is mentioned in the text.  (See WikiPageNames.)
@ If you want to put a CamelCase word in your text but you do not
@ want it to become a hyperlink, enclose the name in
@ "{quote: {quote: ...}}".
@ 
@ A link to an external website is created for every URL beginning
@ with "http:", "https:", "ftp:", or "mailto:".  If the URL ends with
@ ".jpg", ".jpeg", ".gif", or ".png" then the image that the URL points
@ to is displayed inline on the wiki page.  You can also create an
@ inline image using markup like this: "{quote: {image: URL}}".
@ Using the {quote: {image:...}} markup allows the image URL to be
@ relative.  This allows an image stored in an
@ attachment to be displayed inline.
@ 
@ Text of the form *#NNN* where the *NNN* is a valid ticket number
@ becomes a hyperlink to the ticket.  Text of the form *[NNN]* where
@ *NNN* is a valid check-in or milestone number becomes a hyperlink
@ to that check-in or milestone.  This hyperlinks only work if the
@ user has permission to read tickets, check-ins, and/or milestones.
@ 
@ To create a hyperlink on arbitrary text, use "{quote: {link: ...}}"
@ markup.  Any text of the form:  "{quote: {link: URL PHRASE}}" 
@ displays PHRASE as hyperlink to URL.  URL can be an absolute 
@ URL beginning with a prefix like "http:", or it can be a 
@ relative URL referring to another page within the same CVSTrac 
@ server.  For example, to create a link to a report, one might 
@ write: "{quote: {link: rptview?rn=1 Active Tickets}}".
@ 
@ **Horizontal Lines**
@ 
@ A horizontal line (the <hr> markup of HTML) is generated 
@ whenever four or more "-" characters appear on a line by 
@ themselves.
@ 
@ **Other Markup Rules**
@ 
@ The special markup "{quote: {linebreak}}" will be rendered
@ as a line break or hard return.  The content of 
@ "{quote: {quote: ...}}" markup is shown verbatim.
;
static const char zFrequentlyAskedQuestions[] =
@ If you have a new question, enter it on this page and leave the
@ _Answer:_ section blank.  Someone will eventually add text that
@ provides an answer.  At least, that's the theory...
@ 
@ ----
@ _Question:_
@ Can CVSTrac be run in a chroot jail?
@ 
@ _Answer:_
@ See ChrootJailForCvstrac.
@ 
@ ----
@ _Question:_
@ I don't want CVSTrac to change my CVSROOT/passwd file. How do I keep it
@ from changing that file when I add new users to CVSTrac?
@ 
@ _Answer:_
@ CVSTrac will not change the CVSROOT/passwd file if you turn off
@ write permission on that file.  There is also an option under
@ the Setup menu that an administrator can use to turn off CVSROOT/passwd
@ writing.
@ 
@ ----
@ _Question:_
@ I have multiple projects in the same CVS repository.  How can I set
@ of separate CVSTrac instances for each project?
@ 
@ _Answer:_
@ There is an option under the Setup menu that will cause CVSTrac to
@ ignore all files whose names do not match a specified prefix.
@ If all of your projects have distinct prefixes, you can use this
@ option to restrict each CVSTrac instance to a single project.
;
static const char zLocalizationOfCvstrac[] =
@ *Localization of CVSTrac*
@ 
@ All the text generated by CVSTrac is American English.  But dates can
@ be displayed in the local language and format.
@ To show dates in the local language, do this:
@ 
@ *: Create bash script to run CVSTrac in required locale.
@    Example : _uk-cvstrac_
@ 
@        #!/bin/bash
@        LC_ALL=uk_UA.KOI8-U
@        export LC_ALL
@        /usr/local/bin/cvstrac $*
@ 
@ *: Replace "cvstrac" in your scripts by name of your new script
@    (uk-cvstrac, in my case).
@ 
@ If you want to run a localized CVSTrac in chroot environment
@ (such as described in ChrootJailForCvstrac),
@ use the following command to find all files required by the
@ cvstrac binary.
@ 
@    strace -e file cvstrac 2>logfile ...
;
static const char zMultipleCvsRepositories[] =
@ Many people are used to hosting multiple projects in a single
@ CVS repository.  Unfortunately, CVSTrac is not able to deal with
@ that situation.  CVSTrac requires a separate CVS repository for
@ each project.
@ 
@ _To_do: Talk about how to set up multiple CVS repositories.
@ Or maybe refer the user to the CVS documentation on how to do
@ that._
@ 
@ If you already have multiple projects in your CVS repository, 
@ you will need to split it into multiple separate repositories.
@ How to do this is explained at CvsRepositorySplitting.
;
static const char zWhatIsWiki[] =
@ The term "{quote: WikiWiki}" 
@ ("wiki wiki" means "quick" in the Hawaiian language and
@ is pronounced "wickee wickee") can be used to identify either a
@ type of hypertext document or the software used to write it. 
@ Often called "wiki" for short, the collaborative software application
@ enables web documents to be authored collectively using a simple markup
@ scheme and without the content being reviewed prior to its acceptance.
@ The resulting collaborative hypertext document, also called either "wiki" 
@ or "{quote: WikiWikiWeb}," is typically produced by a community of users.
@ Many wikis (including this one) are easily identified by their use of
@ CamelCase hyperlinks.
@ 
@ The original {quote: WikiWikiWeb} was established in 1995 by Ward Cunningham, 
@ who invented and named the Wiki concept and produced the first
@ implementation of a {quote: WikiWiki} server.  Ward's server is still online
@ at http://www.c2.com/cgi/wiki.  Visit that server for additional
@ information.  Or read a book Ward (and Bo Leuf) wrote on the concept:
@ _The Wiki Way: Quick collaboration on the Web_, Addison-Wesley,
@ ISBN-020171499X.
@ Some people maintain that only Ward's wiki should be called "Wiki"
@ (upper case) or the "{quote: WikiWikiWeb}".
;
static const char zWikiDocumentation[] =
@ The following topics are available:
@ 
@ *: FormattingWikiPages
@ *: WikiPageNames
@ *: WhatIsWiki
@ *: CreatingNewWiki
@ *: DeleteWiki
;
static const char zWikiIndex[] =
@ This is the default wiki index page that is automatically
@ created when you initialize a new CVSTrac database.
@ You should edit this page to include information specific
@ to your particular CVSTrac installation.
@ 
@ Where are some links to more information:
@ 
@ *: FrequentlyAskedQuestions
@ *: CvstracDocumentation
@ *: WikiDocumentation
@ *: CreatingNewWiki
;
static const char zWikiPageNames[] =
@ Wiki pages names are written using CamelCase.
@ Within a wiki pages, any word written in {quote: CamelCase}
@ becomes a hyperlink to the wiki page with that same name.
@ 
@ The formatting rules for wiki page names on this
@ server are shown below.  (Other servers may use minor
@ variations on these rules.)
@ 
@ 1: The name must consist of alphabetic characters only.
@    No digits, spaces, punctuation, or underscores.
@ 
@ 2: There must be at least two capital letters in the name.
@ 
@ 3: The first character of the name must be capitalized.
@ 
@ 4: Every capital letter must be followed by one or more
@    lower-case letters.
;
static const struct {
  const char *zName;
  const char *zText;
} aPage[] = {
  { "CamelCase", zCamelCase },
  { "ChrootJailForCvstrac", zChrootJailForCvstrac },
  { "CreatingNewWiki", zCreatingNewWiki },
  { "CvsRepositorySplitting", zCvsRepositorySplitting },
  { "CvstracDocumentation", zCvstracDocumentation },
  { "DeleteWiki", zDeleteWiki },
  { "FormattingWikiPages", zFormattingWikiPages },
  { "FrequentlyAskedQuestions", zFrequentlyAskedQuestions },
  { "LocalizationOfCvstrac", zLocalizationOfCvstrac },
  { "MultipleCvsRepositories", zMultipleCvsRepositories },
  { "WhatIsWiki", zWhatIsWiki },
  { "WikiDocumentation", zWikiDocumentation },
  { "WikiIndex", zWikiIndex },
  { "WikiPageNames", zWikiPageNames },
};
void initialize_wiki_pages(void){
  int i;
  time_t now = time(0);
  for(i=0; i<sizeof(aPage)/sizeof(aPage[0]); i++){
    db_execute("INSERT INTO wiki(name,invtime,locked,who,ipaddr,text)"
       "VALUES('%s',%d,%d,'','','%q')",
       aPage[i].zName, (int)now,
       strcmp(aPage[i].zName, "WikiIndex")!=0, 
       aPage[i].zText);
  }
}

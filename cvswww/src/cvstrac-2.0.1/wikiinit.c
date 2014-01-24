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
@ In this wiki implementation, usually only an administrator
@ can {wiki:DeleteWiki delete a page}.
;
static const char zCvsRepositorySplitting[] =
@ Suppose you have two projects, named "one" and "two", in a single
@ CVS repository named "common".  Your directory structure looks something
@ like this:
@ 
@     /home
@                               |- one
@                               `- two
@ 
@ In other words, the CVS repository is rooted at /home/cvs/common.
@ All the source files are in /home/cvs/common/one and /home/cvs/common/two
@ and the administrative files are in /home/cvs/common/CVSROOT.
@ Our objective is to split this one repository into two, as follows:
@ 
@     /home
@                    |       `- one
@                    |
@                    `- two
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
static const char zCvstracAdmin[] =
@ ***CVSTrac Administration***
@ 
@ *Getting Started*
@ 
@ *: {link: http://www.cvstrac.org/cvstrac/wiki?p=DownloadCvstrac Downloading}
@ *: {link: http://www.cvstrac.org/cvstrac/wiki?p=HowToCompile Compiling}
@ *: {link: http://www.cvstrac.org/cvstrac/wiki?p=CvstracInstallation Installing}
@ *: {wiki: ChrootJailForCvstrac Secure installations}
@ *: {wiki: LocalizationOfCvstrac Localization}
@ 
@ *Setup Notes*
@ 
@ The CVSTrac configuration pages are mostly self-explanatory. Some notes are in
@ order for other things.
@ 
@ *: {wiki: CvstracAdminHomePage Site home page}
@ *: {wiki: CvstracAdminTicketState Choosing ticket states}
;
static const char zCvstracAdminHomePage[] =
@ ***CVSTrac Home Page***
@ 
@ The default home page for a CVSTrac installation is a simple index page.
@ 
@ If a Wiki page called HomePage exists, is non-empty, and is _locked_, that page
@ will be used instead of the index page (which is still accessible under a "Main
@ Menu" link).
;
static const char zCvstracAdminTicketState[] =
@ ***CVSTrac Ticket State***
@ 
@ The default ticket states are (at the database level) _new_, _review_, _defer_,
@ _active_, _fixed_, _tested_, and _closed_.
@ 
@ While it is certainly possible to change them, note that internally CVSTrac
@ interprets states starting with the characters "n" or "a" as *active* and those
@ starting with "f", "t" or "c" as *fixed*.
;
static const char zCvstracAttachment[] =
@ ***CVSTrac Attachments***
@ 
@ CVSTrac allows arbitrary files to be attached to any
@ {wiki: CvstracWiki wiki} and {wiki: CvstracTicket ticket} pages.
@ 
@ Attachments to a page are generally listed near the bottom of the page.
@ 
@ **Attaching a File**
@ 
@ Assuming you have proper permissions, clicking on an *Attach* link in a wiki or
@ ticket page will send you to an upload form. Choose the file from your local
@ disk and, optionally, add a
@ {wiki: FormattingWikiPages wiki-formatted} description. Then upload.
@ 
@ Note that any provided description cannot be editted after uploading.
@ 
@ Each uploaded attachment is internally assigned a unique identifier. As such,
@ you can upload the same filename multiple times to as many places as you'd
@ like, as many times as you'd like.
@ 
@ Maximum attachments sizes are
@ configurable. The default is usually 100k.
@ 
@ **Getting Attachments**
@ 
@ Uploaded attachments can be downloaded simply by clicking on the link in the
@ listing. CVSTrac preserves the original MIME type so as long as the uploader
@ got that part right things like special viewers should be automatically
@ launched by the browser.
@ 
@ Image attachments may also be inlined by the attachment filename with
@ {quote: {image:}} markups. Note that if the same filename was uploaded multiple
@ times the results are undefined.
;
static const char zCvstracBrowse[] =
@ ***CVSTrac Browser***
@ 
@ CVSTrac includes a powerful repository browser. It allows you to quickly
@ navigate through the repository (deleted or otherwise) and view the
@ {wiki: CvstracFileHistory change history} for any given file.
@ 
@ **Short View**
@ 
@ The basic browser view is simply a list of filenames. Directories are always
@ listed first. Clicking on a file takes you to the
@ {wiki: CvstracFileHistory file history} page.
@ 
@ **Long View**
@ 
@ More recently added, the Long view is similar to that provided by applications
@ like {link: http://freshmeat.net/projects/cvsweb/ cvsweb}. Files are listed in
@ a table with revision, user, age and {wiki: CvstracCheckin check-in} comments.
@ Clicking on the column headers causes the display to be sorted according to the
@ column.
@ 
@ Clicking on the revision takes you directly to the
@ {wiki: CvstracFileview file view} for that revision. Selecting the filename
@ takes you to the {wiki: CvstracFileHistory file history} page. You can also go
@ directly to the {wiki: CvstracCheckin check-in}.
@ 
@ **Deleted Files**
@ 
@ The browser shows deleted files with a red 'X' icon. Because of how CVS does
@ branches, this deleted state only reflects the most recent commit for that file
@ in any branch. It may (and probably is) still be an active file _somewhere_ in
@ the revision tree.
;
static const char zCvstracCheckin[] =
@ ***CVSTrac Check-ins***
@ 
@ A check-in (sometimes called a patchset or changeset) is simply a commit to the
@ repository. It includes a message
@ describing the change, a source code diff, and various information such as who
@ made the change, when it was made.
@ 
@ CVSTrac attempts to represent a CVS commit as a single atomic operation (called
@ a patch-set), even when the commit includes multiple directories. Other SCMs
@ supported by CVSTrac would typically already represent commits atomically.
@ 
@ **Check-in Features**
@ 
@ *Fields*
@ 
@ *: *Check-in number* is a unique number for the patchset which can be
@ referenced from any {wiki: FormattingWikiPages wiki} using the
@ {quote: [nnn]} markup.
@ *: *User* is the repository user who made the change. This may not necessarily
@ match a CVSTrac user.
@ *: *Branch* is check-in on a branch. Branch activity is usually highlighted in
@ various displays with a slightly different background.
@ 
@ *Comment*
@ 
@ A check-in comment is simply the contents of the CVS commit message. However,
@ CVSTrac interprets such comments as
@ {wiki: FormattingWikiPages wiki markup}, allowing for a much richer
@ presentation when reading change messages in a browser.
@ 
@ Check-in comments can be editted within CVSTrac (i.e. to fix an incorrect
@ ticket number). However, any such changes are only help in the CVSTrac database
@ and aren't visible in the repository itself. On the flip side, direct
@ manipulation of the repository (such as with the =cvs admin= command) usually
@ won't propogate to the CVSTrac database either.
@ 
@ *Files*
@ 
@ Each check-in has a list of associated files. Following each file will bring
@ you the {wiki: CvstracFileHistory file history} page.
@ 
@ *Diffs*
@ 
@ When viewing a check-in, a complete _diff_ of the change is shown. The
@ formatting of this _diff_ may be enriched with
@ custom filter.
@ 
@ *Tickets*
@ 
@ A check-in can be associated with a specific
@ {wiki: CvstracTicket ticket} by mentioning the ticket
@ number in {quote: #n} format inside the commit message. This allows development
@ activity to be associated with a particular problem report.
@ 
@ **Committing Changes**
@ 
@ Changes are committed in the usual fashion (using a CVS command-line program).
@ However, in order to maximize the grouping of commits within a single patchset,
@ it's recommended that related commits be done all at once, with the same commit
@ message.
@ 
@ Commits can be made using {wiki: FormattingWikiPages wiki markup}.
@ Commits made in relation to a specific ticket can be associated with that
@ ticket simply by mentioning the ticket number. i.e.
@ 
@   cvs commit -m "Fix for ticket #45" Makefile src/file.c include/header.h
@ 
@ **Inspections**
@ 
@ {wiki: CvstracInspection Inspections} are short comments (40 chars)
@ attached to check-ins. There's no limit to the number of inspections that can
@ be associated with a check-in.
@ 
@ **Patchsets**
@ 
@ A patchset is basically a single _diff_ of all the changes in a check-in. In
@ theory, it's designed such that one can download it and recreate the changes
@ using _patch_. In practice, this may not work well with binary files.
;
static const char zCvstracDocumentation[] =
@ ***CVSTrac Documentation***
@ 
@ _A Web-Based {wiki: CvstracTicket Bug} And
@ {wiki: CvstracCheckin Patch-Set} Tracking System For CVS_
@ 
@ **Overview**
@ 
@ *: Single {wiki: CvstracTimeline timeline} shows all activity at a
@ glance
@ *: Automatically generates a {wiki: CvstracCheckin patch-set} log from
@ CVS {wiki: CvstracCheckin check-in} comments
@ *: User-defined color-coded {wiki: CvstracReport database queries}
@ *: {wiki: CvstracAdmin Web-based administration} of the
@ =CVSROOT/passwd= file
@ *: Built-in {wiki: CvstracBrowse repository browser}
@ *: Built-in {wiki: CvstracWiki Wiki}
@ *: Very simple {wiki: CvstracAdmin setup} - a self-contained
@ executable runs as CGI, from inetd, or as
@ a stand-alone web server
@ 
@ **CVSTrac Objects**
@ 
@ *: {wiki: CvstracTicket Tickets}
@ *: {wiki: CvstracCheckin Check-ins and Patch-Sets}
@ *: {wiki: CvstracWiki Wiki pages}
@ *: {wiki: CvstracReport Reports}
@ *: {wiki: CvstracMilestone Milestones}
@ *: {wiki: CvstracAttachment File Attachments}
@ *: {wiki: CvstracInspection Inspections}
@ 
@ **Using CVSTrac**
@ 
@ *: {wiki: CvstracTimeline Using the timeline}
@ *: {wiki: CvstracSearch Searching for things}
@ *: {wiki: CvstracBrowse Browsing the repository}
@ *: {wiki: CvstracFileHistory Viewing file changes}
@ *: {wiki: CvstracFileview Getting file contents}
@ *: {wiki: CvstracLogin Managing your account}
@ 
@ **Setup and Maintenance**
@ 
@ *: {wiki: CvstracAdmin Administrator Documentation}
@ *: {wiki: ChrootJailForCvstrac chroot Jails}
@ *: {wiki: LocalizationOfCvstrac Localization}
;
static const char zCvstracFileHistory[] =
@ ***CVSTrac File History***
@ 
@ The file history view, typically reached after clicking on a repository
@ filename, shows the full chronological revision history for a particular file.
@ From this history it's possible to view individual versions of a file, diff
@ between subsequent versions, or see the {wiki: CvstracCheckin check-in} which
@ created a version. For CVS, branch activity is also shown.
;
static const char zCvstracFileview[] =
@ ***CVSTrac File View***
@ 
@ Viewing a specific revision of a file brings up the file view. This shows the
@ filename (which links to the {wiki: CvstracFileHistory file history}, the
@ revision, and the file contents (run through an optional
@ filter). It's also possible to get a raw version of
@ the file without any HTML markup.
@ 
@ Note that binary files cannot be viewed.
;
static const char zCvstracInspection[] =
@ ***CVSTrac Inspections***
@ 
@ An inspection is simply a short note associated with a
@ {wiki: CvstracCheckin check-in}. Currently, the limit is 40 characters.
@ 
@ Inspections may be used to document the results of code reviews and testing.
@ They may be used to note whether or not the fix worked. They may also be used
@ to provide directions such as whether or not a check-in should be merged or
@ backported.
@ 
@ Any number of inspections may be associated with a given check-in. If they
@ check-in is associated with a {wiki: CvstracTicket ticket} then the
@ inspections will also be displayed in the ticket view and history pages.
@ 
@ Inspections may not be editted or deleted after they're created.
;
static const char zCvstracLogin[] =
@ ***CVSTrac Login***
@ 
@ If you're *anonymous*, you might want to read
@ {wiki: CvstracDocumentation something else}.
@ 
@ If you have a CVSTrac account, you can login by clicking the "Not logged in"
@ message found just below the page title. Login buttons are also avilable in
@ other places and you'll probably be prompted to login if you try something
@ which needs extra permissions.
@ 
@ **Requirements**
@ 
@ In order to login, browser cookies must be enabled. CVSTrac only uses session
@ cookies, so you'll need to login each new browser session. Additionally,
@ CVSTrac tracks your IP address. User behind proxies using variable addresses
@ will have serious difficulty using CVSTrac sites.
@ 
@ **Passwords**
@ 
@ You can change your password by following an "Logout" link or clicking on the
@ "Logged in" text. You may also log out that way.
@ 
@ **User Pages**
@ 
@ Recent versions of CVSTrac have introduced user home pages. Basically, each
@ user can have a personal {wiki: CvstracWiki wiki page}. Once a user creates
@ such a page, his or her name will become a hotlink back to the page in various
@ places such as the {wiki: CvstracTimeline timeline}. This page can be used as a
@ personal scratchpad or can include embedded reports relevant to the user.
@ 
@ Once the user is logged in, they're user name in the upper left corner becomes
@ a link to their page.
;
static const char zCvstracMilestone[] =
@ ***CVSTrac Milestones***
@ 
@ A milestone is an object which pins an event to a specific name and date. This
@ would include things like releases, branch points, builds, project milestones,
@ etc.
@ 
@ **Milestone Features**
@ 
@ *Fields*
@ 
@ *: *Date and Time* should be obvious. A date may be in the future.
@ *: *Type* is either _Release_ or _Event_. _Release_ milestones may be shown in
@ {wiki: CvstracFileHistory file revision} views.
@ *: *Branch* would identify a specific CVS code branch.
@ *: *Comment* is a {wiki: FormattingWikiPages wiki field} describing the
@ milestone.
@ 
@ A milestone may also be associated with tickets by referencing the ticket
@ number in the comment. Some milestones will also be associated with a
@ directory, such as those created by CVS repository activity.
@ 
@ **Creating A Milestone**
@ 
@ There are two ways to create a milestone. Once created, milestones can't
@ currently be deleted.
@ 
@ *Manual Method*
@ 
@ By clicking on the *Milestone* link, one can manually create a milestone and
@ populate each field directly.
@ 
@ *Repository Method*
@ 
@ You can create an _Event_ milestone using the =cvs rtag= command. The name of
@ the tag becomes the milestone comment. For example:
@ 
@   cvs rtag build_5918 cvstrac
@ 
@ will create an _Event_ milestone for the =cvstrac= directory with "build_5918"
@ as the milestone comment.
@ 
@ **Using Milestones**
@ 
@ Milestones are mostly useful as markers in various pages, such as the
@ {wiki: CvstracTimeline timeline}.
;
static const char zCvstracReport[] =
@ ***CVSTrac Reports***
@ 
@ A report is simply a {link: http://www.sqlite.org/ SQLite} database query with
@ special functions and output formatting appropriate for web presentation.
@ 
@ The basic idea is that a SQL query is run and each resulting row is formatted
@ as a separate row in an HTML table. Table headers are determined dynamically
@ from column names and, in a few cases, special columns get their own
@ interpretations (to specify a row background color, or identify a column as
@ containing linkable ticket numbers).
@ 
@ A report query can be written against most tables in
@ the CVSTrac database and must contain a single SELECT statement.
@ Subqueries are not allowed.
@ 
@ **Viewing Reports**
@ 
@ The *Reports* link brings up a list of all available reports. Each report can
@ be viewed simply by selecting the title. The raw (unformatted) data can be seen
@ (and downloaded) from within a report view using the *Raw Data* link.
@ 
@ The SQL used to generate the report can also be viewed at any time. This is
@ useful if you wanted to write custom queries outside of CVSTrac against the
@ SQLite database. Keep in mind, however, that some functions are only available
@ inside CVSTrac. And, of course, you'll lose the meaning of some of the
@ "special" columns.
@ 
@ Reports may also be embedded into {wiki: CvstracWiki wiki pages} using the
@ {quote:{report: rn}} syntax, where _rn_ is the report number as listed in the
@ report URL (_not_ the same number in the report list).
@ 
@ **Creating a report**
@ 
@ You can create a new report directly (following the appropriate link) or by
@ making a copy of an existing report (particularly one that does almost what you
@ need). In order to create or edit a report, a user must have the
@ query permission set.
@ 
@ Each report should have a unique and self-explanatory title.
@ 
@ *Tables*
@ 
@ Most tables in the CVSTrac database are available for use in reports. However,
@ keep in mind that the usual access restrictions are applied to all queries. In
@ other words, users without
@ checkout permissions will not be able to see
@ the contents of the *chng* table.
@ 
@ *Special Column Names*
@ 
@ *: *bgcolor* indicates an HTML table background color which will be used for
@ the entire row. The column itself will now be displayed.
@ 
@ *: A column named {quote: #} indicates that the column contains
@ {link:wiki?p=CvstracTicket ticket numbers}. Numbers in that column will be
@ represented as links to the appropriate ticket and an extra _edit_ column will
@ be shown for users with ticket write permissions.
@ 
@ *: Columns named with a leading {quote: _} will be shown by themselves as
@ separate rows. The row contents are assumed to be
@ {link:wiki?p=FormattingWikiPages wiki formatted}. This is useful for things
@ like ticket remarks, descriptions, check-in comments, attachment descriptions,
@ etc.
@ 
@ The =wiki()=, =tkt()= and =chng()= functions also give some control over column
@ formatting.
@ 
@ **Available SQL Functions**
@ 
@ See the {link: http://www.sqlite.org/ SQLite} documentation for the standard
@ functions.
@ 
@ *: =sdate()= converts an integer which is the number of seconds since 1970 into
@ a short date or time description.  For recent dates (within the past 24 hours)
@ just the time is shown.  (ex: 14:23)  For dates within the past year, the month
@ and day are shown.  (ex: Apr09).  For dates more than a year old, only the year
@ is shown.
@ 
@ *: =ldate()= converts an integer which is the number of seconds since 1970 into
@ a full date and time description.
@ 
@ *: =now()= takes no arguments and returns the current time in seconds since
@ 1970. =now()= is useful mostly for calculating relative cutoff times in
@ queries.
@ 
@ *: =user()= takes no arguments and returns the user ID of the current user.
@ 
@ *: =aux()= takes a single argument which is a parameter name.  It then returns
@ the value of that parameter.  The user is able to enter the parameter on a
@ form. If a second parameter is provided, the value of that parameter will be
@ the initial return value of the =aux()= function. =aux()= allows creation of
@ reports with extra query capabilities, such as:
@ 
@   SELECT
@     cn as 'Change #',
@     ldate(date) as 'Time',
@     message as 'Comment'
@   FROM chng
@   WHERE date>=now()-2592000 AND user=aux('User',user())
@   ORDER BY date DESC
@ 
@ which allows a user to enter a userid and get back a list of
@ {link:wiki?p=CvstracCheckin check-ins} made within the last 30 days.
@ 
@ *: =option()= takes a parameter name as an argument and, as with *aux()*,
@ returns the value of that parameter. The user is able to select an option value
@ from a drop down box. The second argument is a SQLite query which returns one
@ or two columns (the second column being a value description). For example:
@ 
@   SELECT
@     cn as 'Change #',
@     ldate(date) as 'Time',
@     message as 'Comment'
@   FROM chng
@   WHERE date>=now()-2592000
@     AND user=option('User','SELECT id FROM user')
@   ORDER BY date DESC
@ 
@ *: =cgi()= returns the value of a CGI parameter (or the second argument if the
@ CGI parameter isn't set). This is mostly useful in embedded reports.
@ 
@ *: =parsedate()= converts an ISO8601 date/time string into the number of
@ seconds since 1970. It would be useful along with the =aux()= function for
@ creating queries with variable time.
@ 
@ *: =search()= takes a (space separated) keyword pattern and a target text
@ and returns an integer score which increases as more of the keywords are
@ found in the text. The following scoring pattern is used:
@ |0|No sign of the word was found in the text|
@ |6|The word was found but not on a word boundry|
@ |8|The word was found with different capitalization|
@ |10|The word was found in the text exactly as given|
@ 
@ *: =wiki()= causes its argument to be rendered as wiki markup.
@ 
@ *: =tkt()= causes its argument to be rendered as a ticket number. For example:
@ 
@   SELECT
@     tkt(tn) AS 'Tkt',
@     owner AS 'Owner',
@   FROM ticket
@ 
@ *: =chng()= causes its argument to be rendered as a check-in.
@ 
@ *: =path()= is used to extract complete filename from FILE table.
@ It takes 3 parameters: isdir, dir and base. For example:
@ 
@   SELECT path(isdir, dir, base) AS 'filename' FROM file
@ 
@ *: =dirname()= takes filename as only argument and extracts parent directory
@  name from it.
@ 
@   SELECT dirname('/path/to/dir/') => '/path/to/'
@   SELECT dirname('/path/to/dir/file.c') => '/path/to/dir/'
@ 
@ *: =basename()= takes filename as only argument and extracts basename from it.
@ 
@   SELECT basename('/path/to/dir/') => 'dir'
@   SELECT basename('/path/to/dir/file.c') => 'file.c'
@ 
;
static const char zCvstracSearch[] =
@ ***CVSTrac Search***
@ 
@ CVSTrac allows for full-text searching on {wiki: CvstracTicket ticket} title,
@ remarks and descriptions, {wiki: CvstracCheckin check-in comments},
@ and {wiki: CvstracWiki wiki pages}.
@ 
@ Additionally, it can search for attachment and repository filenames. This is a
@ convenient way to navigate to files deep in a repository.
@ 
@ CVSTrac _cannot_ currently do full-text searches on the contents of files.
@ 
@ **Search Syntax**
@ 
@ There is not special syntax. The search algorithm takes all the search keywords
@ and scores them against the various database fields. Results are returned in
@ decreasing score order.
;
static const char zCvstracTicket[] =
@ ***CVSTrac Tickets***
@ 
@ A ticket is an object describing and tracking a bug, feature request, project,
@ action item, or any other type of thing that you might want to track within a
@ software development team.
@ 
@ Unlike some issue tracking systems, CVSTrac doesn't associate significant
@ amounts of process overhead with tickets. There aren't any constraints on who
@ can change specific fields, what states can lead to other states, who can
@ assign tickets, etc.
@ 
@ **Ticket Features**
@ 
@ *Properties*
@ 
@ Tickets have the following fields available:
@ 
@ *: *Ticket Number* is unique to each ticket and can be referenced in
@ {wiki: FormattingWikiPages wiki markup} using the
@ {quote: #n} syntax.
@ *: *Title* should be as descriptive as possible since it appears in
@ various cross-references, reports, ticket link titles, etc.
@ *: *Type* describes the kind of ticket, such as _bug_, _action item_, etc.
@ Ticket types are configurable.
@ *: *Status* describes the current state of the ticket. Possible states
@ are also configurable.
@ *: *Severity* defines how debilitating the issue is. A scale from one to five
@ is used, with one being most critical.
@ *: *Priority* decribes how quickly the ticket should be resolved. This isn't
@ necessarily related to *Severity* (i.e. a serious bug in a non-operational
@ product isn't always high priority).
@ *: *Assigned To* identifies which CVSTrac user should be handling the ticket.
@ *: *Creator* identifies which CVSTrac user created the ticket (including,
@ possible, *anonymous*)
@ *: *Version* should be used to report the product version related to the
@ ticket.
@ *: *Created* shows when the ticket was created.
@ *: *Last Changed* shows when the ticket was last changed.
@ *: *Subsystem* is intended to identify which component/area has a problem.
@ This list _must_ be configured by the administrator.
@ *: *Derived From* lists a parent ticket, if any.
@ *: *Contact* lists a way to get hold of the problem reporter. Typically, this
@ is an e-mail address. Contact information is _not_ made available to
@ *anonymous* CVSTrac users.
@ *: *Description* is a {wiki: FormattingWikiPages wiki} field providing
@ information about the nature of the ticket.
@ 
@ Additional custom ticket fields may be
@ defined by the administrator. This could include things like billing
@ information, due dates, completion percentages, resolutions, reporting
@ organizations, etc.
@ 
@ *Remarks*
@ 
@ The *Remarks* property is a special {wiki: FormattingWikiPages wiki}
@ field which can be editted directly, but also has a special _append_ option
@ where users can add comments and discussion to a ticket. When appending, a
@ timestamp and user name is automatically added into the remarks.
@ 
@ *Related Objects*
@ 
@ Any {wiki: CvstracCheckin check-ins} and
@ {wiki: CvstracMilestone milestones} related to the ticket are listed in
@ their own sections.
@ 
@ Tickets may also have _children_ which reference them in their *Derived From*
@ properties. These child tickets will be listed in a separate section, allowing
@ a certain amount of hierarchical project task management.
@ 
@ *History*
@ 
@ A separate page containing a full ticket history provides a chronologically
@ ordered list of _all_ events associated with a ticket. Property changes,
@ remarks, check-ins, attachments, inspections, etc, are all listed here.
@ 
@ Changes to certain fields, such as *Description* and *Remarks* are shown in
@ _diff_ style.
@ 
@ *Attachments*
@ 
@ Arbitrary files may be
@ {wiki: CvstracAttachment attached} to tickets. This could include things
@ like screenshots, specifications, standards documents, etc. Attachments are of
@ limited size.
@ 
@ **Creating New Tickets**
@ 
@ A new ticket is created by clicking on the *Ticket* link available in the
@ navigation bar. A new ticket form will be opened and the user would then enter
@ all appropriate information.
@ 
@ The ticket form is a combination of text fields and dropdown menus. Dropdown
@ menus provide a fixed set of items to choose from and are often populated with
@ reasonable defaults.
@ 
@ A large text area is provided for the problem description. As with most
@ components of CVSTrac, this text area allows (and encourages) the use of
@ {wiki: FormattingWikiPages wiki formatting}.
@ 
@ As with any such system, detailed and complete information is encouraged.
@ 
@ **Changing Tickets**
@ 
@ Ticket properties can be editted by following the _Edit_ link in the ticket
@ view. Remarks can be added by following the _Add remarks_ link in the *Remarks*
@ area of a ticket view.
@ 
@ A ticket can be automatically associated with a
@ {wiki: CvstracMilestone milestone} or
@ {wiki: CvstracCheckin check-in} simply by mentioning the ticket in the
@ appropriate message using the {quote: #n} wiki markup.
@ 
@ In some cases, ticket changes may result in
@ notifications being triggered, if the
@ administrator has configured this.
@ 
@ **Undoing Changes**
@ 
@ When in the _history_ view, it's possible to undo changes. A user with *delete*
@ permissions or the user responsible
@ for the change can do this within 24 hours while a user with *admin*
@ permissions can do so at any time. An _undo_
@ link is included at the last item of the history page in this case.
@ 
@ **Deleting Tickets**
@ 
@ Users with *setup* permissions can always delete
@ tickets. Tickets should only be deleted as a last resort. If
@ a ticket contains valuable project history, it should instead be closed.
@ 
@ Users (except *anonymous*) with *delete* permissions
@ can delete tickets created by *anonymous* within 24 hours of ticket creation.
;
static const char zCvstracTimeline[] =
@ ***CVSTrac Timeline***
@ 
@ The timeline is a unified chronological view of all
@ {wiki: CvstracTicket ticket}, {wiki: CvstracCheckin check-in},
@ and {wiki: CvstracWiki wiki} activity on a given CVSTrac project.
@ 
@ **Organization**
@ 
@ A timeline is basically a table of CVSTrac objects. Normally, these objects are
@ broken up by day or by milestone. Each object is dated, has some kind of
@ identifying icon, a description of the object, _some_ detail about the change,
@ the name of the user responsible, and, if you have sufficient permissions, a
@ link to a more detailed view of the object.
@ 
@ At the bottom of the page is a grouping of controls to change various timeline
@ settings.
@ 
@ **Filtering**
@ 
@ Most of the timeline toggles are about filtering what objects are seen. It's
@ also possible to control the time window shown in the timeline by adjusting the
@ number of days and the end date/time.
@ 
@ **Preferences**
@ 
@ If you change any timeline settings, the new settings are stored in a browser
@ cookie. Additionally, a user with *admin* settings can force his/her current
@ timeline settings to be the default for all other users.
@ 
@ **Activity Feeds**
@ 
@ The timeline is available as an RSS feed. Just follow the "RSS" link if the
@ site admin doesn't have feed auto-discovery enabled.
;
static const char zCvstracWiki[] =
@ ***CVSTrac Wiki***
@ 
@ CVSTrac includes a full-featured {wiki: WhatIsWiki wiki} document
@ system. What's unique is the ability to use
@ {wiki: FormattingWikiPages wiki markup} virtually anywhere where text
@ can be entered, including things like
@ {wiki: CvstracTicket tickets} and {wiki: CvstracCheckin check-ins}.
@ 
@ **Wiki Documentation**
@ 
@ *: FormattingWikiPages
@ *: WikiPageNames
@ *: WhatIsWiki
@ *: CreatingNewWiki
@ *: DeleteWiki
@ 
@ **Revisions**
@ 
@ Each version of a CVSTrac wiki page is kept in the database. This means that
@ the entire history of a page can be examined, differences identified, old
@ changes resurrected, etc.
@ 
@ *History*
@ 
@ Clicking the *History* link on any wiki page will toggle on a list of all
@ revisions of the page, including the datestamp and user who made the change.
@ It's then possible to quickly view each version of the page. It's also possible
@ to {link: wiki?p=DeleteWiki delete} specific revisions of a page.
@ 
@ Wiki page revisions which have been deleted will, obviously, not be available
@ in this list.
@ 
@ *Diff*
@ 
@ Clicking on the *Diff* link for any revision of a wiki page will show the
@ changes between that revision and the previous revision.
@ 
@ **Attachments**
@ 
@ Arbitrary files can be {wiki: CvstracAttachment attached} to wiki pages
@ by clicking on the *Attach* link and uploading files.
@ 
@ Attachments to a page can be reference by filename in {quote: {image:}} markup
@ sections, allowing images to be attached to pages and directly embedded in the
@ page content.
;
static const char zDeleteWiki[] =
@ CVSTrac normally allows wiki pages to be deleted only by
@ users with administrator privileges. Users with *delete* permissions are also
@ able to delete some pages within 24 hours of creation. This is normally done to
@ help prevent {link: http://cvstrac.org/cvstrac/wiki?p=WikiSpam WikiSpam}.
@ 
@ When a user with delete credentials
@ looks at a wiki page, one of the buttons on the upper right-hand
@ corner of the page will be [Delete].  The user just
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
@ **Fonts**
@ 
@ Text contained between asterisks is rendered in a *bold font.*
@ If you use two or three asterisks in a row, instead of just
@ one, the bold text is also shown in a larger font.
@ Text between underscores is rendered in an _italic font._
@ Text between equals is rendered in a =fixed font.=
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
@ If you want to use alternative text, use the form
@ "{quote: {wiki: TITLE TEXT}}".
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
@ A pathname for any file in the repository becomes a hyperlink to its
@ "rlog" page. This only works if the user has check-out permissions.
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
@ **Tables**
@ 
@ A table is created by starting a line with a "|" character, separating each
@ cell with a "|", and ending each row with a "|". For example, a 3x3 table is
@ created with:
@ 
@   |cell 1|cell 2|cell 3|
@   |cell 4|cell 5|cell 6|
@   |cell 7|cell 8|cell 9|
@ 
@ **Reports**
@ 
@ A {link: reportlist report} can be embedded on any page using the
@ "{quote: {report: rn}}" markup. A report can have text flow around
@ it using the "{quote: {leftreport: rn}}" or "{quote: {rightreport: rn}}"
@ markups. Note that the report number _rn_ is not the same as the order
@ in the {link: reportlist report listing}; follow the link for the report
@ in order to get that.
@ 
@ **Other Markup Rules**
@ 
@ The special markup "{quote: {linebreak}}" will be rendered
@ as a line break or hard return.  The content of
@ "{quote: {quote: ...}}" markup is shown verbatim.
@ 
@ {markups}
;
static const char zFrequentlyAskedQuestions[] =
@ If you have a new question, enter it on this page and leave the
@ _Answer:_ section blank.  Someone will eventually add text that
@ provides an answer.  At least, that's the theory...
@ 
@ _Question:_
@ Can CVSTrac be run in a chroot jail?
@ 
@ _Answer:_
@ See ChrootJailForCvstrac.
@ 
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
  { "CvstracAdmin", zCvstracAdmin },
  { "CvstracAdminHomePage", zCvstracAdminHomePage },
  { "CvstracAdminTicketState", zCvstracAdminTicketState },
  { "CvstracAttachment", zCvstracAttachment },
  { "CvstracBrowse", zCvstracBrowse },
  { "CvstracCheckin", zCvstracCheckin },
  { "CvstracDocumentation", zCvstracDocumentation },
  { "CvstracFileHistory", zCvstracFileHistory },
  { "CvstracFileview", zCvstracFileview },
  { "CvstracInspection", zCvstracInspection },
  { "CvstracLogin", zCvstracLogin },
  { "CvstracMilestone", zCvstracMilestone },
  { "CvstracReport", zCvstracReport },
  { "CvstracSearch", zCvstracSearch },
  { "CvstracTicket", zCvstracTicket },
  { "CvstracTimeline", zCvstracTimeline },
  { "CvstracWiki", zCvstracWiki },
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
    char* zOld = db_short_query("SELECT text FROM wiki "
                                "WHERE name='%q' "
                                "ORDER BY invtime LIMIT 1",
                                aPage[i].zName);
    if(zOld && zOld[0] && 0==strcmp(zOld,aPage[i].zText)) continue;
    db_execute("INSERT INTO wiki(name,invtime,locked,who,ipaddr,text)"
       "VALUES('%s',%d,%d,'setup','','%q')",
       aPage[i].zName, -(int)now,
       strcmp(aPage[i].zName, "WikiIndex")!=0, 
       aPage[i].zText);
  }
}

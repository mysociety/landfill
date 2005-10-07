<?
/*
 * index.php:
 * Admin pages for Placeopedia.
 * 
 * Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
 * Email: francis@mysociety.org. WWW: http://www.mysociety.org
 *
 * $Id: index.php,v 1.1 2005-10-07 19:07:57 matthew Exp $
 * 
 */

require_once "../conf/general";
require_once "../phplib/admin-pop.php";
require_once "../../phplib/template.php";
require_once "../../phplib/admin-phpinfo.php";
require_once "../../phplib/admin-serverinfo.php";
require_once "../../phplib/admin-configinfo.php";
require_once "../../phplib/admin.php";

$pages = array(
    new ADMIN_PAGE_POP_LATEST,
    new ADMIN_PAGE_POP_MAIN,
    new ADMIN_PAGE_POP_INCORRECTPLACES,
    # new ADMIN_PAGE_EMBED('pbwebstats', 'Log Analysis', OPTION_ blah ),
    null, // space separator on menu
    new ADMIN_PAGE_SERVERINFO,
    new ADMIN_PAGE_CONFIGINFO,
    new ADMIN_PAGE_PHPINFO,
);

admin_page_display(str_replace("http://", "", OPTION_URL), $pages, new ADMIN_PAGE_POP_SUMMARY);

?>

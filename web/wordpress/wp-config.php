<?php

/* Include mySociety config file. */
if (!@include "../conf/general"
    && !@include "../../conf/general"
    && !@include "../../../conf/general") {
    /* Should handle error */
    print "Error including conf/general in wp-config.php on mysociety.org WordPress";
    exit;
}

// ** MySQL settings ** //
define('DB_NAME', OPTION_COP_DB_NAME);
define('DB_USER', OPTION_COP_DB_USER);
define('DB_PASSWORD', OPTION_COP_DB_PASSWORD);
define('DB_HOST', OPTION_COP_DB_HOST);

// You can have multiple installations in one database if you give each a unique prefix
$table_prefix  = 'wp_';   // Only numbers, letters, and underscores please!

// Change this to localize WordPress.  A corresponding MO file for the
// chosen language must be installed to wp-includes/languages.
// For example, install de.mo to wp-includes/languages and set WPLANG to 'de'
// to enable German language support.
define ('WPLANG', '');

/* That's all, stop editing! Happy blogging. */

define('ABSPATH', dirname(__FILE__).'/');
require_once(ABSPATH.'wp-settings.php');
?>

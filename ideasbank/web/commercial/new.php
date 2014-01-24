<?

include_once '../../conf/general';
include_once '../fns.php';
include_once '../fns_new.php';
include_once '../../../phplib/template.php';

template_set_style('../../templates');
template_draw('header-commercial');
echo '<form action="/commercial/new" method="post">';
idea_new_main('commercial');
echo '</form>';
template_draw('footer');

# ---
# All the different steps of the process

function step_whowilluseit($data) { ?>
<h1>Usage</h1>

<p>Who will use it? What is the target audience?
<br><? textarea($data, 'benefits') ?></p>

<?
}

function step_solves($data) { ?>
<h1>Solution</h1>

<p>What problem does it solve?
<br><? textarea($data, 'evidence') ?></p>

<p>How <em>exactly</em> does it make &pound;10k?
<br><? textarea($data, 'research') ?></p>

<?
}


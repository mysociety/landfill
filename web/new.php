<?

include_once '../conf/general';
include_once 'fns.php';
include_once '../../phplib/utility.php';
include_once '../../phplib/validate.php';
include_once '../../phplib/template.php';
include_once '../../phplib/auth.php';
include_once '../../phplib/rabx.php';
include_once '../../phplib/db.php';

template_set_style('../templates');
template_draw('header');
echo '<form action="/new" method="post">';

$data = array();
if ($token = get_http_var('t')) {
	$id = auth_token_retrieve('ideasbank', $token);
	if ($id) {
		$data = idea_load($id);
		$_POST['tostep'] = 'Load';
	}
}

if (isset($data['step']) && $data['step']=='done') {
	# Clicked a token for a then submitted idea
	echo '<h1>Already logged</h1> <p>Your idea has already been logged.</p>';
	$current_step = 'done';
} elseif (get_http_var('tostep')) {
	list($current_step, $next_step) = idea_submitted($data);
} else {
	step_intro();
	$current_step = 'intro';
	$next_step = '';
}

echo '<input type="hidden" name="step" value="', $current_step, '">';
if ($current_step != 'done') {
	if ($current_step != 'intro') {
		echo ' <input type="submit" name="tostep" value="Back">';
	}
	if ($next_step == 'done') {
	    echo ' <input type="submit" name="tostep" value="Submit">';
	} else {
	    echo ' <input type="submit" name="tostep" value="Next">';
	}
	if ($current_step != 'intro' && $current_step != 'department' && $current_step != 'user') {
		echo ' <input type="submit" name="tostep" value="Save">';
	}
}
echo '</form>';
template_draw('footer');

# ---

function step_show_errors($errors) {
    if (count($errors)) {
        print '<div id="errors"><ul><li>';
        print join ('</li><li>', array_values($errors));
        print '</li></ul></div>';
    }
}

function idea_submitted($data = array()) {
    $errors = array();

    # Get the data, first from the POST, then from any stored data
    foreach (array_keys($_POST) as $field) {
        $data[$field] = get_http_var($field);
    }
    if (array_key_exists('data', $data)) {
        $alldata = unserialize(base64_decode($data['data']));
        unset($data['data']);
        $data = array_merge($alldata, $data);
    }

    # Basic check for which flow we're in, can't do much without that!
    if (!isset($data['step'])) {
        $errors['idea_type'] = 'Lost what step you were on!';
    }
    if (!isset($data['idea_type']) || $data['idea_type']<1 || $data['idea_type']>5) {
        $errors['idea_type'] = 'You must select one of the options';
    }
    if (count($errors)) {
        step_show_errors($errors);
        step_intro();
	return array('intro', '');
    }

    # Okay, we know which flow we're in. Work out the steps and fields, and default them all if missing
    if ($data['idea_type']==1 || $data['idea_type']==2) {
        $next_steps = array('', 'intro', 'department', 'done');
    } elseif ($data['idea_type']==3) {
	$next_steps = array('', 'intro', 'user', 'basics', 'benefits', 'wherefits', 'timing', 'done');
    } elseif ($data['idea_type']==4 || $data['idea_type']==5) {
	$next_steps = array('', 'intro', 'user', 'basics', 'wherefits', 'evidence', 'datasource', 'cost', 'timing', 'webstats', 'otherbenefits', 'sponsorship', 'done');
    }

    if ($data['tostep'] == 'Save') {
        $data['id'] = idea_save($data);
	$token = auth_token_store('ideasbank', $data['id']);
	db_commit();
	$url = 'http://www.guardianideabank.co.uk/new/' . $token;
	echo '<div id="note">Your current submission has been saved. To start editing
	again, simply visit this unique and private URL:
	<a href="', $url, '">', $url, '</a></div>';
    }

    if (count($data))
        echo '<input type="hidden" name="data" value="', base64_encode(serialize($data)), '">';

    $index = array_search($data['step'], $next_steps);
    if (!$index) {
        # ILLEGAL STEP SUPPLIED
        return;
    } else {
        if ($data['tostep'] == 'Next' || $data['tostep'] == 'Submit') {
	    $errors = step_error_check($data);
	    if (count($errors)) {
	    	step_show_errors($errors);
            	$now_step = $next_steps[$index];
            	$next_step = $next_steps[$index+1];
	    } else {
            	$now_step = $next_steps[$index+1];
            	$next_step = isset($next_steps[$index+2]) ? $next_steps[$index+2] : null;
	    }
        } elseif ($data['tostep'] == 'Back') {
            $now_step = $next_steps[$index-1];
            $next_step = $next_steps[$index];
        } elseif ($data['tostep'] == 'Save' || $data['tostep'] == 'Load') {
            $now_step = $data['step'];
            $next_step = $next_steps[$index+1];
	}
        call_user_func('step_' . $now_step, $data);
    }

    #print '<pre style="position:absolute; bottom: 5px; right: 5px; border: solid 1px #999999; padding: 5px;">Dev' . "\n"; print_r($data); print '</pre>';
    return array($now_step, $next_step);
}

# ---
# All the different steps of the process

function step_intro($data = array()) {
	echo '<p><strong>Please select one of the following options:</strong></p> <p>';
	radio_boxes('idea_type', $data, array(
		1 => 'My idea is a minor change to an existing product or section.',
		2 => 'There is a component that exists elsewhere on a GNM site that I would like to reuse.',
		3 => 'My idea is an addition to an existing service which does not require any new data or content.',
		4 => 'My idea is an addition to an existing service which does require new data or content.',
		5 => 'My idea is an entirely new product.',
	), true);
	echo '<p>If you do not know which of the options best suits your idea, please email your departmental representative.</p>';
}

function step_department($data) { ?>
<h1>Minor change</h1>

<p>Your idea is probably a minor change that can be submitted directly to the
production support team for a quick implementation by your super user. This
should be discussed with your super user directly.</p>

<p>If you do not know who your representative is, then select your department below:
<br><? select_box($data, 'department', array( 'Editorial', 'Commercial', 'Technology Group', 'Research & Insight' )) ?>
</p>

<?
}

function step_user($data) {
	$name = isset($data['name']) ? htmlspecialchars($data['name']) : '';
	$email = isset($data['email']) ? htmlspecialchars($data['email']) : '';
?>
<h1>Who are you?</h1>

<p>First we need to know who you are and where you work so that your
representative can contact you to take the idea forward.</p>

<p>What&rsquo;s your name?
<br><input type="text" name="name" value="<?=$name?>"></p>

<p>What&rsquo;s your email address?
<br><input type="text" name="email" value="<?=$email?>"></p>

<p>What department are you in?
<br><? select_box($data, 'department', array( 'Editorial', 'Commercial', 'Technology Group', 'Research & Insight' )) ?></p>

<?
}

function step_basics($data) { 
	$title = isset($data['title']) ? htmlspecialchars($data['title']) : '';
?>
<h1>Describe your idea</h1>

<p>Provide a title for your idea
<br><input type="text" name="title" value="<?=$title?>"></p>

<p>Describe your idea
<br><? textarea($data, 'description') ?></p>

<?
}

function step_benefits($data) { ?>
<h1>Benefits</h1>

<p>Describe why this is a good idea
<br><? textarea($data, 'benefits') ?></p>

<?
}

function step_wherefits($data) { ?>
<h1>Where it fits</h1>

<p>Which product is the change for...</p>

<p>If this is a change to an editorial site or section of guardian.co.uk, which one is it?
<br><? select_box($data, 'change_section', array( 'Not applicable', 'News', 'Sport',
'Life & Style', 'Culture', 'Business', 'Money', 'Travel', 'Environment', 'Jobs', ))
?>
</p>

<?
}

function step_evidence($data) { ?>
<h1>Evidence</h1>

<p>Please provide any URLs to products, sites or research which help to back up your idea.
These can be competitive services or partner products, or simply user requests or evidence for
adoption. Add commentary if required.
<br><? textarea($data, 'evidence') ?></p>

<p>Is there any other research you have done into how your idea could be implemented?
This can be internal or external, come from a supplier or a user anecdote or feedback.
<br><? textarea($data, 'research') ?></p>

<?
}

function step_datasource($data) { ?>
<h1>Data source</h1>
<p>Does the data or content already exist on the Guardian website?
<br><? radio_boxes('data_already', $data, array(1 => 'Yes', 0 => 'No')); ?>
</p>

<p>If you know where the data or content would come from then record it here:
<br><? textarea($data, 'data_source') ?></p>

<?
}

function step_cost($data) { ?>
<h1>Cost of ownership</h1>

<p>What are the cost implications beyond development, e.g. editorial effort?
<br><? textarea($data, 'cost_implications') ?></p>

<p>Is this a one-off idea, or will it require future phases?
<br><? textarea($data, 'cost_future') ?></p>

<?
}

function step_timing($data) { ?>
<h1>Timing constraints</h1>

<p>Does your idea have any timing constraints?
<br><? radio_boxes('timing_constraint', $data, array(1 => 'Yes', 0 => 'No')); ?>
</p>

<div id="constrained_questions"<? if (!isset($data['timing_constraint']) || !$data['timing_constraint']) echo ' class="jshide"'; ?>>

<p>Why is the timing constrained?
<? radio_boxes('timing_why', $data, timing_why(), true); ?>
</p>

<p>What date does your idea need to be live by?
<br><input type="text" name="timing_live" value="">
</p>

<p>How fixed is this date?
<? radio_boxes('date_fixed', $data, date_fixed(), true); ?>
</p>

<p>Any further comments about the timing or date:
<br><? textarea($data, 'timing_comments') ?></p>

</div>

<?
}

function step_webstats($data) {
?>
<h1>Web statistics</h1>

<p>Which of the following key web statistics will your idea contribute to?
Please select one or more of the following options. For each benefit selected, provide a
description of how the benefit is achieved, and quantify it if you can.

<ul class="ticktextareas">
<?
grouped_checkbox($data, 'stats', 'unique', 'Unique Users');
textarea($data, 'webstats_text[unique]'); echo '</div>';
grouped_checkbox($data, 'stats', 'visits', 'Visits');
textarea($data, 'webstats_text[visits]'); echo '</div>';
grouped_checkbox($data, 'stats', 'pageviews', 'Page Views (impressions)');
textarea($data, 'webstats_text[pageviews]'); echo '</div>';
grouped_checkbox($data, 'stats', 'viewspervisit', 'Page Views per Visit');
textarea($data, 'webstats_text[viewspervisit]'); echo '</div>';
grouped_checkbox($data, 'stats', 'visitsperuser', 'Visits per User');
textarea($data, 'webstats_text[visitsperuser]'); echo '</div>';
grouped_checkbox($data, 'stats', 'bouncerate', 'Bounce Rate');
textarea($data, 'webstats_text[bouncerate]'); echo '</div>';
grouped_checkbox($data, 'stats', 'exitrate', 'Exit Rate');
textarea($data, 'webstats_text[exitrate]'); echo '</div>';
grouped_checkbox($data, 'stats', 'averagedwell', 'Average Dwell Time');
textarea($data, 'webstats_text[averagedwell]'); echo '</div>';
?>
</ul>
<?
}

function step_otherbenefits($data) { ?>
<h1>Other benefits</h1>

<p>Which of the following benefits will your idea contribute to?
Please select one or more of the following options. For each benefit selected,
provide a description of how the benefit is achieved, and quanitfy it if you can.

<ul class="ticktextareas">
<?
grouped_checkbox($data, 'benefits_tick', 'revenue', 'Direct increase in revenue');
textarea($data, 'benefits_text[revenue]'); echo '</div>';
grouped_checkbox($data, 'benefits_tick', 'efficiency', 'Efficiency improvement');
textarea($data, 'benefits_text[efficiency]'); echo '</div>';
grouped_checkbox($data, 'benefits_tick', 'strategic', 'Contributes to a strategic initiative');
textarea($data, 'benefits_text[strategic]'); echo '</div>';
grouped_checkbox($data, 'benefits_tick', 'award', 'Aimed at an award');
textarea($data, 'benefits_text[award]'); echo '</div>';
grouped_checkbox($data, 'benefits_tick', 'newspaper', 'Part of a newspaper-led initiative');
textarea($data, 'benefits_text[newspaper]'); echo '</div>';
grouped_checkbox($data, 'benefits_tick', 'competitor', 'A competitor has it, or plans to have it');
textarea($data, 'benefits_text[competitor]'); echo '</div>';
grouped_checkbox($data, 'benefits_tick', 'other', 'Other');
textarea($data, 'benefits_text[other]'); echo '</div>';
?>
</ul>

<h2>Benefits research</h2>

<p>Please select any source of research that has been used to validate the expected benefits
and comment on any findings.

<ul class="ticktextareas">
<?
grouped_checkbox($data, 'benefits_research', 'hitbox', 'Hitbox');
textarea($data, 'benefits_research_text[hitbox]'); echo '</div>';
grouped_checkbox($data, 'benefits_research', 'randi', 'Research and Insight');
textarea($data, 'benefits_research_text[randi]'); echo '</div>';
grouped_checkbox($data, 'benefits_research', 'market', 'Market Research');
textarea($data, 'benefits_research_text[market]'); echo '</div>';
grouped_checkbox($data, 'benefits_research', 'other', 'Other');
textarea($data, 'benefits_research_text[other]'); echo '</div>';
?>
</ul>
<?
}

function step_sponsorship($data) { ?>
<h1>Sponsorship</h1>

<p>Provide the names of anyone who has been involved in this idea and supports it.
<br><? textarea($data, 'sponsorship') ?></p>

<?
}

function step_done($data) {
	if ($data['idea_type']==1 || $data['idea_type']==2) {
		print '<h1>Departmental representatives</h1>';
		print '<p>Your representatives are:</p>';
		print '<ul><li>XXX Need data from Guardian to fill this in!</li></ul>';
		return;
	}

	$data['step'] = 'done'; # So we know this is a finished entry
	idea_save($data);
	db_commit();
	print '<h1>Thank you</h1>';
	print '<p>Your idea has been logged and your departmental representative will be in touch.</p>';
}

# ---
# Error checking for each step

function step_error_check($data) {
	$step = $data['step'];
	$errors = array();
	if ($step == 'user') {
		if (!$data['name']) $errors[] = 'Please give your name';
		if (!$data['email']) $errors['email'] = 'Please give your email address';
		elseif (!validate_email($data['email'])) $errors['email'] = 'Please give a valid email address';
		if (!$data['department']) $errors[] = 'Please give your department';
	} elseif ($step == 'department') {
		if (!$data['department']) $errors[] = 'Please give your department';
	} elseif ($step == 'basics') {
		if (!$data['title']) $errors[] = 'Please give your idea a title';
		if (!$data['description']) $errors[] = 'Please give a description';
	} elseif ($step == 'benefits') {
		if (!$data['benefits']) $errors[] = 'Please give the benefits of your idea';
	} elseif ($step == 'wherefits') {
	} elseif ($step == 'evidence') {
	} elseif ($step == 'evidence') {
	} elseif ($step == 'cost') {
	} elseif ($step == 'timing') {
		if (!isset($data['timing_constraint'])) $errors[] = 'Please say if there are timing constraints or not';
	} elseif ($step == 'datasource') {
		if (!isset($data['data_already'])) $errors[] = 'Please say if the data already exists or not';
	} elseif ($step == 'webstats') {
	} elseif ($step == 'otherbenefits') {
	} elseif ($step == 'sponsorship') {
	}
	return $errors;
}

# ---
# HTML display utility functions

function radio_boxes($base, $data, $options, $nl = false) {
	if ($nl) echo '<ul id="', $base, '">';
	foreach ($options as $key => $value) {
		$id = $base . '_' . $key;
		if ($nl) echo '<li>';
		echo '<input type="radio"';
		if (isset($data[$base]) && $data[$base]==$key) echo ' checked';
		echo ' name="', $base, '" id="', $id, '" value="', $key, '"> <label for="', $id, '">', $value, "</label>\n";
	}
	if ($nl) echo '</ul>';
}

function grouped_checkbox($data, $name, $value, $label) {
    $id = $name . '_' . $value;
    echo '<li><input type="checkbox" id="', $id, '" name="', $name, '[]"';
    if (isset($data[$name]) && is_array($data[$name]) && in_array($value, $data[$name])) echo ' checked';
    echo ' value="', $value, '"> <label for="', $id, '">', $label, '</label> <div';
    if (!isset($data[$name])
        || (isset($data[$name]) && !is_array($data[$name]))
        || (isset($data[$name]) && is_array($data[$name]) && !in_array($value, $data[$name]))) echo ' class="jshide"';
    echo '>';
}

function select_box($data, $name, $arr) {
	echo '<select name="', $name, '">';
	foreach ($arr as $opt) {
		echo '<option';
		if (isset($data[$name]) && $data[$name] == $opt) echo ' selected';
		echo '>', htmlspecialchars($opt);
	}
	echo '</select>';
}

function textarea($data, $name) {
	if (preg_match('#^(.*?)\[(.*?)\]$#', $name, $m)) {
		$value = isset($data[$m[1]][$m[2]]) ? htmlspecialchars($data[$m[1]][$m[2]]) : '';
	} else {
		$value = isset($data[$name]) ? htmlspecialchars($data[$name]) : '';
	}
	echo '<textarea cols="50" rows="10" name="', $name, '">', $value, '</textarea>';
}

# ---
# Database functions

function idea_load($id) {
	$q = db_query('SELECT * FROM idea WHERE id = ?', array($id));
        $data = pg_fetch_assoc($q);
	if (!$data) return null;

	$data['step'] = $data['saved_at']; unset($data['saved_at']);

	# Deal with booleans
	$data['data_already'] = $data['data_already']=='t' ? '1' : ($data['data_already']=='f' ? '0' : null);
	$data['timing_constraint'] = $data['timing_constraint']=='t' ? '1' : ($data['timing_constraint']=='f' ? '0' : null);

	# Unconcatenate all the tickbox explanation textareas
	foreach (array('stats', 'webstats_text', 'benefits_tick', 'benefits_text', 'benefits_research', 'benefits_research_text') as $col) {
		if ($data[$col]) $data[$col] = unserialize($data[$col]);
	}

	return $data;
}

function idea_save($data) {
	$created = isset($data['created']) ? $data['created'] : 'now';

	# Easier just to delete and re-insert
	if (isset($data['id']) && $data['id']) {
		$id = $data['id'];
		db_query('DELETE FROM idea WHERE id=?', array($id));
	} else {
        	$id = db_getOne("select nextval('idea_id_seq')");
	}
	
	$fields = array(
	    'name'=>'', 'email'=>'', 'department'=>'',
	    'title'=>'', 'description'=>'',
	    'benefits'=>'',
	    'change_section'=>'',
	    'evidence'=>'', 'research'=>'',
	    'data_already'=>null, 'data_source'=>'',
	    'cost_implications'=>'', 'cost_future'=>'',
	    'timing_constraint'=>null, 'timing_why'=>0, 'timing_live'=>'', 'date_fixed'=>0, 'timing_comments'=>'',
	    'stats'=>null, 'webstats_text'=>null,
	    'benefits_tick'=>null, 'benefits_text'=>null, 'benefits_research'=>null, 'benefits_research_text' => null,
	    'sponsorship'=>'',
	);
	foreach ($fields as $field => $default) {
		if (!isset($data[$field])) $data[$field] = $default;
	}

	if ($data['idea_type'] == 3) {
		db_query('INSERT INTO idea (
			id, created, modified, idea_type, saved_at,
			name, email, department,
			title, description,
			benefits,
			change_section,
			timing_constraint, timing_why, timing_live, date_fixed, timing_comments
		) VALUES (
			?, ?, current_timestamp, ?, ?,
			?, ?, ?,
			?, ?,
			?,
			?,
			?, ?, ?, ?, ?
		)', array(
			$id, $created, $data['idea_type'], $data['step'],
			$data['name'], $data['email'], $data['department'],
			$data['title'], $data['description'],
			$data['benefits'],
			$data['change_section'],
			$data['timing_constraint'], $data['timing_why'], $data['timing_live'], $data['date_fixed'], $data['timing_comments']
		));
	} elseif ($data['idea_type'] == 4 || $data['idea_type'] == 5) {

		# Concatenate all the tickbox explanation textareas
		foreach (array('stats', 'webstats_text', 'benefits_tick', 'benefits_text', 'benefits_research', 'benefits_research_text') as $col) {
			$data[$col] = serialize($data[$col]);
		}
		#print "<!-- "; print_r($data); print " -->";

		db_query('INSERT INTO idea (
			id, created, modified, idea_type, saved_at,
			name, email, department,
			title, description,
			change_section,
			evidence, research,
			data_already, data_source,
			cost_implications, cost_future,
			timing_constraint, timing_why, timing_live, date_fixed, timing_comments,
			stats, webstats_text,
			benefits_tick, benefits_text, benefits_research, benefits_research_text,
			sponsorship
		) VALUES (
			?, ?, current_timestamp, ?, ?,
			?, ?, ?,
			?, ?,
			?,
			?, ?,
			?, ?, 
			?, ?,
			?, ?, ?, ?, ?,
			?, ?,
			?, ?, ?, ?,
			?
		)', array(
			$id, $created, $data['idea_type'], $data['step'],
			$data['name'], $data['email'], $data['department'],
			$data['title'], $data['description'],
			$data['change_section'],
			$data['evidence'], $data['research'],
			$data['data_already'], $data['data_source'],
			$data['cost_implications'], $data['cost_future'],
			$data['timing_constraint'], $data['timing_why'], $data['timing_live'], $data['date_fixed'], $data['timing_comments'],
			$data['stats'], $data['webstats_text'],
			$data['benefits_tick'], $data['benefits_text'], $data['benefits_research'], $data['benefits_research_text'],
			$data['sponsorship'],
		));
	}
	return $id;
}

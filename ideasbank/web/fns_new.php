<?

$_dir = dirname(__FILE__);
include_once $_dir . '/../../phplib/validate.php';
include_once $_dir . '/../../phplib/utility.php';
include_once $_dir . '/../../phplib/db.php';
include_once $_dir . '/../../phplib/auth.php';
include_once $_dir . '/../../phplib/rabx.php';

function idea_new_main($type = '') {
	$data = array();
	if ($token = get_http_var('t')) {
		$id = auth_token_retrieve('ideasbank', $token);
		if ($id) {
			$data = idea_load($id);
			$_POST['tostep'] = 'Load';
		}
	}

	if ($type == 'commercial') {
		$data['idea_type'] = 6;
		$first_step = 'user';
	} else {
		$first_step = 'intro';
	}

	if (isset($data['step']) && $data['step']=='done') {
		# Clicked a token for a then submitted idea
		echo '<h1>Already logged</h1> <p>Your idea has already been logged.</p>';
		$current_step = 'done';
	} elseif (get_http_var('tostep')) {
		list($current_step, $next_step) = idea_submitted($data, $type);
	} else {
		call_user_func('step_' . $first_step, $data);
		$current_step = $first_step;
		$next_step = '';
	}

	echo '<input type="hidden" name="step" value="', $current_step, '">';
	if ($current_step != 'done') {
		if ($current_step != $first_step) {
			echo ' <input type="submit" name="tostep" value="Back">';
		}
		if ($next_step == 'done') {
		    echo ' <input type="submit" name="tostep" value="Submit idea">';
		} else {
		    echo ' <input type="submit" name="tostep" value="Next">';
		}
		if ($current_step != 'intro' && $current_step != 'department' && $current_step != 'user') {
			echo ' <input type="submit" name="tostep" value="Save current progress">';
		}
	}
}

function idea_submitted($data = array(), $type='') {
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
    if (!isset($data['idea_type']) || $data['idea_type']<1 || $data['idea_type']>6) {
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
    } elseif ($data['idea_type']==6) {
	$next_steps = array('', 'user', 'basics', 'whowilluseit', 'solves', 'done');
    }

    if ($data['tostep'] == 'Save current progress') {
        $data['id'] = idea_save($data);
	$token = auth_token_store('ideasbank', $data['id']);
	db_commit();
	$url = 'http://www.guardianideabank.co.uk/';
	if ($type == 'commercial') $url .= 'commercial/';
	$url .= 'new/' . $token;
	echo '<div id="note">Your progress with your current submission has been saved. To start editing
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
        if ($data['tostep'] == 'Next' || $data['tostep'] == 'Submit idea') {
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
        } elseif ($data['tostep'] == 'Save current progress' || $data['tostep'] == 'Load') {
            $now_step = $data['step'];
            $next_step = $next_steps[$index+1];
	}
        call_user_func('step_' . $now_step, $data);
    }

    #print '<pre style="position:absolute; bottom: 5px; right: 5px; border: solid 1px #999999; padding: 5px;">Dev' . "\n"; print_r($data); print '</pre>';
    return array($now_step, $next_step);
}

# ---
# Shared creation functions

function step_show_errors($errors) {
    if (count($errors)) {
        print '<div id="errors"><ul><li>';
        print join ('</li><li>', array_values($errors));
        print '</li></ul></div>';
    }
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
	} elseif ($step == 'cost') {
	} elseif ($step == 'timing') {
		if (!isset($data['timing_constraint'])) $errors[] = 'Please say if there are timing constraints or not';
	} elseif ($step == 'datasource') {
		if (!isset($data['data_already'])) $errors[] = 'Please say if the data already exists or not';
	} elseif ($step == 'webstats') {
	} elseif ($step == 'otherbenefits') {
	} elseif ($step == 'sponsorship') {
	} elseif ($step == 'whowilluseit') {
		if (!$data['benefits']) $errors[] = 'Please answer the questions below.';
	} elseif ($step == 'solves') {
		if (!$data['evidence']) $errors[] = 'Please state what problem your idea solves.';
		if (!$data['research']) $errors[] = 'Please give details on how your idea makes &pound;10k.';
	}
	return $errors;
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
	} elseif ($data['idea_type'] == 6) {
		db_query("INSERT INTO idea (
			id, created, modified, idea_type, saved_at,
			name, email, department,
			title, description,
			benefits, evidence, research
		) VALUES (
			?, ?, current_timestamp, ?, ?,
			?, ?, ?,
			?, ?,
			?, ?, ?
		)", array(
			$id, $created, $data['idea_type'], $data['step'],
			$data['name'], $data['email'], $data['department'],
			$data['title'], $data['description'],
			$data['benefits'], $data['evidence'], $data['research'],
		));
	}
	return $id;
}

# ---
# Shared step functions

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



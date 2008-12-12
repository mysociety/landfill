<?

include_once '../conf/general';
include_once 'fns.php';
include_once '../../phplib/utility.php';
include_once '../../phplib/template.php';
include_once '../../phplib/db.php';

template_set_style('templates');
template_draw('header');
echo '<h1>Logged ideas</h1>';

$id = intval(get_http_var('id'));

if ($id) {
    view_id($id);
} else {
    view_all();
}

template_draw('footer');

# ---

function view_id($id) {
    $idea = db_getRow('select * from idea where id=?', array($id));
    if (!$idea) {
        echo '<p>No data for that ID.</p>'; return;
    }
    $data_already = $idea['data_already'] == 't' ? 'Yes'
        : ($idea['data_already'] == 'f' ? 'No' : '');
    foreach (array('stats', 'webstats_text', 'benefits_tick', 'benefits_text', 'benefits_research', 'benefits_research_text') as $col) {
        if ($idea[$col]) $idea[$col] = unserialize($idea[$col]);
    }

    echo '<ul>';
    if ($idea['saved_at'] != 'done') {
        echo '<li><em>Idea submission in progress, not complete!</em></li>';
    }
    echo '<li>Created by <a href="mailto:', $idea['email'], '">', $idea['name'], '</a>, ', $idea['department'];
    if ($idea['idea_type'] == 3) {
        echo '<li>3 week submission (type 3: addition to existing service, no new data/content)</li>';
    } else {
        echo '<li>3 month submission (type ', $idea['idea_type'], ': ';
	if ($idea['idea_type']==4) {
	    echo 'addition to existing service, new data/content)';
	} else {
	    echo 'entirely new product)';
	}
    }
    echo '</ul>';
    echo '<dl>';
    if ($idea['title']) echo '<dt>Title</dt> <dd>', $idea['title'];
    $desc = preg_replace('#(\r?\n){2,}#', '</p> <p>', $idea['description']);
    if ($desc) echo '<dt>Description</dt> <dd><p>', $desc, '</p></dd>';
    if ($idea['idea_type'] == 3) {
	if ($idea['benefits']) echo '<dt>Benefits</dt> <dd>', $idea['benefits'];
	if ($idea['change_section']) echo '<dt>Section</dt> <dd>', $idea['change_section'];
	view_timing($idea);
    } else {
	if ($idea['change_section']) echo '<dt>Section</dt> <dd>', $idea['change_section'];
	if ($idea['evidence']) echo '<dt>Evidence</dt> <dd>', $idea['evidence'];
	if ($idea['research']) echo '<dt>Research</dt> <dd>', $idea['research'];
	if ($data_already) {
	    echo '<dt>Data already</dt> <dd>', $data_already;
	    if ($idea['data_source']) echo '<dt>Data sources</dt> <dd>', $idea['data_source'];
	}
	if ($idea['cost_implications']) echo '<dt>Cost implications</dt> <dd>', $idea['cost_implications'];
	if ($idea['cost_future']) echo '<dt>Cost futures</dt> <dd>', $idea['cost_future'];
	view_timing($idea);
	echo '</dl>';
	if (count($idea['stats'])) {
	    echo '<h2>Web statistics</h2> <dl>';
	    foreach ($idea['stats'] as $pt) {
	        echo "<dt>$pt</dt> <dd>", $idea['webstats_text'][$pt];
	    }
	    echo '</dl>';
	}
	if (count($idea['benefits_tick'])) {
	    echo '<h2>Other benefits</h2> <dl>';
	    foreach ($idea['benefits_tick'] as $pt) {
	        echo "<dt>$pt</dt> <dd>", $idea['benefits_text'][$pt];
	    }
	    echo '</dl>';
	}
	if (count($idea['benefits_research'])) {
	    echo '<h2>Benefits research</h2> <dl>';
	    foreach ($idea['benefits_research'] as $pt) {
	        echo "<dt>$pt</dt> <dd>", $idea['benefits_research_text'][$pt];
	    }
	    echo '</dl>';
	}
	if ($idea['sponsorship']) echo '<dt>Sponsorship</dt> <dd>', $idea['sponsorship'];

    }
}

function view_timing($idea) {
    $timing_constraint = $idea['timing_constraint'] == 't' ? 'Yes'
        : ($idea['timing_constraint'] == 'f' ? 'No' : '');
    if ($timing_constraint) {
	echo '<dt>Timing constrained</dt> <dd>', $timing_constraint;
	if ($timing_constraint == 'Yes') {
	    echo '<dl>';
	    $timing_why = timing_why($idea['timing_why']);
	    $date_fixed = date_fixed($idea['date_fixed']);
	    if ($timing_why) echo '<dt>Why</dt> <dd>', $timing_why;
	    if ($idea['timing_live']) echo '<dt>Live</dt> <dd>', $idea['timing_live'];
	    if ($date_fixed) echo '<dt>Date fixed</dt> <dd>', $date_fixed;
	    if ($idea['timing_comments']) echo '<dt>Comments</dt> <dd>', $idea['timing_comments'];
	    echo '</dl>';
	}
    }
}

function view_all() {
    $ideas = db_getAll("select * from idea where saved_at='done' order by id");
    echo '<table cellpadding="5" cellspacing="0" border="0">';
    echo '<tr><th>ID</th><th>Title</th><th>Type</th><th>Raised by</th><th>Department</th><th>Submitted</th></tr>';
    foreach ($ideas as $idea) {
        $modified = preg_replace('#\..*#', '', $idea['modified']);
	if ($idea['idea_type'] == 5) {
	    $idea_type = '3 month; entirely new';
	} elseif ($idea['idea_type'] == 4) {
	    $idea_type = '3 month; addition to existing with new data/content';
	} elseif ($idea['idea_type'] == 3) {
	    $idea_type = '3 week; addition to existing with no new data/content';
	}
        echo '<tr><td><a href="./view/', $idea['id'], '">', $idea['id'], '</a></td><td>', $idea['title'], '</td><td>', $idea_type, '</td><td><a href="mailto:', $idea['email'], '">', $idea['name'], '</a></td><td>', $idea['department'], '</td><td>', $modified, '</td></tr>';
    }
    echo '</table>';
}

# ---


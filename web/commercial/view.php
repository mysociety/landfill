<?

include_once '../../conf/general';
include_once '../fns.php';
include_once '../../../phplib/utility.php';
include_once '../../../phplib/template.php';
include_once '../../../phplib/db.php';

template_set_style('../../templates');
template_draw('header-commercial');
echo '<h1>Logged ideas</h1>';

$id = intval(get_http_var('id'));

if ($id) {
    view_id($id);
} else {
    view_all(array(6));
}

template_draw('footer');

# ---

function view_id($id) {
    $idea = db_getRow('select * from idea where id=?', array($id));
    if (!$idea) {
        echo '<p>No data for that ID.</p>'; return;
    }

    echo '<ul>';
    if ($idea['saved_at'] != 'done') {
        echo '<li><em>Idea submission in progress, not complete!</em></li>';
    }
    echo '<li>Created by <a href="mailto:', $idea['email'], '">', $idea['name'], '</a>, ', $idea['department'];
    echo '</ul>';
    echo '<dl>';
    if ($idea['title']) echo '<dt>Title</dt> <dd>', $idea['title'];
    $desc = preg_replace('#(\r?\n){2,}#', '</p> <p>', $idea['description']);
    if ($desc) echo '<dt>Description</dt> <dd><p>', $desc, '</p></dd>';
    if ($idea['benefits']) echo '<dt>Who will use it? What is the target audience?</dt> <dd>', $idea['benefits'];
    if ($idea['evidence']) echo '<dt>What problem does it solve?</dt> <dd>', $idea['evidence'];
    if ($idea['research']) echo '<dt>How exactly does it make &pound;10k?</dt> <dd>', $idea['research'];
    echo '</dl>';
}


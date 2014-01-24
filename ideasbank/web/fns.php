<?

function date_fixed($opt = null) {
    $opts = array(
1 => 'Definitive and will not move',
2 => 'The earliest possible date',
3 => 'An estimated likely date',
    );
    if (is_null($opt)) return $opts;
    if (isset($opts[$opt])) return $opts[$opt];
    return '';
}

function timing_why($opt = null) {
    $opts = array(
1 => 'Your idea is associated with an event which is external to the Guardian, such as a sporting or political event',
2 => 'Your idea is associated with a Guardian event, such as a promotional launch or a strategic initiative',
3 => 'A competitor is launching an equivalent idea',
4 => 'Your idea is time critical for some other reason',
    );
    if (is_null($opt)) return $opts;
    if (isset($opts[$opt])) return $opts[$opt];
    return '';
}

function view_all($types) {
    $types = join(',', $types);
    $ideas = db_getAll("select * from idea where saved_at='done' and idea_type in ($types) order by id");
    print '<table cellpadding="5" cellspacing="0" border="0">';
    print '<tr><th>ID</th><th>Title</th>';
    if ($types != 6) print '<th>Type</th>';
    print '<th>Raised by</th><th>Department</th><th>Submitted</th></tr>';
    foreach ($ideas as $idea) {
        $modified = preg_replace('#\..*#', '', $idea['modified']);
        if ($idea['idea_type'] == 5) {
            $idea_type = '3 month; entirely new';
        } elseif ($idea['idea_type'] == 4) {
            $idea_type = '3 month; addition to existing with new data/content';
        } elseif ($idea['idea_type'] == 3) {
            $idea_type = '3 week; addition to existing with no new data/content';
        }
        print '<tr><td><a href="./view/' . $idea['id'] . '">' . $idea['id'] . '</a></td><td>' . $idea['title'] . '</td>';
        if ($types !=6) print '<td>' . $idea_type . '</td>';
        print '<td><a href="mailto:' . $idea['email'] . '">' . $idea['name'] . '</a></td><td>' . $idea['department'] . '</td><td>' . $modified . '</td></tr>';
    }
    echo '</table>';
}


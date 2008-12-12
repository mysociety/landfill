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



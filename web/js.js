$(document).ready(function(){

	$('#timing_constraint_1').click(function(){
		$('#constrained_questions').show('fast');
	});

	$('#timing_constraint_0').click(function(){
		$('#constrained_questions').hide('fast');
	});

	$('.ticktextareas input').click(function(){
		$(this).nextAll('div').toggle('fast');
	});
});

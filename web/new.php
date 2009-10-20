<?

include_once '../conf/general';
include_once 'fns.php';
include_once 'fns_new.php';
include_once '../../phplib/template.php';

template_set_style('../templates');
template_draw('header');
echo '<form action="/new" method="post">';
idea_new_main();
echo '</form>';
template_draw('footer');

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


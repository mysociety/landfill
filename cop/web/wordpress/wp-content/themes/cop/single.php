<?php
function indent_recommendations($content) {
    if (preg_match('/^<p>[1-9]\d*\./', $content)) {
        $content = preg_replace("/^<p>([1-9]\d*)\./i", "<ol start=\"\\1\"><li>", $content);
        $content = preg_replace("#</p>#i", "</li></ol>", $content);
    }
    return $content;
}
add_filter('the_content', 'indent_recommendations');
?>
<?php get_header(); ?>

	<div id="content" class="widecolumn" style="margin-left: 4%;">
				
  <?php if (have_posts()) : while (have_posts()) : the_post(); ?>
                <h2><?=the_title()?></h2>
		<div class="navigation">
			<?php next_post('&laquo; %', '', 'yes', 'yes', '', 2) ?>
                        |
			<?php previous_post('% &raquo;', '', 'yes', 'yes', '', 2) ?>
		</div>
	
		<div class="post" id="post-<?php the_ID(); ?>">
	
			<div class="entrytext">
				<?php the_content('<p class="serif">Read the rest of this entry &raquo;</p>'); ?>
	
				<?php link_pages('<p><strong>Pages:</strong> ', '</p>', 'number'); ?>
	
			</div>
                <p><a href="<?php the_permalink() ?>" rel="bookmark" title="Link to <?php the_title(); ?>">Link to this section</a></p>
		</div>
		
	<?php comments_template(); ?>
	
	<?php endwhile; else: ?>
	
		<p>Sorry, no posts matched your criteria.</p>
	
<?php endif; ?>
	
	</div>

<?php get_footer(); ?>

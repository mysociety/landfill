<?php
function indent_recommendations($content) {
    $content = preg_replace("/(\d+)\./i", "<ol start=\"\\1\"><li>", $content);
    $content .= '</li></ol>';
    return $content;
}
add_filter('the_content', 'indent_recommendations');
?>
<?php get_header(); ?>

	<div id="content" class="narrowcolumn">

	<?php if (have_posts()) : ?>
		
        <table>

		<?php while (have_posts()) : the_post(); ?>
            <tr>
                <td width="60%" valign="top">
				<!-- <small><?php the_time('F jS, Y') ?> by <?php the_author() ?> </small>-->
				
				<div class="entry">
					<?php the_content('Read the rest of this entry &raquo;'); ?>
				</div>
                </td>
                <td width="10%" valign="top">
                </td>
		
                <td width="30%" valign="top">
				<div class="postthoughts">
               <?php 
                    $comments = get_approved_comments($id);
                    if ($comments) {
               ?>
                <p>Some responses:
                <ul id="commentlist">
                <?php foreach ($comments as $comment) { ?>
                    <li id="comment-<?php comment_ID() ?>">
                    <?php comment_excerpt() ?>
                    </li>

                <?php } // end for each comment ?>
                </ul>
               <? }  ?>
                <p>
               <?
                    comments_popup_link('Leave a response to this &#187;', 'Leave a response and read more&#187;', 'Leave a response and read % &#187;', '', '');
               ?>
                </td>
            </tr>
	
		<?php endwhile; ?>
        </table>

		<div class="navigation">
			<div class="alignleft"><?php next_posts_link('Next page &raquo;') ?></div>
			<div class="alignright"><?php previous_posts_link('&laquo; Previous page') ?></div>
		</div>
		
	<?php else : ?>

		<h2 class="center">Not Found</h2>
		<p class="center">Sorry, but you are looking for something that isn't here.</p>
		<?php include (TEMPLATEPATH . "/searchform.php"); ?>

	<?php endif; ?>

	</div>

<?php get_footer(); ?>

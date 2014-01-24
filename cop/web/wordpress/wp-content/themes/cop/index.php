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

	<div id="content" class="narrowcolumn">

	<?php if (have_posts()) : ?>
		
        <table>

		<?php while (have_posts()) : the_post(); ?>
            <tr>
                <td width="2%" valign="top">
                                <div class="permalink"><p><a title="Link to this" href="#<?=$id?>">#</a></p></div>
                </td>
                <td width="59%" valign="top">
				<div class="entry">
                                <a name="<?=$id?>"></a>
					<?php the_content('Read the rest of this entry &raquo;'); ?>
				</div>
                </td>
                <td width="9%" valign="top">
                </td>
		
                <td width="30%" valign="top">
				<div class="postthoughts">
               <?php 
                    $comments = get_approved_comments($id);
                    if ($comments) {
               ?>
                <p>Some responses:</p>
                <ul id="commentlist">
                <?php foreach ($comments as $comment) { ?>
                    <li id="comment-<?php comment_ID() ?>">
                    <?php comment_excerpt() ?>
                    </li>

                <?php } // end for each comment ?>
                </ul>
               <? }  ?>
               <?
                    comments_popup_link('Respond &#187;', 'Leave a response and read more&#187;', 'Leave a response and read % &#187;', '', '');
               ?>
                                </div>
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

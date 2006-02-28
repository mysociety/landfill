<?php get_header(); ?>

	<div id="content" class="narrowcolumn">

	<?php if (have_posts()) : ?>
		
        <table>
		<?php while (have_posts()) : the_post(); ?>
            <tr>
                <td>
				<h2><?php the_title(); ?></h2>
                </td>
                <td>
                <h2>Your Thoughts</h2>
                </td>
            </tr>
            <tr>
                <td>
				<!-- <small><?php the_time('F jS, Y') ?> by <?php the_author() ?> </small>-->
				
				<div class="entry">
					<?php the_content('Read the rest of this entry &raquo;'); ?>
				</div>
                </td>
		
                <td width="30%" valign="top">
				<div class="postthoughts">
               <?php 
                    $comments = get_approved_comments($id);
                    if ($comments) {
               ?>
                <ul id="commentlist">
                <?php foreach ($comments as $comment) { ?>
                    <li id="comment-<?php comment_ID() ?>">
                    <?php comment_excerpt() ?>
                    </li>

                <?php } // end for each comment ?>
                </ul>
               <? } 
                    comments_popup_link('Add your thoughts &#187;', 'Show thought &#187;', 'Show % thoughts &#187;');
               ?>
                <p><a href="<?php the_permalink() ?>" rel="bookmark" title="Permanent Link to <?php the_title(); ?>">Link to this section</a></p>
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

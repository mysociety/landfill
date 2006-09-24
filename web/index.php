<? include_once('header.php');

if (isset($_SERVER['HTTP_REFERER']) && preg_match('#www\.dft\.gov\.uk#', $_SERVER['HTTP_REFERER'])) { ?>
<p>If you're looking for our 
<a href="http://www.mysociety.org/2006/travel-time-maps/">travel time maps</a>,
please visit <a href="http://www.mysociety.org/2006/travel-time-maps/">http://www.mysociety.org/2006/travel-time-maps/</a>.</p>
<? } ?>

<p>Welcome to mySociety.co.uk &mdash; the new not-for-profit trading arm of
charitable project <a href="http://www.mysociety.org/">mySociety.org</a>.
mySociety.co.uk sells licenced web service
products based on our award-winning civic websites, particularly focusing on
the local government and charitable sectors.</p>

<h2>Our Products</h2>

<p id="nav">
<a href="brokencivicinfrastructure/"><img class="t" src="i/thumb_bcim.png"></a>
<a href="pledgebank/"><img class="t" src="i/thumb_pb.png"></a>
<a href="writetothem/"><img class="t" src="i/thumb_wtt.png"></a>
<a href="hearfromyourcouncillor/"><img class="t" src="i/thumb_hyfc.png"></a>
<a href="giveitaway/"><img class="t" src="i/thumb_gia.jpg"></a>
</p>

<? include_once('footer.php'); ?>

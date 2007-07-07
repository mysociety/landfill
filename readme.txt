
this is technical detail about the graphing module.


All points are within a single <points> block.

Each point is within its own <point> block. It has a name,
which is what's displayed, a colour which is the colour of
the point, sized according to the value of pointsize.
for_matching is an optional field, not displayed, which can
be used for matching in data (in the below, it's the
constituency (equivalent to the US Congressional district)
name for that MP). There is a group to which that point
belongs (an optional text field, only displayed) and a link
which you get taken to on clicking the infomration about
the point.

Data is either a single number - a variable; or a sequence
of numbers (a sequence). All the entries in a sequence MUST
be prefixed by the letter 's'. This is removed prior to
display, and allows for numbers (e.g. years) to be used as
the index and be valid XML.


Variable information is included in a variables block. Each
variable has a name, which is how it is referred to in the
points. The maximum and minium values for that variable
(including any sequences) are also stored, along with unit
information and the title for display.


Any scaling of units (other than log scaling) is expected to
be done by the generator of the XML, and not the flash
applet. It displays what it is given.


<?xml version='1.0' standalone='yes'?>
<iquango>
  <points>
    <point>
      <name>Lembit &amp;Ouml;pik, Montgomeryshire</name>
      <colour>D2691E</colour>
      <for_matching>Montgomeryshire</for_matching>
      <group>LDem</group>
      <link>http://www.theyworkforyou.com/mp/?m=1720</link>
      <pointsize>2</pointsize>
      <sequences>
        <col1>
          <s2002>12894</s2002>
          <s2003>19022</s2003>
          <s2004>11933</s2004>
          <s2005>14026</s2005>
          <s2006>14685</s2006>
        </col1>
        <col1_rank>
          <s2002>491</s2002>
          <s2003>363</s2003>
          <s2004>539</s2004>
          <s2005>504</s2005>
        </col1_rank>
      <variables>
        <comments_on_speeches>22</comments_on_speeches>
        <comments_on_speeches_rank>39</comments_on_speeches_rank>
        <debate_sectionsspoken_inlastyear>51</debate_sectionsspoken_inlastyear>
        <writetothem_sent_2006>44</writetothem_sent_2006>
      </variables>
    </point>
    <point>
      <name>Diane Abbott, Hackney North &amp;amp; Stoke Newington</name>
      <colour>ff0000</colour>
      <for_matching>Hackney North &amp;amp; Stoke Newington</for_matching>
      <group>Lab</group>
      <link>http://www.theyworkforyou.com/mp/?m=1604</link>
      <pointsize>2</pointsize>
      <sequences>
        <col1>
          <s2002>0</s2002>
          <s2003>0</s2003>
          <s2004>0</s2004>
          <s2005>0</s2005>
          <s2006>0</s2006>
        </col1>
        <col1_rank>
          <s2002>615</s2002>
          <s2003>618</s2003>
          <s2004>611</s2004>
          <s2005>611</s2005>
        </col1_rank>
      </sequences>
      <variables>
        <comments_on_speeches>5</comments_on_speeches>
        <comments_on_speeches_rank>242</comments_on_speeches_rank>
        <debate_sectionsspoken_inlastyear>18</debate_sectionsspoken_inlastyear>
        <debate_sectionsspoken_inlastyear_rank>407</debate_sectionsspoken_inlastyear_rank>
        <majority_in_seat>7427</majority_in_seat>
        <writetothem_sent_2006>302</writetothem_sent_2006>
      </variables>
    </point>
  </points>
  <variables>
    <variable>
      <name>col1</name>
      <max>21634</max>
      <min>0</min>
      <sequences>
        <s2002>2002</s2002>
        <s2003>2003</s2003>
        <s2004>2004</s2004>
        <s2005>2005</s2005>
        <s2006>2006</s2006>
      </sequences>
      <title>Additional Costs Allowance</title>
      <unit></unit>
    </variable>
    <variable>
      <name>col1_rank</name>
      <max>632</max>
      <min>0</min>
      <sequences>
        <s2002>2002</s2002>
        <s2003>2003</s2003>
        <s2004>2004</s2004>
        <s2005>2005</s2005>
        <s2006>2006</s2006>
      </sequences>
      <title>Additional Costs Allowance rank</title>
      <unit></unit>
    </variable>
    <variable>
      <name>writetothem_sent_2006</name>
      <max>822</max>
      <min>0</min>
      <title>writetothem sent 2006</title>
      <unit></unit>
    </variable>
    <variable>
      <name>majority_in_seat</name>
      <max>19519</max>
      <min>0</min>
      <title>majority in seat</title>
      <unit></unit>
    </variable>
    <variable>
      <name>debate_sectionsspoken_inlastyear</name>
      <max>351</max>
      <min>0</min>
      <title>debate sectionsspoken inlastyear</title>
      <unit></unit>
    </variable>
    <variable>
      <name>debate_sectionsspoken_inlastyear_rank</name>
      <max>632</max>
      <min>0</min>
      <title>debate sectionsspoken inlastyear rank</title>
      <unit></unit>
    </variable>
    <variable>
      <name>comments_on_speeches</name>
      <max>180</max>
      <min>0</min>
      <title>comments on speeches</title>
      <unit></unit>
    </variable>
    <variable>
      <name>comments_on_speeches_rank</name>
      <max>632</max>
      <min>0</min>
      <title>comments on speeches rank</title>
      <unit></unit>
    </variable>
  </variables>
</iquango>

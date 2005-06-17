CREATE TABLE `comments` (
  `commentid` int(10) unsigned NOT NULL auto_increment,
  `postid` int(10) unsigned NOT NULL default '0',
  `email` varchar(255) default NULL,
  `comment` text,
  `visible` tinyint(3) unsigned NOT NULL default '1',
  `posted` datetime default NULL,
  `istrackback` tinyint(3) unsigned NOT NULL default '0',
  `name` varchar(100) NOT NULL default '',
  PRIMARY KEY  (`commentid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;


--
-- Table structure for table `emailnotify`
--
CREATE TABLE `emailnotify` (
  `notifyid` int(10) unsigned NOT NULL auto_increment,
  `email` varchar(100) default NULL,
  `search` varchar(255) default NULL,
  `authcode` varchar(30) default NULL,
  `lastupdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `lastrun` timestamp NOT NULL default '0000-00-00 00:00:00',
  `validated` tinyint(3) unsigned NOT NULL default '0',
  `cancelled` tinyint(3) unsigned NOT NULL default '0',
  PRIMARY KEY  (`notifyid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `foranalysis`
--
CREATE TABLE `foranalysis` (
  `postid` int(10) unsigned NOT NULL auto_increment,
  `age` varchar(10) NOT NULL default '',
  `sex` varchar(6) NOT NULL default '',
  `region` varchar(9) default '',
  `evervoted` varchar(8) NOT NULL default '',
  `why` text,
  `nochildren` tinyint(4) NOT NULL default '-1',
  `postcode` varchar(8) NOT NULL default '',
  `posted` datetime default NULL,
  `title` varchar(100) NOT NULL default '',
  `commentcount` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`postid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;


--
-- Table structure for table `posts`
--

CREATE TABLE `posts` (
  `postid` int(10) unsigned NOT NULL auto_increment,
  `email` varchar(200) NOT NULL default '',
  `age` varchar(10) NOT NULL default '',
  `sex` varchar(6) NOT NULL default '',
  `region` varchar(9) default '',
  `evervoted` varchar(8) NOT NULL default '',
  `why` text,
  `nochildren` tinyint(4) NOT NULL default '-1',
  `postcode` varchar(8) NOT NULL default '',
  `hidden` tinyint(3) unsigned NOT NULL default '0',
  `validated` tinyint(3) unsigned NOT NULL default '0',
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `posted` datetime default NULL,
  `title` varchar(100) NOT NULL default '',
  `commentcount` int(10) unsigned NOT NULL default '0',
  `ethgroup` varchar(15) NOT NULL default '',
  `authcode` varchar(20) NOT NULL default '',
  `shortwhy` varchar(255) default '',
  `emailalert` tinyint(3) unsigned NOT NULL default '0',
  `interesting` tinyint(3) unsigned NOT NULL default '0',
  PRIMARY KEY  (`postid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `posts`
--

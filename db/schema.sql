
-- Host: localhost    Database: na
-- ------------------------------------------------------
-- Server version       4.0.20

--
-- Table structure for table `comments`
--

CREATE TABLE comments (
  commentid int(10) unsigned NOT NULL auto_increment,
  postid int(10) unsigned NOT NULL default '0',
  email varchar(255) default NULL,
  comment text,
  visible tinyint(3) unsigned NOT NULL default '1',
  posted datetime default NULL,
  istrackback tinyint(3) unsigned NOT NULL default '0',
  name varchar(100) NOT NULL default '',
  PRIMARY KEY  (commentid)
) TYPE=MyISAM;

--
-- Dumping data for table `comments`
--

--
-- Table structure for table `emailnotify`
--

CREATE TABLE emailnotify (
  notifyid int(10) unsigned NOT NULL auto_increment,
  email varchar(100) default NULL,
  search varchar(255) default NULL,
  authcode varchar(30) default NULL,
  lastupdated timestamp(14) NOT NULL,
  lastrun timestamp(14) NOT NULL default '00000000000000',
  validated tinyint(3) unsigned NOT NULL default '0',
  PRIMARY KEY  (notifyid)
) TYPE=MyISAM;

--
-- Dumping data for table `emailnotify`
--

--
-- Table structure for table `posts`
--

CREATE TABLE posts (
  postid int(10) unsigned NOT NULL auto_increment,
  email varchar(200) NOT NULL default '',
  age varchar(10) NOT NULL default '',
  sex varchar(6) NOT NULL default '',
  region varchar(9) default '',
  evervoted varchar(8) NOT NULL default '',
  why text,
  nochildren tinyint(4) NOT NULL default '-1',
  postcode varchar(8) NOT NULL default '',
  hidden tinyint(3) unsigned NOT NULL default '0',
  validated tinyint(3) unsigned NOT NULL default '0',
  timestamp timestamp(14) NOT NULL,
  posted datetime default NULL,
  title varchar(100) NOT NULL default '',
  commentcount int(10) unsigned NOT NULL default '0',
  ethgroup varchar(15) NOT NULL default '',
  authcode varchar(20) NOT NULL default '',
  shortwhy varchar(255) default '',
  PRIMARY KEY  (postid)
) TYPE=MyISAM;

--
-- Dumping data for table `posts`
--


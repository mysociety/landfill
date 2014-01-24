-- MySQL dump 10.10
--
-- Host: localhost    Database: bb
-- ------------------------------------------------------
-- Server version	5.0.21-log

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `abusereports`
--

DROP TABLE IF EXISTS `abusereports`;
CREATE TABLE `abusereports` (
  `reportid` int(10) unsigned NOT NULL auto_increment,
  `postid` int(10) unsigned default NULL,
  `commentid` int(10) unsigned default NULL,
  `reporter_name` varchar(100) NOT NULL,
  `email` varchar(100) NOT NULL,
  `message` text,
  `modified` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `added` timestamp NOT NULL default '0000-00-00 00:00:00',
  `processed` datetime default NULL,
  `open` tinyint(3) unsigned NOT NULL default '1',
  `reporter_ip` varchar(20) NOT NULL default '',
  `site` varchar(40) default NULL,
  PRIMARY KEY  (`reportid`),
  KEY `main` (`site`,`open`,`added`,`postid`,`commentid`,`reportid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `comments`
--

DROP TABLE IF EXISTS `comments`;
CREATE TABLE `comments` (
  `commentid` int(10) unsigned NOT NULL auto_increment,
  `postid` int(10) unsigned NOT NULL default '0',
  `email` varchar(255) default NULL,
  `comment` text,
  `visible` tinyint(3) unsigned NOT NULL default '1',
  `posted` datetime default NULL,
  `istrackback` tinyint(3) unsigned NOT NULL default '0',
  `name` varchar(100) NOT NULL default '',
  `site` varchar(40) NOT NULL,
  PRIMARY KEY  (`commentid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `emailnotify`
--

DROP TABLE IF EXISTS `emailnotify`;
CREATE TABLE `emailnotify` (
  `notifyid` int(10) unsigned NOT NULL auto_increment,
  `email` varchar(100) default NULL,
  `search` varchar(255) default NULL,
  `authcode` varchar(30) default NULL,
  `lastupdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `lastrun` timestamp NOT NULL default '0000-00-00 00:00:00',
  `validated` tinyint(3) unsigned NOT NULL default '0',
  `cancelled` tinyint(3) unsigned NOT NULL default '0',
  `site` varchar(40) NOT NULL,
  PRIMARY KEY  (`notifyid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `foranalysis`
--

--
-- Table structure for table `posts`
--

DROP TABLE IF EXISTS `posts`;
CREATE TABLE `posts` (
  `postid` int(10) unsigned NOT NULL auto_increment,
  `email` varchar(200) NOT NULL default '',
  `why` text,
  `hidden` tinyint(3) unsigned NOT NULL default '0',
  `validated` tinyint(3) unsigned NOT NULL default '0',
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `posted` datetime default NULL,
  `title` varchar(100) NOT NULL default '',
  `commentcount` int(10) unsigned NOT NULL default '0',
  `authcode` varchar(20) NOT NULL default '',
  `shortwhy` varchar(255) default '',
  `emailalert` tinyint(3) unsigned NOT NULL default '0',
  `interesting` tinyint(3) unsigned NOT NULL default '0',
  `site` varchar(40) NOT NULL,
  `q6` varchar(100) default NULL,
  `q5` varchar(100) default NULL,
  `q4` varchar(100) default NULL,
  `q3` varchar(100) default NULL,
  `q2` varchar(100) default NULL,
  `q1` varchar(100) default NULL,
  PRIMARY KEY  (`postid`),
  KEY `main` (`site`,`postid`,`validated`,`hidden`,`posted`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `similarity`
--

DROP TABLE IF EXISTS `similarity`;
CREATE TABLE `similarity` (
  `userid` int(11) NOT NULL default '0',
  `postid1` int(11) NOT NULL default '0',
  `postid2` int(11) NOT NULL default '0',
  `similarity` int(11) NOT NULL default '0',
  PRIMARY KEY  (`userid`,`postid1`,`postid2`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;


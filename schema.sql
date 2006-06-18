-- MySQL dump 10.10
--
-- Host: localhost    Database: panopticon
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
-- Table structure for table `entries`
--

DROP TABLE IF EXISTS `entries`;
CREATE TABLE `entries` (
  `entryid` int(10) unsigned NOT NULL auto_increment,
  `title` varchar(255) default NULL,
  `subject` varchar(255) default NULL,
  `link` text,
  `last_seen` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `first_seen` timestamp NOT NULL default '0000-00-00 00:00:00',
  `feedid` int(10) unsigned NOT NULL default '0',
  `pubdate` varchar(50) default NULL,
  `content` text,
  `trackback_url` varchar(255) default NULL,
  `shortcontent` text,
  `commentcount` int(10) unsigned NOT NULL default '0',
  `visible` tinyint(3) unsigned NOT NULL default '1',
  PRIMARY KEY  (`entryid`),
  KEY `feedid` (`feedid`),
  KEY `lastseen` (`last_seen`),
  KEY `feedpubentry` (`feedid`,`pubdate`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `feedinfo`
--

DROP TABLE IF EXISTS `feedinfo`;
CREATE TABLE `feedinfo` (
  `feedid` int(10) unsigned NOT NULL default '0',
  `siteurl` varchar(255) default NULL,
  `description` text,
  `title` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`feedid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `feeds`
--

DROP TABLE IF EXISTS `feeds`;
CREATE TABLE `feeds` (
  `feedid` int(10) unsigned NOT NULL auto_increment,
  `feedurl` varchar(255) NOT NULL default '',
  `last_attempt` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `last_successful_fetch` timestamp NOT NULL default '0000-00-00 00:00:00',
  `last_failed_fetch` timestamp NOT NULL default '0000-00-00 00:00:00',
  `reason` varchar(255) default NULL,
  `tag` varchar(20) default NULL,
  PRIMARY KEY  (`feedid`),
  KEY `tagindex` (`feedid`,`tag`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;


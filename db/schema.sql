--
-- schema.sql:
-- Schema for Placeopedia MySQL database.
-- Uses same db as YourHistoryHere
-- Dumped from very on 2005-09-10.
--
-- Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
-- Email: matthew@mysociety.org; WWW: http://www.mysociety.org/
--
-- $Id: schema.sql,v 1.6 2007-01-02 23:13:53 matthew Exp $
--

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

CREATE TABLE `cur` (
  `cur_id` int(8) unsigned NOT NULL auto_increment,
  `cur_namespace` tinyint(2) unsigned NOT NULL default '0',
  `cur_title` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `cur_text` mediumtext NOT NULL,
  `cur_comment` tinyblob NOT NULL,
  `cur_user` int(5) unsigned NOT NULL default '0',
  `cur_user_text` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `cur_timestamp` varchar(14) character set latin1 collate latin1_bin NOT NULL default '',
  `cur_restrictions` tinyblob NOT NULL,
  `cur_counter` bigint(20) unsigned NOT NULL default '0',
  `cur_is_redirect` tinyint(1) unsigned NOT NULL default '0',
  `cur_minor_edit` tinyint(1) unsigned NOT NULL default '0',
  `cur_is_new` tinyint(1) unsigned NOT NULL default '0',
  `cur_random` double unsigned NOT NULL default '0',
  `inverse_timestamp` varchar(14) character set latin1 collate latin1_bin NOT NULL default '',
  `cur_touched` varchar(14) character set latin1 collate latin1_bin NOT NULL default '',
  UNIQUE KEY `cur_id` (`cur_id`),
  UNIQUE KEY `name_title_dup_prevention` (`cur_namespace`,`cur_title`),
  KEY `cur_title` (`cur_title`),
  KEY `cur_timestamp` (`cur_timestamp`),
  KEY `cur_random` (`cur_random`),
  KEY `name_title_timestamp` (`cur_namespace`,`cur_title`,`inverse_timestamp`),
  KEY `user_timestamp` (`cur_user`,`inverse_timestamp`),
  KEY `usertext_timestamp` (`cur_user_text`,`inverse_timestamp`),
  KEY `jamesspecialpages` (`cur_is_redirect`,`cur_namespace`,`cur_title`,`cur_timestamp`),
  KEY `id_title_ns_red` (`cur_id`,`cur_title`,`cur_namespace`,`cur_is_redirect`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 PACK_KEYS=1;

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

CREATE TABLE `posts` (
  `postid` int(10) unsigned NOT NULL auto_increment,
  `email` varchar(200) NOT NULL default '',
  `region` varchar(9) default '',
  `why` text,
  `postcode` varchar(8) NOT NULL default '',
  `hidden` tinyint(3) unsigned NOT NULL default '0',
  `validated` tinyint(3) unsigned NOT NULL default '0',
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `posted` datetime default NULL,
  `title` varchar(100) NOT NULL default '',
  `commentcount` int(10) unsigned NOT NULL default '0',
  `authcode` varchar(20) NOT NULL default '',
  `shortwhy` text,
  `emailalert` tinyint(3) unsigned NOT NULL default '0',
  `interesting` tinyint(3) unsigned NOT NULL default '0',
  `original_geography` varchar(255) NOT NULL default '',
  `google_long` varchar(30) default NULL,
  `google_lat` varchar(30) default NULL,
  `google_zoom` tinyint(3) unsigned default NULL,
  `lat` double default NULL,
  `lon` double default NULL,
  `name` varchar(255) default NULL,
  `category` varchar(20) default NULL,
  `site` varchar(20) NOT NULL default 'missing',
  PRIMARY KEY  (`postid`),
  KEY `site` (`site`),
  KEY `posts_google_long_idx` (`google_long`),
  KEY `posts_google_lat_idx` (`google_lat`),
  KEY `posts_lat_idx` (`lat`),
  KEY `posts_lon_idx` (`lon`),
  KEY `latest_posts` (`site`,`validated`,`hidden`,`posted`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- Index of Wikipedia article titles. Generation is used to drop articles which
-- cease to exist.
CREATE TABLE `wikipedia_article` (
  `title` text collate utf8_bin NOT NULL,
  `generation` int(11) NOT NULL default '0',
  KEY `wikipedia_article2_title_idx` (`title`(256)),
  KEY `wikipedia_article2_generation_idx` (`generation`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_bin;

CREATE TABLE `incorrect` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `post_id` int(10) unsigned NOT NULL,
  `name` varchar(255) NOT NULL default '',
  `email` varchar(255) NOT NULL default '',
  `lat` double default NULL,
  `lon` double default NULL,
  `zoom` tinyint(3) unsigned default NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;


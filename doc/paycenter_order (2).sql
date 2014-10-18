-- phpMyAdmin SQL Dump
-- version 4.0.4.2
-- http://www.phpmyadmin.net
--
-- 主机: localhost
-- 生成日期: 2014 年 10 月 18 日 09:35
-- 服务器版本: 5.1.50-log
-- PHP 版本: 5.2.10p1

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- 数据库: `pcorder`
--

-- --------------------------------------------------------

--
-- 表的结构 `paycenter_order`
--

CREATE TABLE IF NOT EXISTS `paycenter_order` (
  `pid` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `mid` int(10) unsigned NOT NULL,
  `sitemid` varchar(128) NOT NULL DEFAULT '0',
  `buyer` varchar(128) NOT NULL DEFAULT '0',
  `sid` int(11) NOT NULL DEFAULT '0',
  `appid` int(11) NOT NULL,
  `pmode` smallint(5) unsigned NOT NULL,
  `pamount` float NOT NULL,
  `pcoins` int(10) unsigned NOT NULL,
  `pchips` bigint(20) unsigned NOT NULL DEFAULT '0',
  `pcard` int(11) unsigned NOT NULL DEFAULT '0',
  `pnum` int(11) NOT NULL DEFAULT '1',
  `payconfid` int(10) unsigned NOT NULL,
  `pcoinsnow` int(10) unsigned NOT NULL,
  `pdealno` varchar(255) NOT NULL,
  `pbankno` varchar(255) NOT NULL,
  `desc` varchar(255) NOT NULL DEFAULT '0',
  `pstarttime` int(10) unsigned NOT NULL,
  `pendtime` int(10) unsigned NOT NULL,
  `pstatus` tinyint(3) unsigned NOT NULL,
  `pamount_rate` float NOT NULL DEFAULT '1',
  `pamount_unit` varchar(64) NOT NULL DEFAULT '',
  `pamount_usd` float NOT NULL DEFAULT '0',
  `ext_1` int(11) NOT NULL DEFAULT '0',
  `ext_2` int(11) NOT NULL DEFAULT '0',
  `ext_3` int(11) NOT NULL DEFAULT '0',
  `ext_4` varchar(64) NOT NULL DEFAULT '',
  `ext_5` varchar(255) NOT NULL DEFAULT '',
  `ext_6` varchar(64) NOT NULL DEFAULT '',
  `ext_7` varchar(64) NOT NULL DEFAULT '',
  `ext_8` varchar(255) NOT NULL DEFAULT '',
  `ext_9` varchar(255) NOT NULL DEFAULT '',
  `ext_10` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`pid`),
  KEY `pdealno` (`pdealno`),
  KEY `idx_order_status_id` (`pstatus`,`pid`),
  KEY `idx_order_sid_starttime` (`sid`,`sitemid`,`pstarttime`),
  KEY `idx_order_appid_starttime` (`sid`,`appid`,`pstarttime`),
  KEY `idx_order_status_id_endtime` (`pstatus`,`appid`,`sitemid`,`pendtime`),
  KEY `idx_order_starttime` (`pstarttime`),
  KEY `idx_pbankno_pmode_pstarttime` (`pbankno`,`pmode`,`pstarttime`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 AUTO_INCREMENT=2018582526 ;

--
-- 转存表中的数据 `paycenter_order`
--

INSERT INTO `paycenter_order` (`pid`, `mid`, `sitemid`, `buyer`, `sid`, `appid`, `pmode`, `pamount`, `pcoins`, `pchips`, `pcard`, `pnum`, `payconfid`, `pcoinsnow`, `pdealno`, `pbankno`, `desc`, `pstarttime`, `pendtime`, `pstatus`, `pamount_rate`, `pamount_unit`, `pamount_usd`, `ext_1`, `ext_2`, `ext_3`, `ext_4`, `ext_5`, `ext_6`, `ext_7`, `ext_8`, `ext_9`, `ext_10`) VALUES
(1970676120, 0, '100002112615471', '100002112615471', 4, 5, 27, 3, 0, 27000, 0, 1, 25296, 0, '0', '0', '0', 1408464000, 0, 0, 1, 'USD', 3, 0, 0, 0, '', '', '', '', '', '', '0101');

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;

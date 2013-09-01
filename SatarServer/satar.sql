SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: `satar`
--

-- --------------------------------------------------------

--
-- Table structure for table `satar`
--

CREATE TABLE IF NOT EXISTS `satar` (
  `index` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `TSS` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'TimeStamp of the local Server',
  `TSN` bigint(20) unsigned NOT NULL COMMENT 'TimeStamp of the Node in ms since last reset',
  `EVENT` int(10) unsigned NOT NULL COMMENT 'Type of event',
  `ID` int(10) unsigned NOT NULL COMMENT 'ID, e.g. unique number of the competition''s participant',
  `NODE` int(10) unsigned NOT NULL COMMENT 'SATAR Node ID, unique number of the sending SATAR controller',
  PRIMARY KEY (`index`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 COMMENT='contains raw values from the SATAR nodes' AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `satar_results`
--

CREATE TABLE IF NOT EXISTS `satar_results` (
  `runID` int(11) NOT NULL AUTO_INCREMENT,
  `raceID` int(11) NOT NULL,
  `riderID` int(11) NOT NULL,
  `runTime` int(11) NOT NULL,
  `onTrack` tinyint(1) NOT NULL,
  PRIMARY KEY (`runID`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `satar_users`
--

CREATE TABLE IF NOT EXISTS `satar_users` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `uid` int(11) unsigned NOT NULL DEFAULT '0',
  `firstname` varchar(254) NOT NULL,
  `lastname` varchar(254) NOT NULL,
  `email` varchar(254) NOT NULL,
  `team` varchar(254) NOT NULL,
  `disclaimer` varchar(254) NOT NULL,
  `registered` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  `ip` int(11) NOT NULL,
  `size` varchar(254) NOT NULL,
  `city` varchar(254) NOT NULL,
  `street` varchar(254) NOT NULL,
  `streetno` int(11) unsigned NOT NULL,
  `zipcode` varchar(254) NOT NULL,
  `dob` date NOT NULL DEFAULT '0000-00-00',
  `sex` varchar(254) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COMMENT='Table for all registered participants. ' AUTO_INCREMENT=1 ;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;

CREATE SCHEMA IF NOT EXISTS `authservdb`;
USE `authservdb`;

CREATE TABLE `users` (
  `ID` bigint unsigned NOT NULL AUTO_INCREMENT,
  `username` varchar(50) NOT NULL,
  `last_login` timestamp NULL DEFAULT '0000-00-00 00:00:00',
  `creation_date` timestamp NOT NULL,
  PRIMARY KEY (`ID`),
  UNIQUE KEY `username_UNIQUE` (`username`),
  UNIQUE KEY `ID_UNIQUE` (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE `web_auth` (
  `ID` bigint unsigned NOT NULL AUTO_INCREMENT,
  `User_ID` bigint unsigned NOT NULL,
  `email` varchar(255) NOT NULL,
  `password` char(64) NOT NULL,
  `salt` char(64) NOT NULL,
  PRIMARY KEY (`ID`),
  UNIQUE KEY `User_ID_UNIQUE` (`User_ID`),
  UNIQUE KEY `ID_UNIQUE` (`ID`),
  CONSTRAINT `UserID_FK` FOREIGN KEY (`User_ID`) REFERENCES `users` (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

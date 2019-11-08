CREATE SCHEMA IF NOT EXISTS `authservdb`;

CREATE TABLE `user` (
  `ID` bigint unsigned NOT NULL AUTO_INCREMENT,
  `username` varchar(50) NOT NULL,
  `last_login` timestamp NULL DEFAULT NULL,
  `creation_date` timestamp NOT NULL,
  PRIMARY KEY (`ID`),
  UNIQUE KEY `username_UNIQUE` (`username`),
  UNIQUE KEY `ID_UNIQUE` (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE `web_auth` (
  `ID` bigint unsigned NOT NULL AUTO_INCREMENT,
  `User_ID` bigint unsigned NOT NULL,
  `username` varchar(50) NOT NULL,
  `email` varchar(255) NOT NULL,
  `password` char(64) NOT NULL,
  `salt` char(64) NOT NULL,
  PRIMARY KEY (`ID`),
  UNIQUE KEY `User_ID_UNIQUE` (`User_ID`),
  UNIQUE KEY `username_UNIQUE` (`username`),
  UNIQUE KEY `ID_UNIQUE` (`ID`),
  KEY `UserName_IDX` (`username`),
  CONSTRAINT `UserID_FK` FOREIGN KEY (`User_ID`) REFERENCES `user` (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

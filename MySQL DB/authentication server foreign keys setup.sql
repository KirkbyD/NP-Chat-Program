ALTER TABLE `authservdb`.`web_auth` 
ADD CONSTRAINT `UserID_FK`
  FOREIGN KEY (`User_ID`)
  REFERENCES `authservdb`.`user` (`ID`)
  ON DELETE NO ACTION
  ON UPDATE NO ACTION
;

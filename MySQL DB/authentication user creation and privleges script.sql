DROP USER IF EXISTS 'auth_serv'@'localhost';
CREATE USER IF NOT EXISTS 'auth_serv'@'localhost'
	IDENTIFIED BY 'password'
    REQUIRE SSL
;

GRANT SELECT, INSERT, UPDATE, DELETE ON authservdb.* TO 'auth_serv'@'localhost';

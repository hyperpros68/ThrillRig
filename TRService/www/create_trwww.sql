INSERT INTO tr_users (login_id, password, name, role, status) 
VALUES ('trwww', SHA2(CONCAT('trwww!@#$', 'trwww'), 256), '대시보드', 'ADMIN', 'ACTIVE') 
ON DUPLICATE KEY UPDATE password=VALUES(password);

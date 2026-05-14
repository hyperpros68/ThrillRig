CREATE USER IF NOT EXISTS 'thrillrig'@'localhost' IDENTIFIED BY 'tr!@#$';
GRANT ALL PRIVILEGES ON ThrillRig.* TO 'thrillrig'@'localhost';
FLUSH PRIVILEGES;

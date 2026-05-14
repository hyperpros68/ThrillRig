#!/usr/bin/php
<?php
/**
 * ThrillRig eJabberd External Authentication Script
 * 
 * This script allows eJabberd to authenticate users directly against 
 * the 'tr_users' table using the custom SHA256(password + id) hash.
 */

error_reporting(0); // Disable PHP errors on stdout to avoid protocol corruption

// DB Configuration
$db_host = 'localhost';
$db_user = 'thrillrig';
$db_pass = 'tr!@#$';
$db_name = 'ThrillRig';

function log_debug($msg) {
    file_put_contents("/tmp/trauth.log", date('[Y-m-d H:i:s] ') . $msg . "\n", FILE_APPEND);
}

$conn = new mysqli($db_host, $db_user, $db_pass, $db_name);

if ($conn->connect_error) {
    log_debug("Database connection failed: " . $conn->connect_error);
    exit(1);
}
log_debug("Database connected successfully.");

if ($conn->connect_error) {
    // If DB connection fails, we can't authenticate
    while(true) {
        if (!fread(STDIN, 2)) break;
        fwrite(STDOUT, pack("nn", 2, 0));
    }
    exit;
}

while (true) {
    // Read 2 bytes length header
    $header = fread(STDIN, 2);
    if (!$header || strlen($header) < 2) break;
    
    $len = unpack("n", $header)[1];
    log_debug("Received packet length: $len");
    
    // Read payload
    $data = fread(STDIN, $len);
    if (!$data) break;
    log_debug("Payload: $data");
    
    $fields = explode(":", $data);
    $op = $fields[0];
    
    $success = false;
    
    if ($op == "auth") {
        $user = $fields[1];
        $pass = $fields[3];
        log_debug("Attempting auth for user: $user");
        
        $hashed = hash('sha256', $pass . $user);
        $stmt = $conn->prepare("SELECT login_id FROM tr_users WHERE login_id = ? AND password = ?");
        if ($stmt) {
            $stmt->bind_param("ss", $user, $hashed);
            $stmt->execute();
            $stmt->store_result();
            if ($stmt->num_rows > 0) {
                $success = true;
            }
            log_debug("Auth result for $user: " . ($success ? "SUCCESS" : "FAIL"));
            $stmt->close();
        }
    } elseif ($op == "isuser") {
        $user = $fields[1];
        log_debug("Checking if user exists: $user");
        $stmt = $conn->prepare("SELECT login_id FROM tr_users WHERE login_id = ?");
        if ($stmt) {
            $stmt->bind_param("s", $user);
            $stmt->execute();
            $stmt->store_result();
            if ($stmt->num_rows > 0) {
                $success = true;
            }
            log_debug("Exist check for $user: " . ($success ? "YES" : "NO"));
            $stmt->close();
        }
    }
    
    fwrite(STDOUT, pack("nn", 2, $success ? 1 : 0));
}
?>

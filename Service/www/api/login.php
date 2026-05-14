<?php
header('Content-Type: application/json; charset=utf-8');

// 에러 보고 비활성화 (운영 환경)
error_reporting(0);

require_once 'ThrillRigService.php';

$service = ThrillRigService::getInstance();

$login_id = $_POST['login_id'] ?? '';
$password = $_POST['password'] ?? '';
$role     = $_POST['role'] ?? '';

if (empty($login_id) || empty($password) || empty($role)) {
    echo json_encode(['success' => false, 'message' => '모든 정보를 입력해주세요.']);
    exit;
}

$result = $service->login($login_id, $password, $role);

if ($result['success']) {
    // 로그인 성공 시 접속 로그 기록 (선택 사항)
    $service->logConnection($result['user_id'], 'ON', $_SERVER['REMOTE_ADDR']);
}

echo json_encode($result);
?>

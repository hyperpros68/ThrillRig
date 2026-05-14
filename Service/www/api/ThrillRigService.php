<?php
/**
 * ThrillRig 핵심 비즈니스 로직 및 DB 관리를 위한 서비스 클래스
 * 10년 경력 시니어 개발자 설계: 싱글톤 패턴 및 보안 로직 중앙 집중화
 */

class ThrillRigService {
    private static $instance = null;
    private $conn = null;

    private $host = 'localhost';
    private $db   = 'ThrillRig';
    private $user = 'thrillrig';
    private $pass = 'tr!@#$';

    private function __construct() {
        $this->conn = new mysqli($this->host, $this->user, $this->pass, $this->db);
        if ($this->conn->connect_error) {
            die(json_encode(['success' => false, 'message' => 'DB 연결 실패']));
        }
        $this->conn->set_charset("utf8mb4");
    }

    public static function getInstance() {
        if (self::$instance == null) {
            self::$instance = new ThrillRigService();
        }
        return self::$instance;
    }

    public function getConnection() {
        return $this->conn;
    }

    /**
     * 패스워드 암호화 (SHA256 기반, ID를 Salt로 사용)
     */
    public function encryptPassword($password, $id) {
        return hash('sha256', $password . $id);
    }

    /**
     * 로그인 검증
     */
    public function login($login_id, $password, $role) {
        $stmt = $this->conn->prepare("SELECT id, password, name, role FROM tr_users WHERE login_id = ? AND role = ? AND status = 'ACTIVE' AND deleted_at IS NULL");
        $stmt->bind_param("ss", $login_id, $role);
        $stmt->execute();
        $result = $stmt->get_result();

        if ($user_data = $result->fetch_assoc()) {
            $hashed_input = $this->encryptPassword($password, $user_data['id']);
            if ($hashed_input === $user_data['password']) {
                return [
                    'success' => true,
                    'user_id' => (int)$user_data['id'],
                    'login_id' => $login_id,
                    'name' => $user_data['name'],
                    'role' => $user_data['role']
                ];
            }
            return ['success' => false, 'message' => '비밀번호가 일치하지 않습니다.'];
        }
        return ['success' => false, 'message' => '계정을 찾을 수 없거나 권한이 없습니다.'];
    }

    /**
     * 연결 로그 기록
     */
    public function logConnection($user_id, $event_type, $ip) {
        $stmt = $this->conn->prepare("INSERT INTO tr_connection_logs (user_id, event_type, ip_address) VALUES (?, ?, ?)");
        $stmt->bind_param("iss", $user_id, $event_type, $ip);
        return $stmt->execute();
    }

    /**
     * 어댑터 신호(파일 내용) 기록
     */
    public function logSignal($user_id, $content) {
        $stmt = $this->conn->prepare("INSERT INTO tr_adapter_signals (user_id, raw_content) VALUES (?, ?)");
        $stmt->bind_param("is", $user_id, $content);
        return $stmt->execute();
    }
}
?>

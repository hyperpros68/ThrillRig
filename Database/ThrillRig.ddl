-- 1. 데이터베이스 초기 설정
DROP DATABASE IF EXISTS ThrillRig;
CREATE DATABASE IF NOT EXISTS ThrillRig 
DEFAULT CHARACTER SET utf8mb4 
COLLATE utf8mb4_unicode_ci;

USE ThrillRig;

-- 2. 패스워드 암호화 전용 함수 생성 (id를 salt로 사용)
-- 보안 정책: SHA2(password + id) 방식으로 암호화 수행
DELIMITER //
CREATE FUNCTION fn_encrypt_password(p_password VARCHAR(255), p_id INT) 
RETURNS VARCHAR(255)
DETERMINISTIC
BEGIN
    RETURN SHA2(CONCAT(p_password, CAST(p_id AS CHAR)), 256);
END //
DELIMITER ;

-- 3. 테이블 구조 생성 (Class Table Inheritance)

-- 메인 계정 테이블
CREATE TABLE `tr_users` (
    `id` INT AUTO_INCREMENT PRIMARY KEY COMMENT '내부 유니크 ID',
    `login_id` VARCHAR(50) NOT NULL UNIQUE COMMENT '로그인 ID 및 XMPP JID',
    `password` VARCHAR(255) NOT NULL COMMENT '암호화된 패스워드',
    `name` VARCHAR(100) NOT NULL COMMENT '실명/매장명/장비명',
    `role` ENUM('ADMIN', 'MANAGER', 'AGENT', 'SHOP', 'ADAPTER') NOT NULL,
    `parent_id` INT DEFAULT NULL COMMENT '상위 관리자 ID',
    `status` VARCHAR(20) DEFAULT 'ACTIVE' COMMENT '상태 (ACTIVE, INACTIVE, SUSPENDED)',
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    `deleted_at` TIMESTAMP NULL DEFAULT NULL COMMENT '소프트 델리트 일시',
    CONSTRAINT `fk_users_parent` FOREIGN KEY (`parent_id`) REFERENCES `tr_users`(`id`) ON DELETE SET NULL,
    INDEX `idx_login_id` (`login_id`),
    INDEX `idx_parent_role` (`parent_id`, `role`)
) ENGINE=InnoDB;

-- 역할별 상세 테이블 (Extension)
CREATE TABLE `tr_admin_info` (
    `user_id` INT PRIMARY KEY,
    `admin_level` INT DEFAULT 1,
    `memo` TEXT,
    CONSTRAINT `fk_admin_ref` FOREIGN KEY (`user_id`) REFERENCES `tr_users`(`id`) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE `tr_manager_info` (
    `user_id` INT PRIMARY KEY,
    `region` VARCHAR(100),
    `phone` VARCHAR(20),
    CONSTRAINT `fk_manager_ref` FOREIGN KEY (`user_id`) REFERENCES `tr_users`(`id`) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE `tr_agent_info` (
    `user_id` INT PRIMARY KEY,
    `commission_rate` DECIMAL(5,2) DEFAULT 0.00,
    `contract_date` DATE,
    CONSTRAINT `fk_agent_ref` FOREIGN KEY (`user_id`) REFERENCES `tr_users`(`id`) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE `tr_shop_info` (
    `user_id` INT PRIMARY KEY,
    `adapter_limit` INT DEFAULT 10,
    `address` VARCHAR(255),
    `business_no` VARCHAR(20),
    CONSTRAINT `fk_shop_ref` FOREIGN KEY (`user_id`) REFERENCES `tr_users`(`id`) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE `tr_adapter_info` (
    `user_id` INT PRIMARY KEY,
    `file_path` VARCHAR(255) DEFAULT 'C:\\HGame\\ThrillRig.txt',
    `pc_spec` TEXT,
    `mac_address` VARCHAR(17),
    `status` VARCHAR(20) DEFAULT 'OFFLINE',
    `last_seen` DATETIME,
    CONSTRAINT `fk_adapter_ref` FOREIGN KEY (`user_id`) REFERENCES `tr_users`(`id`) ON DELETE CASCADE
) ENGINE=InnoDB;

-- 관제 및 감사 시스템 테이블
CREATE TABLE `tr_adapter_signals` (
    `id` BIGINT AUTO_INCREMENT PRIMARY KEY,
    `user_id` INT NOT NULL,
    `raw_content` TEXT,
    `received_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX `idx_sig_user` (`user_id`, `received_at`)
) ENGINE=InnoDB;

CREATE TABLE `tr_connection_logs` (
    `id` BIGINT AUTO_INCREMENT PRIMARY KEY,
    `user_id` INT NOT NULL,
    `event_type` ENUM('ON', 'OFF') NOT NULL,
    `ip_address` VARCHAR(45),
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

CREATE TABLE `tr_notifications` (
    `id` INT AUTO_INCREMENT PRIMARY KEY,
    `user_id` INT NOT NULL,
    `message` TEXT NOT NULL,
    `is_read` BOOLEAN DEFAULT FALSE,
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

CREATE TABLE `tr_system_logs` (
    `id` BIGINT AUTO_INCREMENT PRIMARY KEY,
    `admin_id` INT,
    `target_id` INT,
    `action` VARCHAR(50),
    `description` TEXT,
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- 4. 어댑터 자동 생성을 위한 Stored Procedure (암호화 로직 포함)
DROP PROCEDURE IF EXISTS sp_auto_generate_adapters;
DELIMITER //
CREATE PROCEDURE sp_auto_generate_adapters(
    IN p_shop_id INT,
    IN p_count INT
)
BEGIN
    DECLARE i INT DEFAULT 1;
    DECLARE v_shop_login_id VARCHAR(50);
    DECLARE v_new_user_id INT;
    DECLARE v_plain_pwd VARCHAR(255) DEFAULT 'tradp!@#$';
    
    SELECT login_id INTO v_shop_login_id FROM tr_users WHERE id = p_shop_id AND role = 'SHOP';
    
    IF v_shop_login_id IS NOT NULL THEN
        WHILE i <= p_count DO
            -- 1. 사용자 계정 생성 (패스워드는 임시로 넣고 이후 업데이트하여 ID 기반 암호화 적용)
            INSERT INTO tr_users (login_id, password, name, role, parent_id)
            VALUES (CONCAT('adp_', v_shop_login_id, '_', LPAD(i, 3, '0')), 'TEMP_PWD', CONCAT(v_shop_login_id, ' 장비 ', i), 'ADAPTER', p_shop_id);
            
            SET v_new_user_id = LAST_INSERT_ID();
            
            -- 2. 생성된 ID를 기반으로 패스워드 암호화 업데이트
            UPDATE tr_users SET password = fn_encrypt_password(v_plain_pwd, v_new_user_id) WHERE id = v_new_user_id;
            
            -- 3. 상세 정보 생성
            INSERT INTO tr_adapter_info (user_id) VALUES (v_new_user_id);
            
            SET i = i + 1;
        END WHILE;
    END IF;
END //
DELIMITER ;

-- 5. 샘플 데이터 삽입 및 계층 연결 (ID 기반 암호화 적용)

-- [5.1] Admin 계정
INSERT INTO tr_users (login_id, password, name, role, parent_id) VALUES ('admin', 'TEMP', '최고관리자', 'ADMIN', NULL);
SET @admin_id = LAST_INSERT_ID();
UPDATE tr_users SET password = fn_encrypt_password('esg!@#$', @admin_id) WHERE id = @admin_id;
INSERT INTO tr_admin_info (user_id, memo) VALUES (@admin_id, '시스템 메인 관리자');

-- [5.2] Manager 계정 (Parent: Admin)
INSERT INTO tr_users (login_id, password, name, role, parent_id) VALUES ('manager_001', 'TEMP', '서울매니저01', 'MANAGER', @admin_id);
SET @mgr_id = LAST_INSERT_ID();
UPDATE tr_users SET password = fn_encrypt_password('trmgr!@#$', @mgr_id) WHERE id = @mgr_id;
INSERT INTO tr_manager_info (user_id, region) VALUES (@mgr_id, '서울/경기');

-- [5.3] Agent 계정 (Parent: Manager)
INSERT INTO tr_users (login_id, password, name, role, parent_id) VALUES ('agent_001', 'TEMP', '강남총판', 'AGENT', @mgr_id);
SET @agt_id = LAST_INSERT_ID();
UPDATE tr_users SET password = fn_encrypt_password('tragt!@#$', @agt_id) WHERE id = @agt_id;
INSERT INTO tr_agent_info (user_id, commission_rate) VALUES (@agt_id, 5.00);

-- [5.4] Shop 계정 (Parent: Agent)
INSERT INTO tr_users (login_id, password, name, role, parent_id) VALUES ('shop_001', 'TEMP', '강남PC방', 'SHOP', @agt_id);
SET @shop_id = LAST_INSERT_ID();
UPDATE tr_users SET password = fn_encrypt_password('trshop!@#$', @shop_id) WHERE id = @shop_id;
INSERT INTO tr_shop_info (user_id, adapter_limit, address) VALUES (@shop_id, 10, '서울시 강남구 역삼동');

-- [5.5] 프로시저로 어댑터 3개 자동 생성 (암호화 자동 처리됨)
CALL sp_auto_generate_adapters(@shop_id, 3);

-- [5.6] 테스트 계정 세트
-- WWW 서버
INSERT INTO tr_users (login_id, password, name, role, parent_id) VALUES ('test_trwww', 'TEMP', '서비스서버', 'ADMIN', @admin_id);
SET @tmp_id = LAST_INSERT_ID();
UPDATE tr_users SET password = fn_encrypt_password('trwww!@#$', @tmp_id) WHERE id = @tmp_id;
-- LLM 서버
INSERT INTO tr_users (login_id, password, name, role, parent_id) VALUES ('test_trllm', 'TEMP', 'LLM서버', 'ADMIN', @admin_id);
SET @tmp_id = LAST_INSERT_ID();
UPDATE tr_users SET password = fn_encrypt_password('trllm!@#$', @tmp_id) WHERE id = @tmp_id;
-- 테스트 매니저
INSERT INTO tr_users (login_id, password, name, role, parent_id) VALUES ('test_trmgr_001', 'TEMP', '테스트매니저01', 'MANAGER', @admin_id);
SET @test_mgr_id = LAST_INSERT_ID();
UPDATE tr_users SET password = fn_encrypt_password('trmgr!@#$', @test_mgr_id) WHERE id = @test_mgr_id;
-- 테스트 총판
INSERT INTO tr_users (login_id, password, name, role, parent_id) VALUES ('test_tragt_001', 'TEMP', '테스트총판01', 'AGENT', @test_mgr_id);
SET @test_agt_id = LAST_INSERT_ID();
UPDATE tr_users SET password = fn_encrypt_password('tragt!@#$', @test_agt_id) WHERE id = @test_agt_id;
-- 테스트 매장
INSERT INTO tr_users (login_id, password, name, role, parent_id) VALUES ('test_shop_001', 'TEMP', '테스트매장01', 'SHOP', @test_agt_id);
SET @test_shop_id = LAST_INSERT_ID();
UPDATE tr_users SET password = fn_encrypt_password('trshop!@#$', @test_shop_id) WHERE id = @test_shop_id;

-- 6. 주요 조회 쿼리 가이드

-- [eJabberd 인증용 비밀번호 검증 쿼리]
-- SELECT (password = fn_encrypt_password('입력비밀번호', id)) as is_valid FROM tr_users WHERE login_id = '유저아이디';

-- [대시보드: 최근 관제 신호 10개]
-- SELECT u.name, s.raw_content, s.received_at FROM tr_adapter_signals s JOIN tr_users u ON s.user_id = u.id ORDER BY s.id DESC LIMIT 10;

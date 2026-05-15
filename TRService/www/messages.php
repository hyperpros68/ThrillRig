<?php
// DB Configuration
$db_host = 'localhost';
$db_user = 'thrillrig';
$db_pass = 'tr!@#$';
$db_name = 'thrillrig';

$conn = new mysqli($db_host, $db_user, $db_pass, $db_name);
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

// Search Filters
$search_user = isset($_GET['user']) ? $_GET['user'] : '';
$search_keyword = isset($_GET['keyword']) ? $_GET['keyword'] : '';
$start_date = isset($_GET['start_date']) ? $_GET['start_date'] : '';
$end_date = isset($_GET['end_date']) ? $_GET['end_date'] : '';

$sql = "SELECT id, bare_peer, txt, created_at FROM archive WHERE 1=1";

if ($search_user) {
    $sql .= " AND bare_peer LIKE '%" . $conn->real_escape_string($search_user) . "%'";
}
if ($search_keyword) {
    $sql .= " AND txt LIKE '%" . $conn->real_escape_string($search_keyword) . "%'";
}
if ($start_date) {
    $sql .= " AND created_at >= '" . $conn->real_escape_string($start_date) . " 00:00:00'";
}
if ($end_date) {
    $sql .= " AND created_at <= '" . $conn->real_escape_string($end_date) . " 23:59:59'";
}

$sql .= " ORDER BY id DESC LIMIT 100";
$result = $conn->query($sql);

$aes_key = "ThrillRigV1_SecretKey_2026_0515";

function decrypt_message($body, $key_str) {
    $data = json_decode($body, true);
    if (!$data || !isset($data['enc']) || $data['enc'] !== true) return $body;

    $key = str_pad($key_str, 32, '0');
    if (strlen($key) > 32) $key = substr($key, 0, 32);

    $ciphertext_combined = base64_decode($data['data']);
    $iv = base64_decode($data['iv']);
    
    if (strlen($ciphertext_combined) < 16) return "[Invalid Payload]";
    
    $tag = substr($ciphertext_combined, -16);
    $cipher = substr($ciphertext_combined, 0, -16);

    $decrypted = openssl_decrypt($cipher, 'aes-256-gcm', $key, OPENSSL_RAW_DATA, $iv, $tag);
    return $decrypted !== false ? $decrypted : "[Decryption Failed]";
}
?>
<!DOCTYPE html>
<html lang="ko">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ThrillRig - Advanced Archive</title>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;600;800&display=swap" rel="stylesheet">
    <style>
        :root {
            --primary-color: #6366f1;
            --bg-gradient: linear-gradient(135deg, #0f172a 0%, #1e293b 100%);
            --glass-bg: rgba(255, 255, 255, 0.03);
            --glass-border: rgba(255, 255, 255, 0.1);
            --text-main: #f8fafc;
            --text-muted: #94a3b8;
            --accent: #818cf8;
        }

        body {
            margin: 0;
            padding: 2rem;
            font-family: 'Inter', sans-serif;
            background: var(--bg-gradient);
            color: var(--text-main);
            min-height: 100vh;
        }

        .container {
            max-width: 1300px;
            margin: 0 auto;
        }

        .header {
            margin-bottom: 2.5rem;
        }

        h1 {
            font-size: 2.2rem;
            font-weight: 800;
            background: linear-gradient(to right, #818cf8, #c084fc);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            margin: 0.5rem 0;
        }

        .filter-panel {
            background: var(--glass-bg);
            backdrop-filter: blur(10px);
            border: 1px solid var(--glass-border);
            padding: 1.5rem;
            border-radius: 20px;
            margin-bottom: 2rem;
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 1.5rem;
            align-items: flex-end;
        }

        .filter-group {
            display: flex;
            flex-direction: column;
            gap: 0.5rem;
        }

        .filter-group label {
            font-size: 0.8rem;
            font-weight: 600;
            color: var(--accent);
            text-transform: uppercase;
            letter-spacing: 0.05em;
        }

        input {
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid var(--glass-border);
            padding: 0.75rem 1rem;
            border-radius: 12px;
            color: white;
            outline: none;
            transition: all 0.2s;
        }

        input:focus {
            border-color: var(--primary-color);
            background: rgba(255, 255, 255, 0.08);
        }

        .button-group {
            display: flex;
            gap: 0.5rem;
        }

        button {
            background: var(--primary-color);
            color: white;
            border: none;
            padding: 0.75rem 1.5rem;
            border-radius: 12px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s;
            flex: 1;
        }

        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(99, 102, 241, 0.3);
        }

        .reset-btn {
            background: rgba(255, 255, 255, 0.1);
        }

        .archive-card {
            background: var(--glass-bg);
            backdrop-filter: blur(12px);
            border: 1px solid var(--glass-border);
            border-radius: 24px;
            overflow: hidden;
            box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.5);
        }

        table {
            width: 100%;
            border-collapse: collapse;
            text-align: left;
        }

        th {
            padding: 1.25rem 1.5rem;
            background: rgba(255, 255, 255, 0.05);
            font-size: 0.75rem;
            text-transform: uppercase;
            letter-spacing: 0.1em;
            color: var(--accent);
            font-weight: 700;
        }

        td {
            padding: 1.25rem 1.5rem;
            border-bottom: 1px solid var(--glass-border);
            font-size: 0.95rem;
            line-height: 1.5;
        }

        tr:hover td {
            background: rgba(255, 255, 255, 0.02);
        }

        .badge {
            background: rgba(99, 102, 241, 0.15);
            color: #a5b4fc;
            padding: 0.25rem 0.6rem;
            border-radius: 6px;
            font-size: 0.85rem;
            font-family: monospace;
            word-break: break-all;
        }

        .timestamp {
            color: var(--text-muted);
            font-size: 0.85rem;
            white-space: nowrap;
        }

        .empty-state {
            padding: 5rem;
            text-align: center;
            color: var(--text-muted);
        }

        .nav-link {
            color: var(--text-muted);
            text-decoration: none;
            font-size: 0.9rem;
            transition: color 0.2s;
        }

        .nav-link:hover {
            color: var(--accent);
        }

        /* Responsive Mobile */
        @media (max-width: 768px) {
            .filter-panel {
                grid-template-columns: 1fr;
            }
            body { padding: 1rem; }
            th:nth-child(3), td:nth-child(3) { display: none; }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <a href="index.php" class="nav-link">← Back to Dashboard</a>
            <h1>Advanced Archive</h1>
        </div>

        <form class="filter-panel" method="GET">
            <div class="filter-group">
                <label>User JID</label>
                <input type="text" name="user" placeholder="예: shop_001" value="<?= htmlspecialchars($search_user) ?>">
            </div>
            <div class="filter-group">
                <label>Keyword</label>
                <input type="text" name="keyword" placeholder="메시지 내용 검색..." value="<?= htmlspecialchars($search_keyword) ?>">
            </div>
            <div class="filter-group">
                <label>Start Date</label>
                <input type="date" name="start_date" value="<?= htmlspecialchars($start_date) ?>">
            </div>
            <div class="filter-group">
                <label>End Date</label>
                <input type="date" name="end_date" value="<?= htmlspecialchars($end_date) ?>">
            </div>
            <div class="button-group">
                <button type="submit">Search</button>
                <button type="button" class="reset-btn" onclick="location.href='messages.php'">Reset</button>
            </div>
        </form>

        <div class="archive-card">
            <table>
                <thead>
                    <tr>
                        <th style="width: 250px;">Sender</th>
                        <th>Message Content</th>
                        <th style="width: 200px;">Timestamp</th>
                    </tr>
                </thead>
                <tbody>
                    <?php if ($result && $result->num_rows > 0): ?>
                        <?php while($row = $result->fetch_assoc()): ?>
                            <tr>
                                <td><span class="badge"><?= htmlspecialchars($row['bare_peer']) ?></span></td>
                                <td><?= nl2br(htmlspecialchars(decrypt_message($row['txt'], $aes_key))) ?></td>
                                <td class="timestamp"><?= $row['created_at'] ?></td>
                            </tr>
                        <?php endwhile; ?>
                    <?php else: ?>
                        <tr>
                            <td colspan="3" class="empty-state">
                                검색 조건에 맞는 메시지가 없습니다.
                            </td>
                        </tr>
                    <?php endif; ?>
                </tbody>
            </table>
        </div>
        
        <p style="text-align: center; color: var(--text-muted); margin-top: 2rem; font-size: 0.8rem;">
            © 2026 ThrillRig Messaging Service - Search optimized for up to 100 records
        </p>
    </div>
</body>
</html>
<?php $conn->close(); ?>

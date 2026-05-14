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

// Search Filter
$search_user = isset($_GET['user']) ? $_GET['user'] : '';
$sql = "SELECT id, bare_peer, txt, created_at FROM archive";
if ($search_user) {
    $sql .= " WHERE bare_peer LIKE '%" . $conn->real_escape_string($search_user) . "%'";
}
$sql .= " ORDER BY id DESC LIMIT 50";
$result = $conn->query($sql);
?>
<!DOCTYPE html>
<html lang="ko">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ThrillRig - Message Archive</title>
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
            max-width: 1200px;
            margin: 0 auto;
        }

        .header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 2.5rem;
        }

        h1 {
            font-size: 2rem;
            font-weight: 800;
            background: linear-gradient(to right, #818cf8, #c084fc);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            margin: 0;
        }

        .search-box {
            display: flex;
            gap: 0.5rem;
        }

        input[type="text"] {
            background: var(--glass-bg);
            border: 1px solid var(--glass-border);
            padding: 0.6rem 1rem;
            border-radius: 12px;
            color: white;
            font-family: inherit;
            outline: none;
            width: 250px;
        }

        input[type="text"]:focus {
            border-color: var(--primary-color);
        }

        button {
            background: var(--primary-color);
            color: white;
            border: none;
            padding: 0.6rem 1.2rem;
            border-radius: 12px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s;
        }

        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(99, 102, 241, 0.3);
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
        }

        tr:last-child td {
            border-bottom: none;
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
        }

        .timestamp {
            color: var(--text-muted);
            font-size: 0.85rem;
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
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <div>
                <a href="index.php" class="nav-link">← Back to Dashboard</a>
                <h1>Message Archive</h1>
            </div>
            <form class="search-box" method="GET">
                <input type="text" name="user" placeholder="User JID 검색..." value="<?= htmlspecialchars($search_user) ?>">
                <button type="submit">검색</button>
                <button type="button" onclick="location.href='messages.php'" style="background: rgba(255,255,255,0.1); color: #fff;">초기화</button>
            </form>
        </div>

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
                                <td><?= htmlspecialchars($row['txt']) ?></td>
                                <td class="timestamp"><?= $row['created_at'] ?></td>
                            </tr>
                        <?php endwhile; ?>
                    <?php else: ?>
                        <tr>
                            <td colspan="3" class="empty-state">
                                검색된 메시지가 없습니다.
                            </td>
                        </tr>
                    <?php endif; ?>
                </tbody>
            </table>
        </div>
        
        <p style="text-align: center; color: var(--text-muted); margin-top: 2rem; font-size: 0.8rem;">
            © 2026 ThrillRig Messaging Service - Last 50 messages displayed
        </p>
    </div>
</body>
</html>
<?php $conn->close(); ?>

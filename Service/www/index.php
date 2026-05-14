<!DOCTYPE html>
<html lang="ko">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ThrillRig Service - Hello World</title>
    <link rel="icon" href="data:,">
    <script src="https://unpkg.com/strophe.js/dist/strophe.js"></script>
    <style>
        :root {
            --primary-color: #6366f1;
            --bg-gradient: linear-gradient(135deg, #0f172a 0%, #1e293b 100%);
            --glass-bg: rgba(255, 255, 255, 0.05);
            --glass-border: rgba(255, 255, 255, 0.1);
            --text-main: #f8fafc;
            --text-muted: #94a3b8;
        }

        body {
            margin: 0;
            padding: 0;
            font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: var(--bg-gradient);
            color: var(--text-main);
            height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            overflow: hidden;
        }

        .container {
            text-align: center;
            padding: 3rem;
            background: var(--glass-bg);
            backdrop-filter: blur(12px);
            border: 1px solid var(--glass-border);
            border-radius: 24px;
            box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.5);
            animation: fadeIn 0.8s ease-out;
        }

        h1 {
            font-size: 3.5rem;
            margin-bottom: 1rem;
            background: linear-gradient(to right, #818cf8, #c084fc);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            font-weight: 800;
        }

        p {
            font-size: 1.2rem;
            color: var(--text-muted);
        }

        .status-badge {
            display: inline-block;
            margin-top: 2rem;
            padding: 0.5rem 1rem;
            border-radius: 9999px;
            font-size: 0.875rem;
            font-weight: 600;
            background: rgba(239, 68, 68, 0.1);
            color: #ef4444;
            border: 1px solid rgba(239, 68, 68, 0.2);
            transition: all 0.3s ease;
        }

        .status-badge.connected {
            background: rgba(34, 197, 94, 0.1);
            color: #22c55e;
            border: 1px solid rgba(34, 197, 94, 0.2);
        }

        @keyframes fadeIn {
            from { opacity: 0; transform: translateY(20px); }
            to { opacity: 1; transform: translateY(0); }
        }

        /* Custom Alert Style */
        #custom-alert {
            display: none;
            position: fixed;
            top: 20px;
            right: 20px;
            padding: 1.5rem;
            background: #6366f1;
            color: white;
            border-radius: 12px;
            box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.2);
            z-index: 1000;
            animation: slideIn 0.5s ease-out;
        }

        @keyframes slideIn {
            from { transform: translateX(100%); opacity: 0; }
            to { transform: translateX(0); opacity: 1; }
        }

        .notification {
            margin-bottom: 1rem;
            padding: 1.25rem;
            background: rgba(30, 41, 59, 0.9);
            backdrop-filter: blur(8px);
            color: white;
            border-radius: 16px;
            border-left: 4px solid #6366f1;
            box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.3);
            min-width: 300px;
            animation: slideIn 0.4s cubic-bezier(0.16, 1, 0.3, 1);
            transition: all 0.3s ease;
        }

        #notification-container {
            position: fixed;
            top: 20px;
            right: 20px;
            z-index: 1000;
            display: flex;
            flex-direction: column;
            align-items: flex-end;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Hello world</h1>
        <p>ThrillRig System Messaging Service</p>
        <div id="xmpp-status" class="status-badge">XMPP: Disconnected</div>
    </div>

    <div id="notification-container"></div>

    <script>
        const BOSH_SERVICE = 'http://59.187.96.23:5280/http-bind';
        const JID = 'trwww@59.187.96.23';
        const PASSWORD = 'trwww!@#$';

        let connection = null;

        function log(msg) {
            console.log("[XMPP]", msg);
        }

        function onMessage(msg) {
            log("Incoming message packet received");
            console.log(msg); // Log the full XML packet

            const to = msg.getAttribute('to');
            const from = msg.getAttribute('from');
            const type = msg.getAttribute('type');
            const elems = msg.getElementsByTagName('body');

            if (type === "chat" && elems.length > 0) {
                const body = Strophe.getText(elems[0]);
                log("Message from " + from + ": " + body);
                
                // Show as an alert
                showAlert("Notification", "Message from " + from.split('/')[0] + ": " + body);
            }

            return true;
        }

        function onConnect(status) {
            const statusBadge = document.getElementById('xmpp-status');
            if (status === Strophe.Status.CONNECTING) {
                log('Strophe is connecting.');
                statusBadge.innerText = 'XMPP: Connecting...';
            } else if (status === Strophe.Status.CONNFAIL) {
                log('Strophe failed to connect.');
                statusBadge.innerText = 'XMPP: Failed to connect';
            } else if (status === Strophe.Status.DISCONNECTING) {
                log('Strophe is disconnecting.');
            } else if (status === Strophe.Status.DISCONNECTED) {
                log('Strophe is disconnected.');
                statusBadge.innerText = 'XMPP: Disconnected';
                statusBadge.classList.remove('connected');
            } else if (status === Strophe.Status.CONNECTED) {
                log('Strophe is connected.');
                console.log('%c XMPP Connection Success! ', 'background: #22c55e; color: #fff; font-weight: bold;');
                
                statusBadge.innerText = 'XMPP: Connected';
                statusBadge.classList.add('connected');
                
                // Show a success notification on screen
                showAlert("System", "XMPP Server Connected Successfully!");

                connection.addHandler(onMessage, null, 'message', null, null,  null);
                connection.send($pres().tree());
            }
        }

        function showAlert(title, message) {
            const container = document.getElementById('notification-container');
            const notification = document.createElement('div');
            notification.className = 'notification';
            notification.innerHTML = `
                <div style="font-weight: 800; font-size: 0.75rem; text-transform: uppercase; letter-spacing: 0.05em; color: #818cf8; margin-bottom: 0.25rem;">${title}</div>
                <div style="font-size: 1rem; line-height: 1.5;">${message}</div>
            `;
            
            container.appendChild(notification);

            setTimeout(() => {
                notification.style.opacity = '0';
                notification.style.transform = 'translateX(20px)';
                setTimeout(() => {
                    notification.remove();
                }, 300);
            }, 5000);
        }

        window.onload = () => {
            log("Initializing Strophe connection to " + BOSH_SERVICE);
            connection = new Strophe.Connection(BOSH_SERVICE);
            
            // HTTP 환경에서 WebCrypto API 부재로 인한 에러 방지 (필요 시)
            if (window.location.protocol !== 'https:') {
                log("Notice: Running in non-secure context. Some auth mechanisms may be limited.");
            }

            connection.connect(JID, PASSWORD, onConnect);
        };
    </script>
</body>
</html>

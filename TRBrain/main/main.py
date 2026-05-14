import warnings
import os
import sys
import logging

# 라이브러리 경고 메시지 억제 (RequestsDependencyWarning 등)
warnings.filterwarnings("ignore", message="urllib3 .* doesn't match a supported version!")
warnings.filterwarnings("ignore", message="Using slower stringprep")

import configparser
import datetime
import threading
import json
from flask import Flask, render_template, request, redirect, url_for, session, flash
from flask_socketio import SocketIO, emit
from slixmpp import ClientXMPP
import mysql.connector
import requests

# --- Configuration & Logging Setup ---
class Config:
    def __init__(self, cfg_path):
        self.cfg = configparser.ConfigParser()
        self.cfg.read(cfg_path, encoding='utf-8')
        
        self.login_id = self.cfg.get('Login', 'TR_login_ID', fallback='brain')
        self.login_pw = self.cfg.get('Login', 'TR_login_PW', fallback='brain!@#$')
        self.xmpp_host = self.cfg.get('Login', 'TR_login_Host', fallback='59.187.96.23')
        
        self.db_host = self.cfg.get('Database', 'host', fallback='59.187.96.23')
        self.db_user = self.cfg.get('Database', 'user', fallback='root')
        self.db_pass = self.cfg.get('Database', 'password', fallback='esg!@#$')
        self.db_name = self.cfg.get('Database', 'database', fallback='ThrillRig')
        
        self.active_llm = self.cfg.get('System', 'llm', fallback='Ollama')

def setup_logging():
    log_dir = os.path.join(os.path.dirname(__file__), '..', 'log')
    if not os.path.exists(log_dir):
        os.makedirs(log_dir)
    
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    log_file = os.path.join(log_dir, f"TRBrain_{timestamp}.log")
    
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s [%(levelname)s] %(message)s',
        handlers=[
            logging.FileHandler(log_file, encoding='utf-8'),
            logging.StreamHandler()
        ]
    )
    return logging.getLogger("TRBrain")

# --- XMPP Bot ---
class TRBrainBot(ClientXMPP):
    def __init__(self, jid, password, logger, socketio):
        super().__init__(jid, password)
        self.logger = logger
        self.socketio = socketio
        self.add_event_handler("session_start", self.session_start)
        self.add_event_handler("message", self.message)

    async def session_start(self, event):
        self.send_presence()
        await self.get_roster()
        self.logger.info(f"XMPP Session Started for {self.boundjid.bare}")

    def message(self, msg):
        if msg['type'] in ('chat', 'normal'):
            body = msg['body']
            sender = msg['from'].bare
            self.logger.info(f"Received message from {sender}: {body}")
            # Web UI로 실시간 전송
            self.socketio.emit('new_msg', {'sender': sender, 'body': body}, namespace='/test')

# --- LLM Providers ---
class LLMManager:
    def __init__(self, cfg):
        self.cfg = cfg

    def ask(self, prompt):
        provider = self.cfg.get('System', 'llm')
        if provider == 'Ollama':
            return self._call_ollama(prompt)
        elif provider == 'OpenRouter':
            return self._call_openrouter(prompt)
        elif provider == 'Gemini':
            return self._call_gemini(prompt)
        return "Unknown LLM Provider"

    def _call_ollama(self, prompt):
        url = self.cfg.get('Ollama', 'host') + "/api/generate"
        payload = {
            "model": self.cfg.get('Ollama', 'model'),
            "prompt": prompt,
            "stream": False
        }
        try:
            resp = requests.post(url, json=payload, timeout=30)
            return resp.json().get('response', 'No response')
        except Exception as e:
            return f"Ollama Error: {str(e)}"

    def _call_gemini(self, prompt):
        # Implementation for Gemini API...
        return "Gemini Response (To be implemented)"

# --- Flask App Setup ---
app = Flask(__name__)
app.secret_key = "thrillrig_brain_secret"
socketio = SocketIO(app, async_mode='threading')

cfg_manager = Config(os.path.join(os.path.dirname(__file__), '..', 'cfg', 'TRBrain.cfg'))
logger = setup_logging()
llm_manager = LLMManager(cfg_manager.cfg)
xmpp_bot = None

@app.route('/')
def index():
    if 'logged_in' not in session:
        return redirect(url_for('login'))
    return render_template('dashboard.html', user=session['user_id'], llm=cfg_manager.active_llm)

@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        user_id = request.form.get('id')
        user_pw = request.form.get('pw')
        
        # Simple config-based auth (Can be changed to DB-based)
        if user_id == cfg_manager.login_id and user_pw == cfg_manager.login_pw:
            session['logged_in'] = True
            session['user_id'] = user_id
            return redirect(url_for('index'))
        else:
            flash("Invalid credentials")
    return render_template('login.html')

@app.route('/logout')
def logout():
    session.clear()
    return redirect(url_for('login'))

@socketio.on('send_chat', namespace='/test')
def handle_chat(data):
    target = data.get('to')
    body = data.get('msg')
    if xmpp_bot and target and body:
        xmpp_bot.send_message(mto=target, mbody=body, mtype='chat')
        logger.info(f"Sent message to {target}: {body}")

def start_xmpp():
    global xmpp_bot
    import asyncio
    
    jid = f"{cfg_manager.login_id}@{cfg_manager.xmpp_host}"
    logger.info(f"Connecting to XMPP as {jid}...")
    
    try:
        # 새로운 이벤트 루프 생성 (스레드 내 실행을 위해)
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        
        xmpp_bot = TRBrainBot(jid, cfg_manager.login_pw, logger, socketio)
        xmpp_bot.register_plugin('xep_0030')
        xmpp_bot.register_plugin('xep_0199')
        
        if xmpp_bot.connect():
            logger.info("XMPP Connected successfully.")
            # Slixmpp의 비동기 루프 실행 (메서드가 없을 경우 loop 실행)
            if hasattr(xmpp_bot, 'process'):
                xmpp_bot.process()
            else:
                loop.run_forever()
        else:
            logger.error("Failed to connect to XMPP.")
    except Exception as e:
        logger.error(f"XMPP Startup Error: {str(e)}")

if __name__ == '__main__':
    # XMPP Bot을 별도 스레드에서 실행
    xmpp_thread = threading.Thread(target=start_xmpp, daemon=True)
    xmpp_thread.start()
    
    # 로컬 디버깅 시 브라우저 자동 오픈
    import webbrowser
    def open_browser():
        import time
        time.sleep(2) # 서버 가동 대기
        protocol = "http" if os.name == 'nt' else "https"
        webbrowser.open(f"{protocol}://127.0.0.1:19990")
    
    threading.Thread(target=open_browser, daemon=True).start()
    
    if os.name == 'nt':
        # 로컬(Windows)은 편의를 위해 HTTP로 실행
        logger.info("TRBrain Flask Server starting on port 19990 (HTTP mode for local)...")
        socketio.run(app, host='0.0.0.0', port=19990, debug=False)
    else:
        # 서버(Linux 등)는 보안을 위해 HTTPS로 실행
        logger.info("TRBrain Flask Server starting on port 19990 (HTTPS enabled)...")
        socketio.run(app, host='0.0.0.0', port=19990, debug=False, ssl_context='adhoc', allow_unsafe_werkzeug=True)

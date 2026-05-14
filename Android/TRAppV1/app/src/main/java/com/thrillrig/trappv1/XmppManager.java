package com.thrillrig.trappv1;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import java.util.ArrayList;
import java.util.List;

import org.jivesoftware.smack.AbstractXMPPConnection;
import org.jivesoftware.smack.ConnectionConfiguration;
import org.jivesoftware.smack.ConnectionListener;
import org.jivesoftware.smack.ReconnectionManager;
import org.jivesoftware.smack.XMPPConnection;
import org.jivesoftware.smackx.ping.PingManager;
import org.jivesoftware.smack.chat2.Chat;
import org.jivesoftware.smack.chat2.ChatManager;
import org.jivesoftware.smack.chat2.IncomingChatMessageListener;
import org.jivesoftware.smack.packet.Message;
import org.jivesoftware.smack.tcp.XMPPTCPConnection;
import org.jivesoftware.smack.tcp.XMPPTCPConnectionConfiguration;
import org.jxmpp.jid.EntityBareJid;
import org.jxmpp.jid.impl.JidCreate;

public class XmppManager {
    private static final String TAG = "XmppManager";
    private static XmppManager instance;
    private AbstractXMPPConnection connection;
    private final List<ConnectionStatusListener> statusListeners = new ArrayList<>();
    private Handler mainHandler = new Handler(Looper.getMainLooper());
    private boolean isConnecting = false;
    private Context context;

    public interface ConnectionStatusListener {
        void onStatusChanged(boolean connected);
        void onMessageReceived(String from, String body);
    }

    private XmppManager() {}

    public static synchronized XmppManager getInstance() {
        if (instance == null) {
            instance = new XmppManager();
        }
        return instance;
    }

    public void init(Context context) {
        this.context = context.getApplicationContext();
        Logger.i(this.context, TAG, "XmppManager initialized");
    }

    public void addStatusListener(ConnectionStatusListener listener) {
        synchronized (statusListeners) {
            if (!statusListeners.contains(listener)) {
                statusListeners.add(listener);
            }
        }
    }

    public void removeStatusListener(ConnectionStatusListener listener) {
        synchronized (statusListeners) {
            statusListeners.remove(listener);
        }
    }

    public int getListenerCount() {
        synchronized (statusListeners) {
            return statusListeners.size();
        }
    }

    public void connect(String user, String pass, String host) {
        if (isConnected() || isConnecting) {
            Logger.i(context, TAG, "Already connected or connecting. Skipping connect request.");
            return;
        }

        Log.i("ThrillRig_Conn", "====================================================");
        Log.i("ThrillRig_Conn", "[XMPP 접속 시도]");
        Log.i("ThrillRig_Conn", " - User JID: " + user + "@" + host);
        Log.i("ThrillRig_Conn", " - Server Host: " + host);
        Log.i("ThrillRig_Conn", " - Server Port: 5222");
        Log.i("ThrillRig_Conn", "====================================================");

        Logger.i(context, TAG, "Starting connection to " + host + " for user " + user);
        isConnecting = true;
        new Thread(() -> {
            try {
                XMPPTCPConnectionConfiguration config = XMPPTCPConnectionConfiguration.builder()
                        .setXmppDomain(host)
                        .setHost(host)
                        .setPort(5222)
                        .setSecurityMode(ConnectionConfiguration.SecurityMode.disabled)
                        .setConnectTimeout(5000)
                        .setUsernameAndPassword(user, pass)
                        .setResource("TRAppV1_Android")
                        .build();

                connection = new XMPPTCPConnection(config);
                
                // Message Listener
                ChatManager chatManager = ChatManager.getInstanceFor(connection);
                chatManager.addIncomingListener(new IncomingChatMessageListener() {
                    @Override
                    public void newIncomingMessage(EntityBareJid from, Message message, Chat chat) {
                        Log.d(TAG, "New message from " + from + ": " + message.getBody());
                        notifyMessage(from.toString(), message.getBody());
                    }
                });

                // Enable Automatic Reconnection
                ReconnectionManager reconnectionManager = ReconnectionManager.getInstanceFor(connection);
                reconnectionManager.enableAutomaticReconnection();
                
                // Enable Ping to keep connection alive
                PingManager pingManager = PingManager.getInstanceFor(connection);
                pingManager.setPingInterval(60); // Ping every 60 seconds
                
                connection.addConnectionListener(new ConnectionListener() {
                    @Override
                    public void connected(XMPPConnection connection) {
                        Log.d(TAG, "Connected");
                    }

                    @Override
                    public void authenticated(XMPPConnection connection, boolean resumed) {
                        Log.d(TAG, "Authenticated");
                        notifyStatus(true);
                    }

                    @Override
                    public void connectionClosed() {
                        Log.d(TAG, "Connection closed");
                        notifyStatus(false);
                    }

                    @Override
                    public void connectionClosedOnError(Exception e) {
                        Log.e(TAG, "Connection closed on error", e);
                        notifyStatus(false);
                    }
                });

                connection.connect().login();
                isConnecting = false;
                Log.i("ThrillRig_Conn", "[XMPP 로그인 성공] JID: " + user + "@" + host);
                Logger.i(context, TAG, "Login successful");
            } catch (Exception e) {
                Logger.e(context, TAG, "Connection/Login failed", e);
                isConnecting = false;
                notifyStatus(false);
            }
        }).start();
    }

    private void notifyStatus(boolean connected) {
        mainHandler.post(() -> {
            synchronized (statusListeners) {
                for (ConnectionStatusListener listener : statusListeners) {
                    listener.onStatusChanged(connected);
                }
            }
        });
    }

    private void notifyMessage(String from, String body) {
        mainHandler.post(() -> {
            synchronized (statusListeners) {
                for (ConnectionStatusListener listener : statusListeners) {
                    listener.onMessageReceived(from, body);
                }
            }
        });
    }

    public void disconnect() {
        if (connection != null && connection.isConnected()) {
            connection.disconnect();
        }
    }

    public boolean isConnected() {
        return connection != null && connection.isAuthenticated();
    }

    public void sendMessage(String to, String body) {
        if (!isConnected()) {
            Logger.e(context, TAG, "Cannot send message: Not connected", null);
            return;
        }
        new Thread(() -> {
            try {
                ChatManager chatManager = ChatManager.getInstanceFor(connection);
                EntityBareJid jid = JidCreate.entityBareFrom(to);
                Chat chat = chatManager.chatWith(jid);
                chat.send(body);
                Logger.i(context, TAG, "Message sent to " + to + ": " + body);
            } catch (Exception e) {
                Logger.e(context, TAG, "Failed to send message", e);
            }
        }).start();
    }
}

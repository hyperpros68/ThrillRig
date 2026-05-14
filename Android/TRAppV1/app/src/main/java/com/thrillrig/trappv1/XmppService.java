package com.thrillrig.trappv1;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.Build;
import android.os.IBinder;
import android.util.Log;
import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;

public class XmppService extends Service {
    private static final String TAG = "XmppService";
    private static final String CHANNEL_ID_SERVICE = "XmppServiceChannel_v2";
    private static final String CHANNEL_ID_MESSAGE = "XmppMessageChannel_v2";
    private static final int SERVICE_NOTIFICATION_ID = 1;
    private static final int MESSAGE_NOTIFICATION_ID = 100;
    public static final String ACTION_RESET_COUNT = "com.thrillrig.trappv1.RESET_COUNT";
    
    private XmppManager xmppManager;
    private int unreadCount = 0;

    @Override
    public void onCreate() {
        super.onCreate();
        xmppManager = XmppManager.getInstance();
        xmppManager.init(this);
        Logger.i(this, TAG, "Service Created");
        setupMessageListener();
    }

    private void setupMessageListener() {
        xmppManager.addStatusListener(new XmppManager.ConnectionStatusListener() {
            @Override
            public void onStatusChanged(boolean connected) {
                // Connection status updates are handled by the activity via shared ViewModel if needed,
                // but service can also log it.
            }

            @Override
            public void onMessageReceived(String from, String body) {
                // If Activity is in foreground (listenerCount > 1), don't increment badge
                if (xmppManager.getListenerCount() > 1) {
                    Logger.i(XmppService.this, TAG, "App in foreground, skipping badge increment");
                    return;
                }
                
                unreadCount++;
                Logger.i(XmppService.this, TAG, "Message received from " + from + ", unreadCount: " + unreadCount);
                updateNotification("New Message from " + from, body, true);
            }
        });
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent != null && ACTION_RESET_COUNT.equals(intent.getAction())) {
            unreadCount = 0;
            Logger.i(this, TAG, "Unread count reset by Action");
            
            // Cancel the message notification to clear badge
            NotificationManager manager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
            if (manager != null) {
                manager.cancel(MESSAGE_NOTIFICATION_ID);
            }
            
            updateNotification("ThrillRig Monitoring", "Monitoring XMPP messages in background", false);
        } else {
            createNotificationChannel();
            Logger.i(this, TAG, "Service started via onStartCommand");
            updateNotification("ThrillRig Monitoring", "Connected and ready", false);
        }

        // Ensure XMPP is connected
        if (!xmppManager.isConnected()) {
            xmppManager.connect("test_trmgr_001", "trmgr!@#$", "59.187.96.23");
        }

        return START_STICKY;
    }

    private void updateNotification(String title, String content, boolean highPriority) {
        Intent notificationIntent = new Intent(this, MainActivity.class);
        notificationIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
        PendingIntent pendingIntent = PendingIntent.getActivity(this,
                0, notificationIntent, PendingIntent.FLAG_IMMUTABLE);

        String channelId = highPriority ? CHANNEL_ID_MESSAGE : CHANNEL_ID_SERVICE;

        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, channelId)
                .setContentTitle(title)
                .setContentText(content)
                .setSmallIcon(android.R.drawable.ic_dialog_email)
                .setContentIntent(pendingIntent)
                .setOngoing(!highPriority)
                .setAutoCancel(highPriority);

        if (unreadCount > 0 && highPriority) {
            builder.setNumber(unreadCount);
        }

        if (highPriority) {
            builder.setPriority(NotificationCompat.PRIORITY_HIGH)
                   .setDefaults(Notification.DEFAULT_ALL);
        } else {
            builder.setPriority(NotificationCompat.PRIORITY_LOW);
        }

        Notification notification = builder.build();
        
        NotificationManager manager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        if (manager == null) return;

        if (!highPriority) {
            startForeground(SERVICE_NOTIFICATION_ID, notification);
        } else {
            manager.notify(MESSAGE_NOTIFICATION_ID, notification);
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationManager manager = getSystemService(NotificationManager.class);
            if (manager == null) return;

            // 1. Service Channel (No Badge)
            NotificationChannel serviceChannel = new NotificationChannel(
                    CHANNEL_ID_SERVICE,
                    "Xmpp Monitoring Service",
                    NotificationManager.IMPORTANCE_LOW
            );
            serviceChannel.setShowBadge(false); // Disable badge for background service
            manager.createNotificationChannel(serviceChannel);

            // 2. Message Channel (Show Badge)
            NotificationChannel messageChannel = new NotificationChannel(
                    CHANNEL_ID_MESSAGE,
                    "Xmpp New Messages",
                    NotificationManager.IMPORTANCE_HIGH
            );
            messageChannel.setShowBadge(true); // Enable badge for actual messages
            manager.createNotificationChannel(messageChannel);
        }
    }
}

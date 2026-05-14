package com.thrillrig.trappv1;

import android.content.Context;
import android.util.Log;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

public class Logger {
    private static final String TAG = "TRLogger";
    private static final String FILE_NAME = "thrillrig_log.txt";
    private static final long MAX_FILE_SIZE = 1024 * 1024; // 1MB

    public static void i(Context context, String tag, String message) {
        writeLog(context, "INFO", tag, message);
    }

    public static void e(Context context, String tag, String message, Throwable e) {
        String errorMessage = message + (e != null ? " | Error: " + e.getMessage() : "");
        writeLog(context, "ERROR", tag, errorMessage);
    }

    private static synchronized void writeLog(Context context, String level, String tag, String message) {
        if (context == null) return;

        String timeStamp = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault()).format(new Date());
        String logLine = String.format("[%s] [%s] [%s] %s\n", timeStamp, level, tag, message);

        // Also output to Android Logcat
        if (level.equals("ERROR")) {
            Log.e(tag, message);
        } else {
            Log.d(tag, message);
        }

        try {
            File logFile = new File(context.getFilesDir(), FILE_NAME);
            
            // Check file size and delete if it exceeds limit (Rotation)
            if (logFile.exists() && logFile.length() > MAX_FILE_SIZE) {
                logFile.delete();
            }

            BufferedWriter writer = new BufferedWriter(new FileWriter(logFile, true));
            writer.append(logLine);
            writer.close();
        } catch (IOException e) {
            Log.e(TAG, "Failed to write log to file", e);
        }
    }
    
    public static String getLogFilePath(Context context) {
        return new File(context.getFilesDir(), FILE_NAME).getAbsolutePath();
    }
}

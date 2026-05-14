package com.thrillrig.trappv1;

import android.Manifest;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.drawerlayout.widget.DrawerLayout;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.NavController;
import androidx.navigation.Navigation;
import androidx.navigation.ui.AppBarConfiguration;
import androidx.navigation.ui.NavigationUI;

import com.google.android.material.navigation.NavigationView;
import com.google.android.material.snackbar.Snackbar;
import com.thrillrig.trappv1.databinding.ActivityMainBinding;
import com.thrillrig.trappv1.ui.home.HomeViewModel;

public class MainActivity extends AppCompatActivity {

    private AppBarConfiguration mAppBarConfiguration;
    private ActivityMainBinding binding;
    private TextView statusTextView;
    private MenuItem statusItem;
    private HomeViewModel homeViewModel;
    private XmppManager.ConnectionStatusListener xmppListener;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // 항상 로그인 화면을 먼저 보여주기 위해 자동 로그인 체크 로직 제거
        /*
        if (!isLoggedIn()) {
            startActivity(new Intent(this, LoginActivity.class));
            finish();
            return;
        }
        */

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        setSupportActionBar(binding.appBarMain.toolbar);
        binding.appBarMain.fab.setOnClickListener(view -> {
            XmppManager.getInstance().sendMessage("trwww@59.187.96.23", "Hello");
            Snackbar.make(view, "XMPP: 'Hello' sent to Service", Snackbar.LENGTH_LONG)
                    .setAnchorView(R.id.fab).show();
        });

        DrawerLayout drawer = binding.drawerLayout;
        NavigationView navigationView = binding.navView;

        // 2. 역할에 따른 메뉴 필터링
        filterMenuByRole(navigationView.getMenu());

        mAppBarConfiguration = new AppBarConfiguration.Builder(
                R.id.nav_home, R.id.nav_manager, R.id.nav_agent, R.id.nav_shop, R.id.nav_settings, R.id.nav_logout)
                .setOpenableLayout(drawer)
                .build();
        NavController navController = Navigation.findNavController(this, R.id.nav_host_fragment_content_main);
        NavigationUI.setupActionBarWithNavController(this, navController, mAppBarConfiguration);
        NavigationUI.setupWithNavController(navigationView, navController);

        // 로그아웃 직접 처리
        navigationView.getMenu().findItem(R.id.nav_logout).setOnMenuItemClickListener(item -> {
            performLogout();
            return true;
        });
        
        homeViewModel = new ViewModelProvider(this).get(HomeViewModel.class);

        // Header 정보 설정
        View headerView = navigationView.getHeaderView(0);
        setupHeaderInfo(headerView);
        statusTextView = headerView.findViewById(R.id.statusTextView);

        setupXmppListener();
        updateStatusUI(XmppManager.getInstance().isConnected());

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.POST_NOTIFICATIONS}, 101);
            }
        }

        // Start Foreground Service
        Intent serviceIntent = new Intent(this, XmppService.class);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(serviceIntent);
        } else {
            startService(serviceIntent);
        }
    }

    private boolean isLoggedIn() {
        SharedPreferences pref = getSharedPreferences("ThrillRig", Context.MODE_PRIVATE);
        return pref.getBoolean("isLoggedIn", false);
    }

    private void filterMenuByRole(Menu menu) {
        SharedPreferences pref = getSharedPreferences("ThrillRig", Context.MODE_PRIVATE);
        String role = pref.getString("userRole", "");

        menu.findItem(R.id.nav_manager).setVisible(role.equals("MANAGER"));
        menu.findItem(R.id.nav_agent).setVisible(role.equals("AGENT"));
        menu.findItem(R.id.nav_shop).setVisible(role.equals("SHOP"));
    }

    private void setupHeaderInfo(View headerView) {
        SharedPreferences pref = getSharedPreferences("ThrillRig", Context.MODE_PRIVATE);
        TextView tvName = headerView.findViewById(R.id.tv_user_name);
        TextView tvRole = headerView.findViewById(R.id.tv_user_role);

        tvName.setText(pref.getString("userName", "Unknown"));
        tvRole.setText(pref.getString("userRole", "Guest"));
    }

    private void setupXmppListener() {
        if (xmppListener == null) {
            xmppListener = new XmppManager.ConnectionStatusListener() {
                @Override
                public void onStatusChanged(boolean connected) {
                    runOnUiThread(() -> updateStatusUI(connected));
                }

                @Override
                public void onMessageReceived(String from, String body) {
                    runOnUiThread(() -> {
                        Snackbar.make(binding.getRoot(), "Message from " + from + ": " + body, Snackbar.LENGTH_LONG).show();
                        homeViewModel.setText(body);
                    });
                }
            };
        }
        XmppManager.getInstance().addStatusListener(xmppListener);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (xmppListener != null) {
            XmppManager.getInstance().removeStatusListener(xmppListener);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        Intent intent = new Intent(this, XmppService.class);
        intent.setAction(XmppService.ACTION_RESET_COUNT);
        startService(intent);
        setupXmppListener();
        updateStatusUI(XmppManager.getInstance().isConnected());
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (xmppListener != null) {
            XmppManager.getInstance().removeStatusListener(xmppListener);
        }
    }

    private void updateStatusUI(boolean connected) {
        if (statusTextView != null) {
            statusTextView.setText(connected ? "XMPP: Connected" : "XMPP: Disconnected");
            statusTextView.setTextColor(connected ? Color.GREEN : Color.RED);
        }
        if (statusItem != null) {
            Drawable dot = statusItem.getIcon();
            if (dot != null) {
                dot.mutate();
                dot.setTint(connected ? Color.GREEN : Color.RED);
            }
        }
        invalidateOptionsMenu();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main, menu);
        statusItem = menu.findItem(R.id.action_status);
        updateStatusUI(XmppManager.getInstance().isConnected());
        return true;
    }

    @Override
    public boolean onSupportNavigateUp() {
        NavController navController = Navigation.findNavController(this, R.id.nav_host_fragment_content_main);
        return NavigationUI.navigateUp(navController, mAppBarConfiguration)
                || super.onSupportNavigateUp();
    }

    private void performLogout() {
        // 1. XMPP 연결 종료
        XmppManager.getInstance().disconnect();

        // 2. 저장된 로그인 정보 삭제
        SharedPreferences pref = getSharedPreferences("ThrillRig", Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = pref.edit();
        // isLoggedIn 등 핵심 정보만 삭제 (아이디/비번은 유지하고 싶으면 그대로 둠)
        editor.remove("isLoggedIn");
        editor.remove("userRole");
        editor.remove("userName");
        editor.apply();

        // 3. 로그인 화면으로 이동
        Intent intent = new Intent(this, LoginActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        startActivity(intent);
        finish();
    }
}
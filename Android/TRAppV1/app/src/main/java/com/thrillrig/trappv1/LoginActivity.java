package com.thrillrig.trappv1;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;
import android.widget.ArrayAdapter;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.thrillrig.trappv1.databinding.ActivityLoginBinding;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;

import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.FormBody;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

public class LoginActivity extends AppCompatActivity {

    private ActivityLoginBinding binding;
    private static final String BASE_URL = "http://59.187.96.23:9990/api/"; 
    private OkHttpClient client = new OkHttpClient();

    private final String[] rolesKr = {"매니저", "총판", "매장"};
    private final String[] rolesEn = {"MANAGER", "AGENT", "SHOP"};

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityLoginBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        ArrayAdapter<String> adapter = new ArrayAdapter<>(this, android.R.layout.simple_dropdown_item_1line, rolesKr);
        binding.roleSelector.setAdapter(adapter);

        loadSavedLoginInfo();

        binding.btnLogin.setOnClickListener(v -> performLogin());

        binding.tvFindPassword.setOnClickListener(v -> 
            Toast.makeText(this, "준비 중인 기능입니다.", Toast.LENGTH_SHORT).show());
        
        binding.tvSignUp.setOnClickListener(v -> 
            Toast.makeText(this, "준비 중인 기능입니다.", Toast.LENGTH_SHORT).show());
    }

    private void loadSavedLoginInfo() {
        SharedPreferences pref = getSharedPreferences("ThrillRig", Context.MODE_PRIVATE);
        boolean rememberMe = pref.getBoolean("rememberMe", false);
        if (rememberMe) {
            String savedRoleEn = pref.getString("savedRole", "");
            String savedId = pref.getString("savedId", "");
            String savedPwd = pref.getString("savedPwd", ""); // 비밀번호 불러오기
            
            String savedRoleKr = "선택해주세요";
            for (int i = 0; i < rolesEn.length; i++) {
                if (rolesEn[i].equals(savedRoleEn)) {
                    savedRoleKr = rolesKr[i];
                    break;
                }
            }
            
            binding.roleSelector.setText(savedRoleKr, false);
            binding.etLoginId.setText(savedId);
            binding.etPassword.setText(savedPwd); // 비밀번호 세팅
            binding.cbRememberMe.setChecked(true);
        }
    }

    private void performLogin() {
        String roleKr = binding.roleSelector.getText().toString();
        String id = binding.etLoginId.getText().toString().trim();
        String pwd = binding.etPassword.getText().toString().trim();

        if (roleKr.equals("선택해주세요") || id.isEmpty() || pwd.isEmpty()) {
            Toast.makeText(this, "모든 정보를 입력해주세요.", Toast.LENGTH_SHORT).show();
            return;
        }

        String roleEn = "";
        for (int i = 0; i < rolesKr.length; i++) {
            if (rolesKr[i].equals(roleKr)) {
                roleEn = rolesEn[i];
                break;
            }
        }

        binding.btnLogin.setEnabled(false);
        binding.btnLogin.setText("로그인 중...");

        RequestBody formBody = new FormBody.Builder()
                .add("login_id", id)
                .add("password", pwd)
                .add("role", roleEn)
                .build();

        Log.i("ThrillRig_Conn", "====================================================");
        Log.i("ThrillRig_Conn", "[API 로그인 시도]");
        Log.i("ThrillRig_Conn", " - API URL: " + BASE_URL + "login.php");
        Log.i("ThrillRig_Conn", " - Login ID: " + id);
        Log.i("ThrillRig_Conn", " - Role: " + roleEn);
        Log.i("ThrillRig_Conn", "====================================================");

        Request request = new Request.Builder()
                .url(BASE_URL + "login.php")
                .post(formBody)
                .build();

        final String finalRoleEn = roleEn;
        final String finalPwd = pwd; // 저장용
        client.newCall(request).enqueue(new Callback() {
            @Override
            public void onFailure(Call call, IOException e) {
                runOnUiThread(() -> {
                    // 서버 연결 실패 시에도 1.5초 후 재시도 가능하게 함
                    binding.btnLogin.postDelayed(() -> {
                        if (binding != null && binding.btnLogin != null) {
                            binding.btnLogin.setEnabled(true);
                            binding.btnLogin.setText("로그인");
                        }
                    }, 1500);
                    Toast.makeText(LoginActivity.this, "서버 연결 실패", Toast.LENGTH_LONG).show();
                });
            }

            @Override
            public void onResponse(Call call, Response response) throws IOException {
                final String responseData = response.body().string();
                runOnUiThread(() -> {
                    // 여기에 있던 즉시 활성화 코드를 제거했습니다.
                    try {
                        JSONObject json = new JSONObject(responseData);
                        if (json.getBoolean("success")) {
                            saveLoginInfo(json, finalRoleEn, id, finalPwd);
                            startActivity(new Intent(LoginActivity.this, MainActivity.class));
                            finish();
                            // 성공 시에는 화면이 넘어가므로 버튼을 다시 활성화하지 않음
                        } else {
                            Toast.makeText(LoginActivity.this, json.getString("message"), Toast.LENGTH_SHORT).show();
                            // 실패 시 1.5초 후에 버튼을 다시 활성화 (연타 방지)
                            binding.btnLogin.postDelayed(() -> {
                                if (binding != null && binding.btnLogin != null) {
                                    binding.btnLogin.setEnabled(true);
                                    binding.btnLogin.setText("로그인");
                                }
                            }, 1500);
                        }
                    } catch (JSONException e) {
                        Toast.makeText(LoginActivity.this, "응답 파싱 실패", Toast.LENGTH_SHORT).show();
                        binding.btnLogin.setEnabled(true);
                        binding.btnLogin.setText("로그인");
                    }
                });
            }
        });
    }

    private void saveLoginInfo(JSONObject json, String roleEn, String id, String pwd) throws JSONException {
        SharedPreferences pref = getSharedPreferences("ThrillRig", MODE_PRIVATE);
        SharedPreferences.Editor editor = pref.edit();
        
        // 자동 로그인 방지를 위해 isLoggedIn 플래그는 저장하지 않음
        // editor.putBoolean("isLoggedIn", true);
        editor.putInt("userId", json.getInt("user_id"));
        editor.putString("loginId", json.getString("login_id"));
        editor.putString("userName", json.getString("name"));
        editor.putString("userRole", json.getString("role"));

        boolean rememberMe = binding.cbRememberMe.isChecked();
        editor.putBoolean("rememberMe", rememberMe);
        if (rememberMe) {
            editor.putString("savedRole", roleEn);
            editor.putString("savedId", id);
            editor.putString("savedPwd", pwd); // 비밀번호 저장
        } else {
            editor.remove("savedRole");
            editor.remove("savedId");
            editor.remove("savedPwd"); // 체크 해제 시 비밀번호 삭제
        }
        
        editor.apply();
    }
}

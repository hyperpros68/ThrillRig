package com.thrillrig.trappv1.ui;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import com.thrillrig.trappv1.LoginActivity;
import com.thrillrig.trappv1.databinding.FragmentCommonBinding;

public class CommonFragment extends Fragment {
    private FragmentCommonBinding binding;

    public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        binding = FragmentCommonBinding.inflate(inflater, container, false);

        binding.btnLogout.setOnClickListener(v -> performLogout());

        return binding.getRoot();
    }

    private void performLogout() {
        SharedPreferences pref = requireActivity().getSharedPreferences("ThrillRig", Context.MODE_PRIVATE);
        pref.edit().clear().apply();

        Intent intent = new Intent(requireActivity(), LoginActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        startActivity(intent);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }
}

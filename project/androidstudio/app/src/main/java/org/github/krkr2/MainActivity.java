package org.github.krkr2;

import android.Manifest;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.view.WindowManager;

import androidx.core.content.ContextCompat;

import org.cocos2dx.hellocpp.R;
import org.libsdl.app.SDLAudioManager;
import org.tvp.kirikiri2.KR2Activity;

public class MainActivity extends KR2Activity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.setEnableVirtualButton(false);
        super.onCreate(savedInstanceState);

        // Workaround in https://stackoverflow.com/questions/16283079/re-launch-of-activity-on-home-button-but-only-the-first-time/16447508
        if (!isTaskRoot()) {
            // Android launched another instance of the root activity into an existing task
            //  so just quietly finish and go away, dropping the user back into the activity
            //  at the top of the stack (ie: the last state of this task)
            // Don't need to finish it again since it's finished in super.onCreate .
            return;
        }

        // Make sure we're running on Pie or higher to change cutout mode
        // Enable rendering into the cutout area
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            WindowManager.LayoutParams lp = getWindow().getAttributes();
            lp.layoutInDisplayCutoutMode =
                WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            getWindow().setAttributes(lp);
        }

        if (!checkStoragePermission()) {
            requestStoragePermission();
        }

        SDLAudioManager.nativeSetupJNI();
        SDLAudioManager.initialize();
//        SDLAudioManager.setContext(getContext());

    }

    @Override
    public void onDestroy() {
        super.onDestroy();
//        SDLAudioManager.release(this);
    }

    private boolean checkStoragePermission() {
        // 检查 MANAGE_EXTERNAL_STORAGE 权限
//        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
//            return Environment.isExternalStorageManager();
//        }
        return ContextCompat.checkSelfPermission(
            this, Manifest.permission.WRITE_EXTERNAL_STORAGE
        ) == PackageManager.PERMISSION_GRANTED;
    }

    // 请求用户授予 MANAGE_EXTERNAL_STORAGE 权限
    private boolean requestStoragePermission() {
        boolean r = false;

//        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
//            registerForActivityResult(
//                ActivityResultContracts.RequestPermission()
//            ) { result -> r = result }
//                .launch(Manifest.permission.WRITE_EXTERNAL_STORAGE)
//            return r;
//        }

//        val startForResult = registerForActivityResult(
//            ActivityResultContracts.StartActivityForResult()
//        ) { result ->
//            r = result.resultCode == 1 && checkStoragePermission()
//        }

        new AlertDialog.Builder(this)
            .setTitle(getString(R.string.request_storage_permission_title))
            .setMessage(getString(R.string.request_storage_permission))
            .setPositiveButton(getString(R.string.ok), new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
//                    startForResult.launch(
//                            Intent(
//                                    Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
//                                    Uri.fromParts("package", packageName, null)
//                            )
//                    );
                }
            })
            .setNegativeButton(getString(R.string.cancel), null)
            .show();

        return r;
    }

}

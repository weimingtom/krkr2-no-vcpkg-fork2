/****************************************************************************
Copyright (c) 2015-2016 Chukong Technologies Inc.
Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
 
http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/
package org.tvp.kirikiri2;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;

import org.cocos2dx.lib.Cocos2dxActivity;
import org.libsdl.app.SDLActivity;

import android.os.Build;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.Toast;

//FIXME:TODO:If want to change this class name, see core/environ/android/AndroidUtils.cpp, KR2ActJavaPath, KR2Activity
public class KR2Activity extends Cocos2dxActivity {
    private int stateStarted_ = 0;
    
	@Override
    protected void onCreate(Bundle savedInstanceState) {
        super.setEnableVirtualButton(true);
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
//        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
//            // Enable rendering into the cutout area
//            WindowManager.LayoutParams lp = getWindow().getAttributes();
//            lp.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
//            getWindow().setAttributes(lp);
//        }
        // DO OTHER INITIALIZATION BELOW
        sInstance = this;
        
        int stateStarted = 0;
        if (savedInstanceState != null) {
            stateStarted = savedInstanceState.getInt(STATE_STARTED, 0);
            stateStarted_ = stateStarted;
        }

        //FIXME:If you want to skip permission check, you need to add android:requestLegacyExternalStorage, 
        //but don't recommended, I use both
        if (stateStarted == 0) {
            checkPermission();
        }
    }

    
    static public KR2Activity sInstance;

    //FIXME:TODO:don't remove this function 'GetInstance', see core/environ/android/AndroidUtils.cpp, static jobject GetKR2ActInstance()
    static public KR2Activity GetInstance() {
        return sInstance;
    }
    
    
    
    
    
    
    
    
    
    
    
    
    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putInt(STATE_STARTED, 1);
    }

    private static final String STATE_STARTED = "STATE_STARTED";
    //https://blog.csdn.net/zuo_er_lyf/article/details/82659426
    //https://www.dev2qa.com/android-read-write-external-storage-file-example/
    private final int REQUEST_CODE_WRITE_EXTERNAL_STORAGE_PERMISSION = 100;
    private void checkPermission() {
    	if (Build.VERSION.SDK_INT >= 23) { //for Android 6.0
	        // Check whether this app has write external storage permission or not.
	    	int writeExternalStoragePermission = this.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE);
	        // If do not grant write external storage permission.
	        if (writeExternalStoragePermission!= PackageManager.PERMISSION_GRANTED) {
	            // Request user to grant write external storage permission.
	            this.requestPermissions(new String[]{
	                    Manifest.permission.WRITE_EXTERNAL_STORAGE}, REQUEST_CODE_WRITE_EXTERNAL_STORAGE_PERMISSION);
	        } else {
	        }
    	} else {
    	}
    }
    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQUEST_CODE_WRITE_EXTERNAL_STORAGE_PERMISSION) {
            int grantResultsLength = grantResults.length;
            if (grantResultsLength > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                Toast.makeText(getApplicationContext(), "You grant write external storage permission. Please restart to continue.", Toast.LENGTH_LONG).show();
                finish();
            } else {
                Toast.makeText(getApplicationContext(), "You denied write external storage permission.", Toast.LENGTH_LONG).show();
            }
        }
    }
}

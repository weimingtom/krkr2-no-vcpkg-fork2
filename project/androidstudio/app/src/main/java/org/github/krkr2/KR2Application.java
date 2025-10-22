package org.github.krkr2;

import android.app.Application;
import android.content.Context;

public class KR2Application extends Application {
    @Override
    public void onCreate() {
        super.onCreate();
        context = this.getApplicationContext();
    }

    public static Context context;
}

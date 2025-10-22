package org.github.krkr2;

import android.app.Application;
import android.content.Context;

public class KR2Application extends Application {
	public static Context context;
	
	@Override
	public void onCreate() {
		super.onCreate();
		context = this.getApplicationContext();
	}

}

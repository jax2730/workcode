/*
 * Created by VisualGDB. Based on hello-jni example.
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package net.pixelgame.xxsj;

import android.content.Intent;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.MotionEvent;
import android.view.View.OnTouchListener;
import android.view.WindowManager.LayoutParams;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.PopupWindow;
import android.widget.TextView;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.view.InputDevice;
import android.content.DialogInterface;
import android.app.AlertDialog;
//import com.tencent.bugly.*;
//import com.tencent.bugly.crashreport.*;
import android.os.Environment;
import android.view.View.OnClickListener ;

import java.io.File;
import java.util.Timer;
import java.util.TimerTask;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;


public class TcgMainActivity extends Activity {

	 static {
		 System.loadLibrary("fmod");
		 System.loadLibrary("fmodstudio");
         System.loadLibrary("PadBO2");
     }
    GameView gameView;

	static private volatile EGLContext mSharedEglContext;
    static private volatile EGLConfig  mSharedEglConfig;
    static private volatile EGLDisplay mSharedEglDisplay;
    static private EGLContext mEglContext;

	ImageView joystick_mov,joystick_dir;
	Button button1,button2, button3, button4, button5;
    static TextView Fpslabel;
	static TextView DrawCallLabel;
	static TextView TriangleLabel;
	static TextView ResMemUsageLabel;
    static TextView CamPoslabel;

	static int nFps = 0, nDrawCall=0, nTriangleCnt=0;
	static String memUsage, camPos;
					
	Handler mTimeHandler = new Handler() {
			public void handleMessage(android.os.Message msg) {
				if (msg.what == 0) {					
					gameView.queueEvent(new Runnable(){
						@Override	public void run(){ 
						 nFps = GameLib.getFPS();
						 nDrawCall = GameLib.getDrawCall();
						 nTriangleCnt = GameLib.getTriangleCnt();
						 camPos = GameLib.getCameraPosition();
						 memUsage = GameLib.getResMemUsageStr();
						}});  
					
					Fpslabel.setText("fps: " + nFps);
					DrawCallLabel.setText("draw: " + nDrawCall);
					float fValue = nTriangleCnt / 10000.0f;
					TriangleLabel.setText("trian: " + String.format("%.2f w",fValue));
					sendEmptyMessageDelayed(0, 1000);
					CamPoslabel.setText("cam: " + camPos);
					ResMemUsageLabel.setText(memUsage);
				}
			}
		};

    public static TcgMainActivity mainActivity = null;

    @Override protected void onCreate(Bundle icicle) {
		super.onCreate(icicle);
		//CrashReport.initCrashReport( getApplicationContext(), "900033396", false);
		//CrashReport.startCrashReport();
		//CrashReport.testNativeCrash();
		mainActivity = this;
		mainActivity.regEnv();

		// this section is only used for getEchoRenderMode(), change asset path here won't
		// affect the engine asset path.
		{
			String[] variables = new String[2];
			String assetPath = "";
			variables[0] = assetPath;

			String dataFilePath;
			{
				File externalCache = mainActivity.getExternalCacheDir();
				File dataDir = new File(externalCache.getParent(),"asset");
				if(!dataDir.exists())
					dataDir.mkdirs();
				dataFilePath = dataDir.getPath() + "/";
			}
			variables[1] = dataFilePath;

			GameLib.init(variables);
		}
		int renderMode;
		renderMode = GameLib.getEchoRenderMode();

		/*if (renderMode == 3) //ECHO_DEVICE_GLES3
		{
			gameView = new GameView(getApplication());
			setContentView(gameView);
			int uiOptions = View.SYSTEM_UI_FLAG_LAYOUT_STABLE
					| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
					| View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
					| View.SYSTEM_UI_FLAG_HIDE_NAVIGATION // hide nav bar
					| View.SYSTEM_UI_FLAG_FULLSCREEN // hide status bar
					| View.SYSTEM_UI_FLAG_IMMERSIVE;
			gameView.setSystemUiVisibility(uiOptions);

			controlViewInit();
			mTimeHandler.sendEmptyMessageDelayed(0, 1000);

			org.fmod.FMOD.init(this);
		}*/

		// Both OpenGLES and Vulkan use GameActivity instead of GLSurfaceView.
		if (renderMode == 3 || renderMode == 4) //ECHO_DEVICE_VULKAN
		{
			// activate VulkanActivity and finish current activity.
			Intent intent = new Intent();
			intent.setClass(TcgMainActivity.this, MainActivity.class);
			intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK | Intent.FLAG_ACTIVITY_NEW_TASK);
			startActivity(intent);
			mainActivity.finish();
			mainActivity.runOnUiThread(GameLib::terminate);
			super.onDestroy();
		}
    }

	public void controlViewInit() {

		FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(  
        		FrameLayout.LayoutParams.MATCH_PARENT, 
        		FrameLayout.LayoutParams.MATCH_PARENT);  

        
		Context context = this.getApplicationContext();

        LayoutInflater iInflater = LayoutInflater.from(context);
        View v = iInflater.inflate(R.layout.control_panel, null);

        addContentView(v,params);
		 
		 joystick_mov = (ImageView)findViewById(R.id.joystick_mov);
		 joystick_dir = (ImageView)findViewById(R.id.joystick_dir);
		 button1 = (Button)findViewById(R.id.button1);
		 button2 = (Button)findViewById(R.id.button2);
	     button3  = (Button)findViewById(R.id.button3);	
		 button4  = (Button)findViewById(R.id.button4);
		 button5 = (Button)findViewById(R.id.button5);
		 Fpslabel = (TextView)findViewById(R.id.fps_text_view);	
		 DrawCallLabel = (TextView)findViewById(R.id.drawcall_text_view);	
		 TriangleLabel = (TextView)findViewById(R.id.triangle_text_view);
		 CamPoslabel = (TextView)findViewById(R.id.camera_position);
		 ResMemUsageLabel = (TextView)findViewById(R.id.res_mem_usage);

	     joystick_mov.setOnTouchListener(new ViewOnTouchListener());
	     joystick_dir.setOnTouchListener(new ViewOnTouchListener());

		 button3.setOnClickListener(new OnClickListener() {         
            @Override      
            public void onClick(View v) {         
                 // TODO Auto-generated method stub         
                CallInput("test");         
            }     
        }); 
		
		button4.setOnClickListener(new OnClickListener() {         
            @Override      
            public void onClick(View v) {         
                 // TODO Auto-generated method stub       
				gameView.queueEvent(new Runnable(){ @Override	public void run() {  GameLib.nextScene(); } });  			   
            }     
        }); 

		button5.setOnClickListener(new OnClickListener() {         
            @Override      
            public void onClick(View v) {         
                 // TODO Auto-generated method stub         
			   gameView.queueEvent(new Runnable(){ @Override	public void run() {   GameLib.lastScene(); } }); 			   
            }     
        });
		
	}

	

	private class ViewOnTouchListener implements OnTouchListener
    {
		boolean m_bDown = false;
		double m_r=100,m_btn_r=16;
		float m_centerX=0,m_centerY=0;
		int m_btnLeft=0,m_btnTop=0;
	    @Override
	    public boolean onTouch(View view, MotionEvent event) {

			Button tempBtn = (view == joystick_mov)? button1 : button2;
			if(event.getSource() == InputDevice.SOURCE_TOUCHSCREEN)
			{
				final int action = event.getActionMasked();
				final int actIdx = event.getActionIndex();
				final int pointId = (view == joystick_mov)? 100 : 200;
				final int pointCount = event.getPointerCount();
				final float x = event.getX(actIdx);
				final float y = event.getY(actIdx);

				if(action == MotionEvent.ACTION_DOWN) {
					m_bDown = true;  
					m_r = view.getWidth() / 2.0;
					m_centerX = view.getWidth()/2.0f;// + view.getLeft();
					m_centerY = view.getHeight()/2.0f;// + view.getTop();
					m_btn_r = tempBtn.getWidth()/2.0;
										
					RelativeLayout.LayoutParams lParams = (RelativeLayout.LayoutParams) tempBtn.getLayoutParams();
					m_btnLeft = lParams.leftMargin;
					m_btnTop  = lParams.topMargin;

					gameView.queueEvent(new Runnable(){ @Override	public void run() { GameLib.onTouch(0, pointId, pointCount, x, y);} });  
					//mainActivity.runOnUiThread(new Runnable(){ @Override	public void run() { GameLib.onTouch(0, pointId, pointCount, x, y);} });  
				}

				if(action == MotionEvent.ACTION_UP ||
					action ==  MotionEvent.ACTION_OUTSIDE ){
					m_bDown = false;

					RelativeLayout.LayoutParams layoutParams = (RelativeLayout.LayoutParams) tempBtn.getLayoutParams();
		            layoutParams.leftMargin = m_btnLeft;
		            layoutParams.topMargin = m_btnTop;
		            layoutParams.rightMargin = 0;
		            layoutParams.bottomMargin = 0;
		            tempBtn.setLayoutParams(layoutParams);

					gameView.queueEvent(new Runnable(){ @Override	public void run() { GameLib.onTouch(1, pointId, pointCount, x, y);} });
					//mainActivity.runOnUiThread(new Runnable(){ @Override	public void run() { GameLib.onTouch(1, pointId, pointCount, x, y);} });
				}
	 	            	
				if(action == MotionEvent.ACTION_MOVE && m_bDown == true) {
					double offset_x = x - m_centerX;
					double offset_y = y - m_centerY;
					final double distance = Math.pow( Math.pow( offset_x, 2) + Math.pow( offset_y, 2), 0.5);
					final double r = m_r - m_btn_r;
		
					if( distance > r ) {
						offset_x = offset_x * ( r / distance );
						offset_y = offset_y * ( r / distance );
					}

					RelativeLayout.LayoutParams layoutParams = (RelativeLayout.LayoutParams) tempBtn.getLayoutParams();
		            layoutParams.leftMargin = m_btnLeft + (int)offset_x;
		            layoutParams.topMargin = m_btnTop + (int)offset_y;
		            layoutParams.rightMargin = 0;
		            layoutParams.bottomMargin = 0;
		            tempBtn.setLayoutParams(layoutParams);

					final float xx = (float)(offset_x/m_r);
					final float yy = (float)(-offset_y/m_r);
					
					gameView.queueEvent(new Runnable(){ @Override	public void run() { GameLib.onTouch(2, pointId, pointCount, xx, yy); } });
					//mainActivity.runOnUiThread(new Runnable(){ @Override	public void run() { GameLib.onTouch(2, pointId, pointCount, xx, yy);} });
				}                
				//如果设置为false
				return true;
			}
			return false;
	    }
    }

    @Override protected void onDestroy()
    {
        super.onDestroy();
		org.fmod.FMOD.close();
    }

    @Override protected void onPause() {
        super.onPause();
		if (gameView != null) {
			gameView.setVisibility(View.GONE);
            gameView.queueEvent(new Runnable() {
                @Override
                public void run() {
                    GameLib.pause();
                }
            });
        }
    }

    @Override protected void onResume(){
        super.onResume();
		if (gameView != null) {
            gameView.queueEvent(new Runnable() {
                @Override
                public void run() {
                    GameLib.resume();
                }
            });
		}
	}
	
	@Override protected void onStart() {
		super.onStart();
		if (gameView != null) {
            gameView.queueEvent(new Runnable() {
                @Override
                public void run() {
                    setStateStart();
                }
            });
		}		
	}
	
	@Override protected void onStop() {		
		super.onStop();
		if (gameView != null) {
            gameView.queueEvent(new Runnable() {
                @Override
                public void run() {
                    setStateStop();
                }
            });
		}	
	}
	
	private native void setStateStart();
	private native void setStateStop();

	@Override
	public void onWindowFocusChanged(boolean hasFocus) {
		super.onWindowFocusChanged(hasFocus);
		if (hasFocus && gameView.getVisibility() == View.GONE) {
			gameView.setVisibility(View.VISIBLE);
		}
	}
	@Override
	public void onBackPressed() {
		new AlertDialog.Builder(this).setTitle("确认退出吗？ ")
				.setIcon(android.R.drawable.ic_dialog_info)
				.setPositiveButton("确定", new DialogInterface.OnClickListener() {

					@Override
					public void onClick(DialogInterface dialog, int which) {
						mainActivity.finish();
						mainActivity.runOnUiThread(new Runnable() {
							@Override
							public void run() {
								GameLib.terminate();
							}
						});
					}
				})
				.setNegativeButton("返回", new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						int SDK_INT = android.os.Build.VERSION.SDK_INT;
						if (SDK_INT >= 16) {
							mainActivity
									.getWindow()
									.getDecorView()
									.setSystemUiVisibility(
											View.SYSTEM_UI_FLAG_LAYOUT_STABLE
													| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
													| View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
													| View.SYSTEM_UI_FLAG_HIDE_NAVIGATION // hide nav bar
													| View.SYSTEM_UI_FLAG_FULLSCREEN // hide status bar
									);
						}
					}
				}).show();
	}


	
	public static TcgMainActivity instance() {
		return mainActivity;
	}

    static String game_string;
    static EditText _text_input;
	static Button _button_input;
	static PopupWindow _popupWindow;
	

    public static void CallInput(String txt) {


		game_string = txt;
		mainActivity.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				if (_popupWindow != null) {
					_popupWindow.dismiss();
					_popupWindow = null;
				}
				LayoutInflater layoutInflater = (LayoutInflater)mainActivity.getBaseContext()
						.getSystemService(Activity.LAYOUT_INFLATER_SERVICE);
				View popupView = layoutInflater.inflate(R.layout.input_panel, null);
				_popupWindow = new PopupWindow(popupView,
						LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);

				// Show our UI over NativeActivity window

				_popupWindow.showAtLocation(mainActivity.getWindow().getDecorView(), Gravity.TOP
						| Gravity.LEFT, 10, 10);

				_button_input = (Button) popupView
						.findViewById(R.id.bt_confirm_input);
				_button_input.setOnClickListener(new View.OnClickListener() {
					public void onClick(View v) {
						mainActivity.HideKeyboard();
					}
				});

				_popupWindow
						.setInputMethodMode(PopupWindow.INPUT_METHOD_NEEDED);
				_popupWindow.setOnDismissListener(new PopupWindow.OnDismissListener() {
					public void onDismiss() {
						//mainActivity.HideKeyboard();
					}
					});
				_popupWindow.setTouchable(true);
				_popupWindow.setFocusable(true);
				_popupWindow.getContentView().requestFocus();

				_popupWindow.update();

				// showInputPanel();
				_text_input = (EditText) popupView
						.findViewById(R.id.txt_input);
				//_text_input.setText(game_string);

				_text_input.setFocusableInTouchMode(true);
				_text_input.setFocusable(true);
				_text_input.requestFocus();


				TimerTask task = new TimerTask() {
					public void run() {
						InputMethodManager imm = (InputMethodManager) mainActivity
								.getSystemService(Context.INPUT_METHOD_SERVICE);
						while (!imm.showSoftInput(_text_input, 0)) {
							if (imm.showSoftInput(_text_input, 0)) {
								break;
							} else {
								continue;
							}
						}
					}
				};

				Timer timer = new Timer();
				timer.schedule(task, 500);
			}

		});
	}

    public void HideKeyboard() {
		_popupWindow.dismiss();
		final String ctrlText = _text_input.getText().toString();
		gameView.queueEvent(new Runnable(){ @Override	public void run() { GameLib.onTextProc( ctrlText );} });
		new Handler().postDelayed(new Runnable() {
			@SuppressLint("InlinedApi")
			public void run() {
				int SDK_INT = android.os.Build.VERSION.SDK_INT;
				if (SDK_INT >= 16) {
					mainActivity
							.getWindow()
							.getDecorView()
							.setSystemUiVisibility(
									View.SYSTEM_UI_FLAG_LAYOUT_STABLE
											| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
											| View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
											| View.SYSTEM_UI_FLAG_HIDE_NAVIGATION // hide nav bar
											| View.SYSTEM_UI_FLAG_FULLSCREEN // hide status bar
							);
				}
			}
		}, 800);

		// InputMethodManager imm =
		// (InputMethodManager)_activity.getSystemService(Context.INPUT_METHOD_SERVICE);
		// imm.hideSoftInputFromWindow(_text_input.getWindowToken(),0 );
	}

    public static void ApplyInput()
    {

    }

	public native void regEnv();

	public File getExternalCacheDir()
	{
		Context context = this.getApplicationContext();
		return context.getExternalCacheDir();
	}

    public static AssetManager GetAssetManager()
	{
		return GameApplication._assetinstance;
	}

	public static String GetSDCardPath()
	{
		return Environment.getExternalStorageDirectory().getPath();
	}

	public void saveEGLInfo(EGLConfig eglConfigs)
	{
		EGL10 mEgl = (EGL10) EGLContext.getEGL();
		mSharedEglDisplay = mEgl.eglGetCurrentDisplay();
		mSharedEglConfig = eglConfigs;
		mSharedEglContext = mEgl.eglGetCurrentContext();		
	}

	public static void SetShareContext(boolean value)
	{
		EGL10 mEgl = (EGL10) EGLContext.getEGL();
		if(value) 
		{
        	int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
    		int[] attrib_list = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL10.EGL_NONE };
			mEglContext = mEgl.eglCreateContext(mSharedEglDisplay, mSharedEglConfig, mSharedEglContext, attrib_list);
        
			if (mEglContext == null || mEglContext == EGL10.EGL_NO_CONTEXT) {
				mEglContext = null;
				Log.w("MainActivity","eglCreateContext failed");
				return;
			}
			
			int[] surfaceAttribList = {
        			EGL10.EGL_WIDTH, 64,
        			EGL10.EGL_HEIGHT, 64,
        			EGL10.EGL_NONE
			};

			// 资源线程不进行实际绘制，因此创建PbufferSurface而非WindowSurface
			EGLSurface mEglSurface = mEgl.eglCreatePbufferSurface(mSharedEglDisplay, mSharedEglConfig, surfaceAttribList);
			if (mEglSurface == EGL10.EGL_NO_SURFACE) {
				Log.w("MainActivity","eglCreatePbufferSurface failed");
				return;
			}
		
			if (!mEgl.eglMakeCurrent(mSharedEglDisplay, mEglSurface, mEglSurface, mEglContext))
			{
				Log.w("MainActivity","eglMakeCurrent failed");
				return;
			}
		}
		else
		{
			 mEgl.eglMakeCurrent(mSharedEglDisplay, EGL10.EGL_NO_SURFACE,
                     EGL10.EGL_NO_SURFACE,
                     EGL10.EGL_NO_CONTEXT);
		}
	}	
}

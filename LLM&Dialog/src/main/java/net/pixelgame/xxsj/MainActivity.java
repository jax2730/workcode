package net.pixelgame.xxsj;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

import com.google.androidgamesdk.GameActivity;

import java.io.File;
import java.util.HashMap;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;

public class MainActivity extends GameActivity {

    static {
        // Load the native library.
        System.loadLibrary("fmod");
        System.loadLibrary("fmodstudio");
        System.loadLibrary("PadBO2");
    }
    int screenWidth=0;
    static String assetPath;
    static String dataFilePath;
    ImageView joystick_mov,joystick_dir;
    Button button1,button2;
    ImageButton button3, button4, button5, button6, button7, button8, button9, button10, button11, button12;
    static ImageButton buttonDialog;

    // NPC对话框控件
    RelativeLayout dialogContainer;
    LinearLayout dialogMessagesContainer;
    ScrollView dialogScrollView;
    EditText dialogInputText;
    Button dialogSendBtn;
    ImageButton dialogEmojiBtn, dialogCloseBtn;
    TextView dialogNpcName;
    boolean isDialogVisible = false;
    DialogManager dialogManager;

    // 新的三级选择UI控件
    LinearLayout countryPanel, statePanel, countyPanel;
    LinearLayout selectionContainer;
    HorizontalScrollView selectionHScroll;
    ScrollView countryScroll, stateScroll, countyScroll;
    Button transmitButton;
    TextView selectedPathText;
    PopupWindow locationPopup;
    PopupWindow countyPopup;
    Button lastCountySelectedButton;
    View locationRoot;

    // 数据存储
    Map<String, Map<String, String[]>> locationData;
    Map<String, String> coordinatesData;
    String selectedCountry = null;
    String selectedState = null;
    String selectedCounty = null;

    // 保持向后兼容的变量
    int selectionLevel = 0;
    boolean suppressSpinnerCallback = false;

    Spinner spinner;
    Spinner spinner2;//洞天
    Spinner spinner3;//福地
    int m_moveEventID=-1,m_dirEventID=-1;
    float m_moveCenterX=0,m_moveCenterY=0, m_dirCenterX=0, m_dirCenterY=0;
    double m_viewRad=150,m_buttonRad=48;//控件可能未被绘制，直接取宽高有可能是0，直接赋值
    int m_btnLeft=0,m_btnTop=0;
    static TextView Fpslabel;
    static TextView DrawCallLabel;
    static TextView TriangleLabel;
    static TextView ResMemUsageLabel;
    static TextView CamPoslabel;

    static int nFps = 0, nDrawCall=0, nTriangleCnt=0;
    static String memUsage, camPos, gameData;
    static String command;
    static int button_mov = 0, button_dir = 0;
    static float mov_x = 0, mov_y = 0, dir_x = 0, dir_y = 0;

    // 当前靠近的NPC信息 (从C++侧解析)
    static String nearbyNpcId = "";
    static String nearbyNpcName = "";

    static boolean jump = false;
    static Handler mTimeHandler = new Handler(Looper.getMainLooper()) {

        @Override
        public void handleMessage(android.os.Message msg) {
            if (msg.what == 0) {
                Fpslabel.setText("fps: " + nFps);
                DrawCallLabel.setText("draw: " + nDrawCall);
                float fValue = nTriangleCnt / 10000.0f;
                TriangleLabel.setText("trian: " + String.format("%.2f w",fValue));
                sendEmptyMessageDelayed(0, 1000);
                CamPoslabel.setText("cam: " + camPos);
                ResMemUsageLabel.setText(memUsage);

                // NPC proximity detection: parse "npc_id|npc_name" from C++ gameData
                if (gameData != null && !gameData.isEmpty() && gameData.contains("|")) {
                    String[] parts = gameData.split("\\|", 2);
                    nearbyNpcId = parts[0];
                    nearbyNpcName = parts.length > 1 ? parts[1] : parts[0];
                    if (buttonDialog != null) buttonDialog.setVisibility(View.VISIBLE);
                } else {
                    nearbyNpcId = "";
                    nearbyNpcName = "";
                    if (buttonDialog != null) buttonDialog.setVisibility(View.GONE);
                }
            }
        }
    };

    @Override
    protected void onCreate(Bundle instance) {
        hideSystemUI();

        {
            File externalCache = this.getExternalCacheDir();
            File dataDir = new File(externalCache.getParent(),"asset");
            if(!dataDir.exists())
                dataDir.mkdirs();
            dataFilePath = dataDir.getPath() + "/";
            assetPath = dataFilePath;
        }

        super.onCreate(instance);

        // 初始化对话管理器
        dialogManager = DialogManager.getInstance();
        dialogManager.setMainActivity(this);

        // 从配置文件加载设置
        dialogManager.setServerUrl(DialogConfig.SERVER_URL);
        dialogManager.setNetworkEnabled(DialogConfig.ENABLE_NETWORK);

        controlViewInit();
        mTimeHandler.sendEmptyMessageDelayed(0, 1000);
        org.fmod.FMOD.init(this);
    }

    @Override protected void onDestroy()
    {
        org.fmod.FMOD.close();
        super.onDestroy();
    }

    private void hideSystemUI() {
        // This will put the game behind any cutouts and waterfalls on devices which have
        // them, so the corresponding insets will be non-zero.
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            getWindow().getAttributes().layoutInDisplayCutoutMode
                    = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
        }
        // From API 30 onwards, this is the recommended way to hide the system UI, rather than
        // using View.setSystemUiVisibility.
        View decorView = getWindow().getDecorView();
        WindowInsetsControllerCompat controller = new WindowInsetsControllerCompat(getWindow(),
                decorView);
        controller.hide(WindowInsetsCompat.Type.systemBars());
        controller.hide(WindowInsetsCompat.Type.displayCutout());
        controller.setSystemBarsBehavior(
                WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
    }

    public String[] getGpuInfo() {
        String[] vara = {"","","","","","","","",""};
        vara[7] = GameUtils.getHardware();				//cpu hardware
        vara[8] = GameUtils.deviceModel();				//device model
        return vara;
    }

    public void controlViewInit() {

        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT);

        Context context = this.getApplicationContext();

        LayoutInflater iInflater = LayoutInflater.from(context);
        View v = iInflater.inflate(R.layout.control_panel, null);

        addContentView(v,params);
        screenWidth = getScreenWidth();

        // 初始化位置数据
        initLocationData();

        joystick_mov = (ImageView)findViewById(R.id.joystick_mov);
        joystick_dir = (ImageView)findViewById(R.id.joystick_dir);
        button1 = (Button)findViewById(R.id.button1);
        button2 = (Button)findViewById(R.id.button2);
        joystick_mov.setVisibility(View.GONE);
        joystick_dir.setVisibility(View.GONE);
        button1.setVisibility(View.GONE);
        button2.setVisibility(View.GONE);

        button3  = (ImageButton)findViewById(R.id.button3);
        button4  = (ImageButton)findViewById(R.id.button4);
        button5 = (ImageButton)findViewById(R.id.button5);
        button6 = (ImageButton)findViewById(R.id.button6);
        button7 = (ImageButton)findViewById(R.id.button7);
        button8 = (ImageButton)findViewById(R.id.button8);
        button9 = (ImageButton)findViewById(R.id.button9);
        button10 = (ImageButton)findViewById(R.id.button10);
        button11 = (ImageButton)findViewById(R.id.button11);
        button12 = (ImageButton)findViewById(R.id.button12);
        buttonDialog = (ImageButton)findViewById(R.id.buttonDialog);

        // 初始化对话框控件
        dialogContainer = (RelativeLayout)findViewById(R.id.dialog_container);
        dialogMessagesContainer = (LinearLayout)findViewById(R.id.dialog_messages_container);
        dialogScrollView = (ScrollView)findViewById(R.id.dialog_scroll_view);
        dialogInputText = (EditText)findViewById(R.id.dialog_input_text);
        dialogSendBtn = (Button)findViewById(R.id.dialog_send_btn);
        dialogEmojiBtn = (ImageButton)findViewById(R.id.dialog_emoji_btn);
        dialogCloseBtn = (ImageButton)findViewById(R.id.dialog_close_btn);
        dialogNpcName = (TextView)findViewById(R.id.dialog_npc_name);

        spinner = (Spinner)findViewById(R.id.spinner);
        spinner2 = (Spinner)findViewById(R.id.spinner2);
        spinner3 = (Spinner)findViewById(R.id.spinner3);

        Fpslabel = (TextView)findViewById(R.id.fps_text_view);
        DrawCallLabel = (TextView)findViewById(R.id.drawcall_text_view);
        TriangleLabel = (TextView)findViewById(R.id.triangle_text_view);
        CamPoslabel = (TextView)findViewById(R.id.camera_position);
        ResMemUsageLabel = (TextView)findViewById(R.id.res_mem_usage);

        // 初始化新的三级选择UI（PopupWindow 形式）
        initLocationSelectionUI();

        // joystick_mov.setOnTouchListener(new ViewOnTouchListener());
        // joystick_dir.setOnTouchListener(new ViewOnTouchListener());

        // 对话按钮 - 切换对话框显示/隐藏
        buttonDialog.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                toggleDialogPanel();
            }
        });

        // 关闭对话框按钮
        dialogCloseBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                hideDialogPanel();
            }
        });

        // 发送按钮
        dialogSendBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                sendDialogMessage();
            }
        });

        // 输入框回车发送
        dialogInputText.setOnEditorActionListener((textView, actionId, event) -> {
            if (actionId == android.view.inputmethod.EditorInfo.IME_ACTION_SEND) {
                sendDialogMessage();
                return true;
            }
            return false;
        });

        // 表情按钮（暂时显示Toast提示）
        dialogEmojiBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Toast.makeText(MainActivity.this, "表情功能开发中...", Toast.LENGTH_SHORT).show();
            }
        });

        button3.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                CallInput("test");
            }
        });
        button4.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                command = "sphcct";
                //GameLib.camModeChange();
            }
        });

        button5.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                command = "csUp";
                //GameLib.camSpeedUp();
            }
        });
        button6.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                command = "csDown";
                //GameLib.camSpeedReduction();;
            }
        });
        button9.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                command = "showmap";
                //GameLib.camSpeedReset();
            }
        });
        button10.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                command = "displayArea";
                //GameLib.camSpeedReset();
            }
        });
        button7.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // 若上一次传送后弹窗已被释放，重新初始化
                if (locationPopup == null) {
                    initLocationSelectionUI();
                }
                if (locationPopup.isShowing()) {
                    locationPopup.dismiss();
                    return;
                }
                // 重置状态并展示弹出框
                selectedCountry = null;
                selectedState = null;
                selectedCounty = null;
                selectedPathText.setVisibility(View.GONE);
                transmitButton.setVisibility(View.GONE);
                populateCountryPanel();
                statePanel.setVisibility(View.GONE);
                countyPanel.setVisibility(View.GONE);
                if (locationPopup != null && button7 != null && button7.getWindowToken() != null) {
                    locationPopup.showAsDropDown(button7, 0, dp2px(8));
                } else if (locationPopup != null) {
                    // 回退到全局定位，确保不空指针
                    locationPopup.showAtLocation(getWindow().getDecorView(), Gravity.TOP|Gravity.START, dp2px(12), dp2px(64));
                }
            }
        });
        button8.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                jump = true;
            }
        });
        // 添加TouchListener确保跳跃按钮事件被正确消费，不传播到其他控件
        button8.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if(event.getAction() == MotionEvent.ACTION_DOWN) {
                    jump = true;
                    v.performClick(); // 触发onClick以保持一致性
                    return true;  // 消费事件，阻止传播
                }
                return false;
            }
        });
        spinner.setSelection(0, false);
        // 简化的Spinner事件处理器（保留用于向后兼容性，推荐使用新的按钮UI）
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener(){
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                // 使用新的坐标数据系统
                String cityName = parent.getItemAtPosition(position).toString();
                String cityPosition = coordinatesData.get(cityName);
                if (cityPosition != null) {
                    command = "transmit " + cityPosition;
                    spinner.setVisibility(View.GONE);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                // 处理未选择情况
            }
        });

        button11.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                int visible = spinner2.getVisibility();
                if(visible == View.VISIBLE){
                    spinner2.setVisibility(View.GONE);
                    button12.setVisibility(View.VISIBLE);
                }
                else if(visible == View.GONE){
                    spinner2.setVisibility(View.VISIBLE);
                    button12.setVisibility(View.GONE);
                    spinner3.setVisibility(View.GONE);
                }

            }
        });
        spinner2.setSelection(0, false);
        spinner2.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener(){
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {}

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                // 处理未选择情况
            }
        });

        button12.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                int visible = spinner3.getVisibility();
                if(visible == View.VISIBLE){
                    spinner3.setVisibility(View.GONE);
                }
                else if(visible == View.GONE){
                    spinner3.setVisibility(View.VISIBLE);
                }

            }
        });
        spinner3.setSelection(0, false);
        spinner3.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener(){
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
//                String cityName = parent.getItemAtPosition(position).toString();
                String cityPosition = "";
                if(position >= 1) {
                    final  String[] positions3 = {};
                    cityPosition = positions3[position];
                }
                String goCommand = "transmit ";
                command = goCommand + cityPosition;
                spinner3.setVisibility(View.GONE);
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                // 处理未选择情况
            }
        });
    }

    private void setSpinnerItems(int arrayResId) {
        ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(this, arrayResId, android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        suppressSpinnerCallback = true;
        spinner.setAdapter(adapter);
        spinner.setSelection(0, false);
        suppressSpinnerCallback = false;
    }


    public int getScreenWidth(){
        Resources resources = this.getResources();
        DisplayMetrics dm = resources.getDisplayMetrics();
        return dm.widthPixels;
    }

    public boolean isJump(float x, float y){
        // 使用屏幕绝对坐标进行判断
        int[] location = new int[2];
        button8.getLocationOnScreen(location);
        float jumpLeft = location[0];
        float jumpTop = location[1];
        float jumpRight = jumpLeft + button8.getWidth();
        float jumpBottom = jumpTop + button8.getHeight();

        // onTouchEvent中的x,y是相对于DecorView的，需要转换
        // 备用检测
        if(x >= jumpLeft && x <= jumpRight && y >= jumpTop && y <= jumpBottom){
            return true;
        }
        return false;
    }
    public void setMoveCenterX(float centerX, float centerY){
        m_moveCenterX = centerX;m_moveCenterY = centerY;
    }
    public void setDirCenterX(float centerX, float centerY){
        m_dirCenterX = centerX;m_dirCenterY = centerY;
    }
    public void setMoveJoystickViewPos(){
        RelativeLayout.LayoutParams layoutViewParams = (RelativeLayout.LayoutParams) joystick_mov.getLayoutParams();
        layoutViewParams.leftMargin = (int)(m_moveCenterX - m_viewRad);
        layoutViewParams.topMargin = (int)(m_moveCenterY - m_viewRad);
        layoutViewParams.rightMargin = 0;
        layoutViewParams.bottomMargin = 0;
        joystick_mov.setLayoutParams(layoutViewParams);
        //joystick_mov.setVisibility(View.VISIBLE);
    }
    public void setDirJoystickViewPos(){

        RelativeLayout.LayoutParams layoutViewParams = (RelativeLayout.LayoutParams) joystick_dir.getLayoutParams();
        layoutViewParams.rightMargin = (int)(screenWidth - m_dirCenterX - m_viewRad);
        layoutViewParams.topMargin = (int)(m_dirCenterY - m_viewRad);
        layoutViewParams.leftMargin = 0;
        layoutViewParams.bottomMargin = 0;
        joystick_dir.setLayoutParams(layoutViewParams);
        //joystick_dir.setVisibility(View.VISIBLE);
    }

    public void setJoystickBtnPos(Button btn, float offset_x, float offset_y){
        RelativeLayout.LayoutParams layoutButtonParams = (RelativeLayout.LayoutParams) btn.getLayoutParams();
        layoutButtonParams.leftMargin = m_btnLeft + (int) offset_x;
        layoutButtonParams.topMargin = m_btnTop + (int) offset_y;
        layoutButtonParams.rightMargin = 0;
        layoutButtonParams.bottomMargin = 0;
        btn.setLayoutParams(layoutButtonParams);
    }

    public void setOffset(double offsetX, double offsetY, Button btn, boolean isMove){
        final double distance = Math.pow(Math.pow(offsetX, 2) + Math.pow(offsetY, 2), 0.5);
        final double r = m_viewRad - m_buttonRad;
        if (distance > r) {
            offsetX = offsetX * (r / distance);
            offsetY = offsetY * (r / distance);
        }

        setJoystickBtnPos(btn, (float)offsetX, (float)offsetY);

        final float xx = (float) (offsetX / m_viewRad);
        final float yy = (float) (-offsetY / m_viewRad);

        if(isMove){
            mov_x = xx;
            mov_y = yy;
        }else{
            dir_x = xx;
            dir_y = yy;
        }
    }

    @Override public boolean onTouchEvent(MotionEvent event) {
        super.onTouchEvent(event);
        if(event.getSource() == InputDevice.SOURCE_TOUCHSCREEN)
        {
            final int action = event.getActionMasked();
            final int actIdx = event.getActionIndex();
            final int pointCount = event.getPointerCount();

            if(action == MotionEvent.ACTION_DOWN){
                final float x = event.getX(actIdx);
                final float y = event.getY(actIdx);
                // 使用getRawX/getRawY获取屏幕绝对坐标进行跳跃检测
                if(isJump(event.getRawX(), event.getRawY())){
                    jump = true;
                    return true;
                }
                final int pointId = (x < screenWidth/2) ? 100 : 200;
                Button tempBtn = (pointId == 100)? button1 : button2;
                RelativeLayout.LayoutParams layoutButtonParams = (RelativeLayout.LayoutParams) tempBtn.getLayoutParams();
                m_btnLeft = layoutButtonParams.leftMargin;
                m_btnTop  = layoutButtonParams.topMargin;
                if(pointId == 100){
                    button_mov = 1;
                    m_moveEventID = event.getActionIndex();
                    setMoveCenterX(x, y);
                    setMoveJoystickViewPos();
                }else if(pointId == 200){
                    button_dir = 1;
                    m_dirEventID = event.getActionIndex();
                    setDirCenterX(x, y);
                    setDirJoystickViewPos();
                }
                //tempBtn.setVisibility(View.VISIBLE);
            }
            if(action == MotionEvent.ACTION_POINTER_DOWN){
                final float x = event.getX(actIdx);
                final float y = event.getY(actIdx);
                // 使用getRawX/getRawY获取屏幕绝对坐标进行跳跃检测
                if(actIdx < event.getPointerCount() && isJump(event.getX(actIdx) + event.getRawX() - event.getX(0), event.getY(actIdx) + event.getRawY() - event.getY(0))){
                    jump = true;
                    return true;  // 消费事件，防止传播到其他控件
                }
                final int pointId = (x < screenWidth/2) ? 100 : 200;
                Button tempBtn = (pointId == 100)? button1 : button2;
                RelativeLayout.LayoutParams layoutButtonParams = (RelativeLayout.LayoutParams) tempBtn.getLayoutParams();
                m_btnLeft = layoutButtonParams.leftMargin;
                m_btnTop  = layoutButtonParams.topMargin;
                if(pointId == 100){
                    button_mov = 1;
                    m_moveEventID = event.getActionIndex();
                    setMoveCenterX(x, y);
                    setMoveJoystickViewPos();
                }else if(pointId == 200){
                    button_dir = 1;
                    m_dirEventID = event.getActionIndex();
                    setDirCenterX(x, y);
                    setDirJoystickViewPos();
                }
                //tempBtn.setVisibility(View.VISIBLE);
            }
            if(action == MotionEvent.ACTION_POINTER_DOWN){
                final float x = event.getX(actIdx);
                final float y = event.getY(actIdx);
                if(isJump(x, y)){
                    jump = true;
                    return true;
                }
                final int pointId = (x < screenWidth/2) ? 100 : 200;
                Button tempBtn = (pointId == 100)? button1 : button2;
                if(pointId == 100 && button_mov == 1){
                    return false;
                }
                if(pointId == 200 && button_dir == 1){
                    return false;
                }
                RelativeLayout.LayoutParams layoutButtonParams = (RelativeLayout.LayoutParams) tempBtn.getLayoutParams();
                m_btnLeft = layoutButtonParams.leftMargin;
                m_btnTop  = layoutButtonParams.topMargin;
                if(pointId == 100){
                    button_mov = 1;
                    m_moveEventID = event.getActionIndex();
                    setMoveCenterX(x, y);
                    setMoveJoystickViewPos();
                }else if(pointId == 200){
                    button_dir = 1;
                    m_dirEventID = event.getActionIndex();
                    m_dirCenterX = x;m_dirCenterY = y;
                    setDirCenterX(x, y);
                    setDirJoystickViewPos();
                }
                //tempBtn.setVisibility(View.VISIBLE);
            }
            if(action == MotionEvent.ACTION_UP){
                setJoystickBtnPos(button1, 0, 0);
                setJoystickBtnPos(button2, 0, 0);
                button_mov = 0;
                button_dir = 0;
                m_moveEventID = -1;
                m_dirEventID = -1;
                mov_x = 0;
                mov_y = 0;
                dir_x = 0;
                dir_y = 0;
                //joystick_dir.setVisibility(View.GONE);
                //button1.setVisibility(View.GONE);
                //joystick_mov.setVisibility(View.GONE);
                //button2.setVisibility(View.GONE);
            }
            if(action == MotionEvent.ACTION_POINTER_UP){
                //Button tempBtn = (m_moveEventID == actIdx)? button1 : button2;
                //ImageView tempView = (m_moveEventID == actIdx)? joystick_mov : joystick_dir;
                if(m_moveEventID == actIdx){
                    button_mov = 0;
                    m_moveEventID = -1;
                    mov_x = 0;
                    mov_y = 0;
                    setJoystickBtnPos(button1, 0, 0);
                }else{
                    button_dir = 0;
                    m_dirEventID = -1;
                    dir_x = 0;
                    dir_y = 0;
                    setJoystickBtnPos(button2, 0, 0);
                }
                //tempView.setVisibility(View.GONE);
                //tempBtn.setVisibility(View.GONE);
            }
            if(action == MotionEvent.ACTION_MOVE) {
                try {
                    if (pointCount > 1 && button_dir == 1 && button_mov == 1) {
                        //move event
                        {
                            final float x = event.getX(m_moveEventID);
                            final float y = event.getY(m_moveEventID);
                            double offset_x = x - m_moveCenterX;
                            double offset_y = y - m_moveCenterY;
                            setOffset(offset_x, offset_y, button1, true);
                        }
                        //dir event
                        {
                            final float x = event.getX(m_dirEventID);
                            final float y = event.getY(m_dirEventID);
                            double offset_x = x - m_dirCenterX;
                            double offset_y = y - m_dirCenterY;
                            setOffset(offset_x, offset_y, button2, false);
                        }
                    } else {
                        if (button_mov != 0 && button_dir != 0) {
                            return false;
                        }
                        float centerX = 0, centerY = 0;
                        Button tempBtn;
                        final float x = event.getX(actIdx);
                        final float y = event.getY(actIdx);
                        boolean isMove;
                        if (button_mov == 1) {
                            centerX = m_moveCenterX;
                            centerY = m_moveCenterY;
                            tempBtn = button1;
                            isMove = true;
                        } else {
                            centerX = m_dirCenterX;
                            centerY = m_dirCenterY;
                            tempBtn = button2;
                            isMove = false;
                        }
                        double offset_x = x - centerX;
                        double offset_y = y - centerY;
                        setOffset(offset_x, offset_y, tempBtn, isMove);

                    }
                }catch (IllegalArgumentException ex){
                    Log.e("control", "onTouchEvent: ACTION_MOVE causes pointerIndex out of range");
                }
            }
        }
        return false;
    }

    static String game_string;
    EditText _text_input;
    Button _button_input;
    static PopupWindow _popupWindow;

    public void CallInput(String txt) {


        game_string = txt;
        runOnUiThread(() -> {
            if (_popupWindow != null) {
                _popupWindow.dismiss();
                _popupWindow = null;
            }
            LayoutInflater layoutInflater = (LayoutInflater)getBaseContext()
                    .getSystemService(Activity.LAYOUT_INFLATER_SERVICE);
            View popupView = layoutInflater.inflate(R.layout.input_panel, null);
            _popupWindow = new PopupWindow(popupView,
                    WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.WRAP_CONTENT);

            // Show our UI over NativeActivity window

            _popupWindow.showAtLocation(getWindow().getDecorView(), Gravity.TOP
                    | Gravity.LEFT, 10, 10);

            _button_input = (Button) popupView
                    .findViewById(R.id.bt_confirm_input);
            _button_input.setOnClickListener(v -> HideKeyboard());

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
            _text_input = popupView
                    .findViewById(R.id.txt_input);
            //_text_input.setText(game_string);

            _text_input.setFocusableInTouchMode(true);
            _text_input.setFocusable(true);
            _text_input.requestFocus();


            TimerTask task = new TimerTask() {
                public void run() {
                    InputMethodManager imm =
                            (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                    while (!imm.showSoftInput(_text_input, 0)) {
                        if (imm.showSoftInput(_text_input, 0)) {
                            break;
                        }
                    }
                }
            };

            Timer timer = new Timer();
            timer.schedule(task, 500);
        });
    }

    public void HideKeyboard() {
        _popupWindow.dismiss();
        final String ctrlText = _text_input.getText().toString();
        command = ctrlText;
        new Handler().postDelayed(() -> {
            int SDK_INT = Build.VERSION.SDK_INT;
            if (SDK_INT >= 16) {
                getWindow().getDecorView().setSystemUiVisibility(
                        View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION // hide nav bar
                                | View.SYSTEM_UI_FLAG_FULLSCREEN // hide status bar
                );
            }
        }, 800);
    }

    // 初始化位置数据
    private void initLocationData() {}

    private void initCoordinatesData() {}

    private void initLocationSelectionUI() {
        // 创建选择容器（作为 PopupWindow 的内容根视图）
        selectionContainer = new LinearLayout(this);
        selectionContainer.setOrientation(LinearLayout.HORIZONTAL);
        selectionContainer.setPadding(16, 16, 16, 16);

        selectionHScroll = new HorizontalScrollView(this);
        selectionHScroll.setHorizontalScrollBarEnabled(true);
        selectionHScroll.addView(selectionContainer);

        locationRoot = new LinearLayout(this);
        ((LinearLayout) locationRoot).setOrientation(LinearLayout.VERTICAL);
        ((LinearLayout) locationRoot).addView(selectionHScroll);

        // 创建三个面板
        countryPanel = createSelectionPanel();
        statePanel = createSelectionPanel();
        countyPanel = createSelectionPanel();

        countryScroll = new ScrollView(this);
        stateScroll = new ScrollView(this);
        countyScroll = new ScrollView(this);
        countryScroll.addView(countryPanel);
        stateScroll.addView(statePanel);
        countyScroll.addView(countyPanel);
        selectionContainer.addView(countryScroll);
        selectionContainer.addView(stateScroll);
        selectionContainer.addView(countyScroll);

        // 创建显示选择路径的文本
        selectedPathText = new TextView(this);
        selectedPathText.setText("未选择");
        selectedPathText.setTextSize(16);
        selectedPathText.setTextColor(0xFFFFFFFF); // 白色文字
        selectedPathText.setPadding(20, 10, 20, 10);
        selectedPathText.setBackgroundColor(0x88000000); // 半透明黑色背景
        selectedPathText.setVisibility(View.GONE);

        // 创建传送按钮
        transmitButton = new Button(this);
        transmitButton.setText("🚀 传送到选择地点");
        transmitButton.setTextSize(16);
        transmitButton.setPadding(30, 20, 30, 20);
        transmitButton.setBackgroundColor(0xFF4CAF50); // 绿色背景
        transmitButton.setTextColor(0xFFFFFFFF); // 白色文字
        transmitButton.setVisibility(View.GONE);
        transmitButton.setOnClickListener(v -> performTransmit());

        ((LinearLayout) locationRoot).addView(selectedPathText);
        ((LinearLayout) locationRoot).addView(transmitButton);

        // 创建 PopupWindow
        locationPopup = new PopupWindow(locationRoot,
                WindowManager.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.WRAP_CONTENT,
                true);
        locationPopup.setOutsideTouchable(true);
        locationPopup.setFocusable(true);

        // 初始化国家选择
        populateCountryPanel();
        statePanel.setVisibility(View.GONE);
        countyPanel.setVisibility(View.GONE);
    }

    private LinearLayout createSelectionPanel() {
        LinearLayout panel = new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setLayoutParams(new LinearLayout.LayoutParams(400, LinearLayout.LayoutParams.WRAP_CONTENT));
        panel.setPadding(15, 15, 15, 15);
        panel.setBackgroundColor(0x88000000); // 半透明黑色背景
        return panel;
    }

    private void populateCountryPanel() {
        countryPanel.removeAllViews();
        TextView title = new TextView(this);
        title.setText("选择国家:");
        title.setTextSize(18);
        title.setTextColor(0xFFFFFFFF); // 白色文字
        title.setPadding(0, 0, 0, 10);
        countryPanel.addView(title);

        for (String country : locationData.keySet()) {
            Button countryBtn = new Button(this);
            countryBtn.setText(country);
            countryBtn.setTextSize(14);
            countryBtn.setPadding(20, 15, 20, 15);
            LinearLayout.LayoutParams btnParams = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT,
                    LinearLayout.LayoutParams.WRAP_CONTENT);
            btnParams.setMargins(0, 5, 0, 5);
            countryBtn.setLayoutParams(btnParams);
            countryBtn.setOnClickListener(v -> selectCountry(country));
            countryPanel.addView(countryBtn);
        }
    }

    private void selectCountry(String country) {
        selectedCountry = country;
        selectedState = null;
        selectedCounty = null;

        populateStatePanel(country);
        statePanel.setVisibility(View.VISIBLE);
        countyPanel.setVisibility(View.GONE);
        // 自动滚动到左侧第一列
        selectionHScroll.post(() -> selectionHScroll.fullScroll(View.FOCUS_LEFT));
        updateSelectedPath();
    }

    private void populateStatePanel(String country) {
        statePanel.removeAllViews();
        TextView title = new TextView(this);
        title.setText("选择州/省:");
        title.setTextSize(18);
        title.setTextColor(0xFFFFFFFF); // 白色文字
        title.setPadding(0, 0, 0, 10);
        statePanel.addView(title);

        Map<String, String[]> states = locationData.get(country);
        if (states != null) {
            for (String state : states.keySet()) {
                Button stateBtn = new Button(this);
                stateBtn.setText(state);
                stateBtn.setTextSize(14);
                stateBtn.setPadding(20, 15, 20, 15);
                LinearLayout.LayoutParams btnParams = new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT,
                        LinearLayout.LayoutParams.WRAP_CONTENT);
                btnParams.setMargins(0, 5, 0, 5);
                stateBtn.setLayoutParams(btnParams);
                stateBtn.setOnClickListener(v -> selectState(state, v));
                statePanel.addView(stateBtn);
            }
        }
    }

    private void selectState(String state) {
        selectedState = state;
        selectedCounty = null;

        String[] counties = locationData.get(selectedCountry).get(state);
        if (counties != null && counties.length > 0) {
            populateCountyPanel(counties);
            countyPanel.setVisibility(View.VISIBLE);
            // 自动滚动到右侧新列
            selectionHScroll.post(() -> selectionHScroll.fullScroll(View.FOCUS_RIGHT));
        } else {
            countyPanel.setVisibility(View.GONE);
            transmitButton.setVisibility(View.VISIBLE);
        }
        // 将滚动定位到当前州在中间列可见区域顶部
        stateScroll.post(() -> stateScroll.fullScroll(View.FOCUS_UP));
        updateSelectedPath();
    }

    // 重载：带锚点视图，县/市弹窗将贴近州按钮展开
    private void selectState(String state, View anchor) {
        selectedState = state;
        selectedCounty = null;
        String[] counties = locationData.get(selectedCountry).get(state);
        if (counties != null && counties.length > 0) {
            showCountyPopup(anchor, counties);
        } else {
            if (countyPopup != null && countyPopup.isShowing()) countyPopup.dismiss();
            transmitButton.setVisibility(View.VISIBLE);
        }
        updateSelectedPath();
    }

    private void showCountyPopup(View anchor, String[] counties) {
        if (countyPopup != null) {
            countyPopup.dismiss();
            countyPopup = null;
        }
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(16, 16, 16, 16);
        TextView title = new TextView(this);
        title.setText("选择县/市:");
        title.setTextColor(0xFFFFFFFF);
        root.addView(title);
        LinearLayout list = new LinearLayout(this);
        list.setOrientation(LinearLayout.VERTICAL);
        lastCountySelectedButton = null;
        for (String c : counties) {
            Button b = new Button(this);
            b.setText(c);
            b.setOnClickListener(v -> selectCountyFromPopup(c, b, list));
            list.addView(b);
        }
        root.addView(list);
        Button go = new Button(this);
        go.setText("🚀 传送");
        go.setOnClickListener(v -> performTransmit());
        go.setVisibility(View.GONE); // 选择后才出现
        root.addView(go);
        countyPopup = new PopupWindow(root,
                WindowManager.LayoutParams.WRAP_CONTENT,
                WindowManager.LayoutParams.WRAP_CONTENT,
                true);
        countyPopup.setOutsideTouchable(true);
        countyPopup.setFocusable(true);
        // 在屏幕Y轴居中，在州按钮右侧显示
        int[] loc = new int[2];
        anchor.getLocationOnScreen(loc);
        int x = loc[0] + anchor.getWidth() + dp2px(8);
        countyPopup.showAtLocation(getWindow().getDecorView(), Gravity.START | Gravity.CENTER_VERTICAL, x, 0);

        // 隐藏主界面上的传送按钮，等待选择后再显示
        transmitButton.setVisibility(View.GONE);
        // 把弹窗内传送按钮的引用存入 tag，供 selectCountyFromPopup 控制
        root.setTag(go);
    }

    private void selectCountyFromPopup(String county) {
        selectedCounty = county;
        updateSelectedPath();
    }

    private void selectCountyFromPopup(String county, Button btn, LinearLayout list) {
        selectedCounty = county;
        // 重置所有按钮状态
        for (int i = 0; i < list.getChildCount(); i++) {
            View child = list.getChildAt(i);
            if (child instanceof Button) {
                ((Button) child).setBackgroundColor(0xFF444444);
                ((Button) child).setTextColor(0xFFFFFFFF);
            }
        }
        // 高亮选中
        btn.setBackgroundColor(0xFF2196F3);
        btn.setTextColor(0xFFFFFFFF);
        lastCountySelectedButton = btn;
        // 显示主界面的传送按钮
        transmitButton.setVisibility(View.VISIBLE);
        // 显示弹窗内部的传送按钮（若存在）
        View parent = (View) list.getParent();
        Object tag = parent.getTag();
        if (tag instanceof View) {
            ((View) tag).setVisibility(View.VISIBLE);
        }
        updateSelectedPath();
    }

    private int dp2px(int dp) {
        float density = getResources().getDisplayMetrics().density;
        return (int) (dp * density + 0.5f);
    }

    private void populateCountyPanel(String[] counties) {
        countyPanel.removeAllViews();
        TextView title = new TextView(this);
        title.setText("选择县/市:");
        title.setTextSize(18);
        title.setTextColor(0xFFFFFFFF); // 白色文字
        title.setPadding(0, 0, 0, 10);
        countyPanel.addView(title);

        for (String county : counties) {
            Button countyBtn = new Button(this);
            countyBtn.setText(county);
            countyBtn.setTextSize(14);
            countyBtn.setPadding(20, 15, 20, 15);
            LinearLayout.LayoutParams btnParams = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT,
                    LinearLayout.LayoutParams.WRAP_CONTENT);
            btnParams.setMargins(0, 5, 0, 5);
            countyBtn.setLayoutParams(btnParams);
            countyBtn.setOnClickListener(v -> selectCounty(county));
            countyPanel.addView(countyBtn);
        }
    }

    private void selectCounty(String county) {
        selectedCounty = county;
        transmitButton.setVisibility(View.VISIBLE);
        if (countyPopup != null && countyPopup.isShowing()) countyPopup.dismiss();
        updateSelectedPath();
    }

    private void updateSelectedPath() {
        StringBuilder path = new StringBuilder();
        if (selectedCountry != null) {
            path.append(selectedCountry);
            if (selectedState != null) {
                path.append(" → ").append(selectedState);
                if (selectedCounty != null) {
                    path.append(" → ").append(selectedCounty);
                }
            }
        }
        selectedPathText.setText(path.toString());
        selectedPathText.setVisibility(View.VISIBLE);
    }


    private void performTransmit() {
        String targetLocation = selectedCounty != null ? selectedCounty : selectedState;
        if (targetLocation != null) {
            String coordinates = coordinatesData.get(targetLocation);
            if (coordinates != null) {
                command = "transmit " + coordinates;
                // 隐藏选择界面
                if (locationPopup != null && locationPopup.isShowing()) locationPopup.dismiss();
                if (countyPopup != null && countyPopup.isShowing()) countyPopup.dismiss();
                selectionContainer.setVisibility(View.GONE);
                selectedPathText.setVisibility(View.GONE);
                transmitButton.setVisibility(View.GONE);
                // 重置选择
                selectedCountry = null;
                selectedState = null;
                selectedCounty = null;

                locationPopup = null;
                countyPopup = null;
            }
        }
    }

    public static AssetManager GetAssetManager()
    {
        return GameApplication._assetinstance;
    }

    // ==================== NPC对话框方法 ====================

    /**
     * 切换对话框显示/隐藏
     */
    private void toggleDialogPanel() {
        if (isDialogVisible) {
            hideDialogPanel();
        } else {
            showDialogPanel();
        }
    }

    /**
     * 显示对话框
     */
    private void showDialogPanel() {
        dialogContainer.setVisibility(View.VISIBLE);
        isDialogVisible = true;

        // 根据当前靠近的NPC启动对话
        String npcId = nearbyNpcId.isEmpty() ? DialogConfig.DEFAULT_NPC_ID : nearbyNpcId;
        String npcName = nearbyNpcName.isEmpty() ? DialogConfig.DEFAULT_NPC_NAME : nearbyNpcName;

        // 如果NPC变了(或首次对话)，重新启动对话
        if (!dialogManager.getCurrentNpcId().equals(npcId)
                || dialogManager.getCurrentNpcName().equals("未知NPC")) {
            dialogManager.startDialog(npcName, npcId);
        }

        // 自动聚焦输入框
        dialogInputText.requestFocus();

        // 显示软键盘
        InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.showSoftInput(dialogInputText, InputMethodManager.SHOW_IMPLICIT);
    }

    /**
     * 隐藏对话框
     */
    private void hideDialogPanel() {
        dialogContainer.setVisibility(View.GONE);
        isDialogVisible = false;

        // 隐藏软键盘
        InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.hideSoftInputFromWindow(dialogInputText.getWindowToken(), 0);
    }

    /**
     * 发送消息
     */
    private void sendDialogMessage() {
        String message = dialogInputText.getText().toString().trim();

        if (message.isEmpty()) {
            Toast.makeText(this, "请输入消息", Toast.LENGTH_SHORT).show();
            return;
        }

        // 添加玩家消息到界面
        addPlayerMessage(message);

        // 清空输入框
        dialogInputText.setText("");

        // 隐藏软键盘
        InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
        if (imm != null) {
            imm.hideSoftInputFromWindow(dialogInputText.getWindowToken(), 0);
        }

        // 显示加载动画
        showLoadingMessage();

        // 通过DialogManager处理消息
        dialogManager.handlePlayerMessage(message);

        // 同时发送到C++引擎（如果需要）
        command = "npc_chat " + message;
    }

    /**
     * 添加玩家消息到对话框
     */
    private void addPlayerMessage(String message) {
        runOnUiThread(() -> {
            LayoutInflater inflater = LayoutInflater.from(this);
            View messageView = inflater.inflate(R.layout.dialog_message_player, null);

            TextView messageText = messageView.findViewById(R.id.player_message);
            messageText.setText(message);

            dialogMessagesContainer.addView(messageView);

            // 滚动到底部
            dialogScrollView.post(() -> dialogScrollView.fullScroll(View.FOCUS_DOWN));
        });
    }

    /**
     * 添加NPC消息到对话框（公开方法，供DialogManager调用）
     */
    public void addNpcMessage(String message) {
        runOnUiThread(() -> {
            // 隐藏加载动画
            hideLoadingMessage();

            LayoutInflater inflater = LayoutInflater.from(this);
            View messageView = inflater.inflate(R.layout.dialog_message_npc, null);

            TextView npcNameText = messageView.findViewById(R.id.npc_name);
            TextView messageText = messageView.findViewById(R.id.npc_message);

            npcNameText.setText(dialogManager.getCurrentNpcName());
            messageText.setText(message);

            dialogMessagesContainer.addView(messageView);

            // 滚动到底部
            dialogScrollView.post(() -> dialogScrollView.fullScroll(View.FOCUS_DOWN));
        });
    }

    // 加载动画相关
    private View loadingMessageView = null;
    private Handler loadingHandler = new Handler(Looper.getMainLooper());
    private Runnable loadingRunnable;
    private int loadingStep = 0;

    /**
     * 显示加载动画
     */
    public void showLoadingMessage() {
        runOnUiThread(() -> {
            // 如果已经有加载动画，先移除
            hideLoadingMessage();

            LayoutInflater inflater = LayoutInflater.from(this);
            loadingMessageView = inflater.inflate(R.layout.dialog_message_loading, null);

            TextView dot1 = loadingMessageView.findViewById(R.id.loading_dot1);
            TextView dot2 = loadingMessageView.findViewById(R.id.loading_dot2);
            TextView dot3 = loadingMessageView.findViewById(R.id.loading_dot3);

            dialogMessagesContainer.addView(loadingMessageView);

            // 启动动画
            loadingStep = 0;
            loadingRunnable = new Runnable() {
                @Override
                public void run() {
                    if (loadingMessageView != null && loadingMessageView.getParent() != null) {
                        loadingStep = (loadingStep + 1) % 4;

                        // 根据步骤设置透明度
                        dot1.setAlpha(loadingStep >= 1 ? 1.0f : 0.3f);
                        dot2.setAlpha(loadingStep >= 2 ? 1.0f : 0.3f);
                        dot3.setAlpha(loadingStep >= 3 ? 1.0f : 0.3f);

                        loadingHandler.postDelayed(this, 400);
                    }
                }
            };
            loadingHandler.post(loadingRunnable);

            // 滚动到底部
            dialogScrollView.post(() -> dialogScrollView.fullScroll(View.FOCUS_DOWN));
        });
    }

    /**
     * 隐藏加载动画
     */
    public void hideLoadingMessage() {
        runOnUiThread(() -> {
            if (loadingRunnable != null) {
                loadingHandler.removeCallbacks(loadingRunnable);
                loadingRunnable = null;
            }

            if (loadingMessageView != null && loadingMessageView.getParent() != null) {
                dialogMessagesContainer.removeView(loadingMessageView);
                loadingMessageView = null;
            }
        });
    }

    /**
     * 设置当前对话的NPC名称
     */
    public void setDialogNpcName(String npcName) {
        runOnUiThread(() -> {
            dialogNpcName.setText(npcName);
        });
    }

    /**
     * 清空对话历史
     */
    public void clearDialogHistory() {
        runOnUiThread(() -> {
            dialogMessagesContainer.removeAllViews();
        });
    }

    /**
     * 从C++调用：开始与NPC对话
     * @param npcName NPC名称
     * @param npcId NPC ID
     */
    /**
     public void startNpcDialog(String npcName, String npcId) {
        runOnUiThread(() -> {
            dialogManager.startDialog(npcName, npcId);
            showDialogPanel();
        });
    }*/

    /**
     * 从C++调用：添加NPC消息（JNI回调）
     */

    /**public static void onNpcMessage(String npcName, String message) {
        // 这个方法会被C++通过JNI调用
        // 需要在GameLib.java中添加对应的native方法
    }*/
}

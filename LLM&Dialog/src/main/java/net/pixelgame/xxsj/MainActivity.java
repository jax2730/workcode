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
    ImageButton button3, button4, button5, button6, button7, button8, button9, button10, button11, button12, buttonDialog;

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
    static String memUsage, camPos;
    static String command;
    static int button_mov = 0, button_dir = 0;
    static float mov_x = 0, mov_y = 0, dir_x = 0, dir_y = 0;

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
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                String cityName = parent.getItemAtPosition(position).toString();
                String cityPosition = "";
                if(cityName.contains("王屋山洞")){
                    cityPosition = "84934.433808,133.489832,31708.514441";
                }
                else if(cityName.contains("委羽山洞") ){
                    cityPosition = "180434.419066,5.178807,101539.28998";
                }
                else if(cityName.contains("西城山洞") ){
                    cityPosition = "84332.243054,222.716298,30042.294547";
                }
                else if(cityName.contains("西玄山洞") ){
                    cityPosition = "61524.612695,179.343576,39238.057694";
                }
                else if(cityName.contains("青城山洞") ){
                    cityPosition = "-7878.368966,139.143365,77247.55672";
                }
                else if(cityName.contains("赤城山洞") ){
                    cityPosition = "177953.410184,15.362337,95764.287276";
                }
                else if(cityName.contains("罗浮山洞") ){
                    cityPosition = "103923.575845,12.405875,158588.2306";
                }
                else if(cityName.contains("句曲山洞") ){
                    cityPosition = "159614.566074,8.248548,68094.677283";
                }
                else if(cityName.contains("林屋山洞") ){
                    cityPosition = "170290.986314,1.517392,75119.456612";
                }
                else if(cityName.contains("括苍山洞") ){
                    cityPosition = "176884.562821,139.937474,99592.44828";
                }
                else if(cityName.contains("霍桐山洞") ){
                    cityPosition = "162341.089631,5.54349,120800.214073";
                }
                else if(cityName.contains("东岳泰山洞") ){
                    cityPosition = "136281.87389,167.961987,20346.746285";
                }
                else if(cityName.contains("南岳衡山洞") ){
                    cityPosition = "89916.3136,13.550493,115912.045636";
                }
                else if(cityName.contains( "西岳华山洞")){
                    cityPosition = "61524.612695,179.343576,39238.057694";
                }
                else if(cityName.contains("北岳常山洞") ){
                    cityPosition = "100180.546211,185.142415,-15881.956426";
                }
                else if(cityName.contains("中岳嵩山洞") ){
                    cityPosition = "93630.417226,67.872145,37031.371139";
                }
                else if(cityName.contains("峨眉山洞") ){
                    cityPosition = "-10211.630737,298.879715,91714.973564";
                }
                else if(cityName.contains("庐山洞") ){
                    cityPosition = "124194.941094,72.342834,91982.744087";
                }
                else if(cityName.contains("四明山洞")){
                    cityPosition = "178691.677643,96.27263,89570.862545";
                }
                else if(cityName.contains("会稽山洞")){
                    cityPosition = "173479.43236,1.328003,86132.899681";
                }
                else if(cityName.contains("太白山洞")){
                    cityPosition = "36906.144049,450.731517,44836.982928";
                }
                else if(cityName.contains("西山洞") ){
                    cityPosition = "121825.923416,21.24053,99770.696";
                }
                else if(cityName.contains("小沩山洞")){
                    cityPosition = "97928.168748,22.463404,121179.507773";
                }
                else if(cityName.contains("灊山洞")){
                    cityPosition = "129369.40916,76.727136,79224.4734";
                }
                else if(cityName.contains("鬼谷山洞") ){
                    cityPosition = "134989.842006,13.240827,107359.059404";
                }
                else if(cityName .contains("武夷山洞") ){
                    cityPosition = "146491.56305,151.271133,108411.949694";
                }
                else if(cityName.contains( "玉笥山洞")){
                    cityPosition = "116961.226401,12.711157,112885.419572";
                }
                else if(cityName.contains("华盖山洞")){
                    cityPosition = "174089.772184,2.099845,107989.612707";
                }
                else if(cityName.contains("盖竹山洞")){
                    cityPosition = "179374.954836,33.316845,100226.108019";
                }
                else if(cityName.contains("都峤山洞") ){
                    cityPosition = "66982.191799,32.361015,163480.037396";
                }
                else if(cityName.contains("白石山洞") ){
                    cityPosition = "62895.677794,43.906427,159294.987329";
                }
                else if(cityName.contains("岣漏山洞") ){
                    cityPosition = "64831.720268,11.536923,164141.452927";
                }
                else if(cityName.contains("九嶷山洞") ){
                    cityPosition = "81772.020104,57.48015,136359.628256";
                }
                else if(cityName.contains("洞阳山洞") ){
                    cityPosition = "98953.69625,16.85915,106160.996427";
                }
                else if(cityName.contains("幕阜山洞") ){
                    cityPosition = "101429.385895,175.04659,97702.990538";
                }
                else if(cityName.contains("大酉山洞") ){
                    cityPosition = "62254.667253,18.326663,108447.120506";
                }
                else if(cityName.contains("金庭山洞") ){
                    cityPosition = "178387.800306,22.497734,91730.239782";
                }
                else if(cityName.contains("麻姑山洞") ){
                    cityPosition = "130557.641465,49.981621,113127.466251";
                }
                else if(cityName.contains("仙都山洞") ){
                    cityPosition = "168445.307028,28.52193,100883.129858";
                }
                else if(cityName.contains("青田山洞") ){
                    cityPosition = "168322.432667,5.316626,105307.907416";
                }
                else if(cityName.contains("钟山洞") ){
                    cityPosition = "154835.647066,36.561797,64871.75";
                }
                else if(cityName.contains("良常山洞") ){
                    cityPosition = "159614.566074,8.248548,68094.677283";
                }
                else if(cityName.contains("紫盖山洞") ){
                    cityPosition = "83261.693536,14.49078,65599.847519";
                }
                else if(cityName.contains("天目山洞") ){
                    cityPosition = "161155.910568,58.996836,83480.897025";
                }
                else if(cityName.contains("桃源山洞") ){
                    cityPosition = "69545.26586,44.567345,105811.809623";
                }
                else if(cityName.contains("金华山洞") ){
                    cityPosition = "163039.813887,68.493531,95400.398093";
                }

                String goCommand = "transmit ";
                command = goCommand + cityPosition;
                spinner2.setVisibility(View.GONE);
            }

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
                    final  String[] positions3 = {"159752.738559,20.545852,67937.376729",
                            "179374.954836,63.616309,100651.647188",
                            "176166.902084,1.016754,107095.597466",
                            "180479.090013,1.287626,101466.310012",
                            "182161.47938,0.690194,102888.674685",
                            "167165.392364,67.588797,109800.909542",
                            "179830.515518,26.375012,106736.612623",
                            "130341.701744,0.393621,160291.036224",
                            "116885.820906,22.613453,112933.218259",
                            "100359.570431,13.031556,139812.814783",
                            "92641.049894,3.410129,93785.349285",
                            "173808.310109,10.222312,105047.530637",
                            "145614.037982,25.219403,112166.151991",
                            "178340.544209,42.215328,95482.654815",
                            "174953.297021,35.052921,89464.14796",
                            "173727.234307,77.682852,93483.918614",
                            "173267.035251,1.490918,86952.573229",
                            "177962.261393,22.628859,91730.239782",
                            "92522.154322,206.118535,140893.343842",
                            "43578.264401,26.754134,163615.560983",
                            "93133.792039,41.288695,131435.278351",
                            "92101.434638,8.721228,104968.848445",
                            "91862.228577,17.03436,106288.115615",
                            "89317.622904,149.471795,115685.669436",
                            "88456.066184,6.701391,120017.498602",
                            "89599.49988,14.153916,116219.859394",
                            "157869.345887,135.192419,117530.658335",
                            "172331.956045,0.824536,109859.580755",
                            "172437.425864,0.376577,112571.658904",
                            "155556.084444,15.71464,98851.807995",
                            "147013.826957,42.503076,122630.745278",
                            "135019.438237,16.065503,107307.675549",
                            "143968.904547,17.0457,102373.793567",
                            "103923.575845,12.405875,158588.2306",
                            "123770.39156,131.232232,119344.987137",
                            "120081.490475,38.082555,109223.86779",
                            "123752.657226,4.542135,108085.369658",
                            "120537.923513,4.009158,102290.664491",
                            "123356.278367,2.136375,101537.864423",
                            "157003.225262,1.068033,48621.855961",
                            "161166.432018,4.108212,63839.477084",
                            "170256.719793,2.365973,75098.530723",
                            "147952.140259,18.069751,67720.841123",
                            "95904.933351,62.891844,61685.514832",
                            "35571.717851,54.916411,80726.286236",
                            "77622.919963,6.459131,97049.309182",
                            "64827.31594,13.043828,103380.395834",
                            "76816.236485,30.524382,94270.368191",
                            "86121.612904,16.233831,140926.120996",
                            "40332.668479,112.841901,65694.200191",
                            "129026.95672,56.361942,91526.598013",
                            "135346.656384,59.144006,91145.612795",
                            "77622.919963,6.459131,97049.309182",
                            "55217.051529,192.042377,43345.446863",
                            "54381.250552,138.521797,43602.195921",
                            "50113.3825,186.935148,44689.799568",
                            "137773.802678,1.37048,79224.4734",
                            "57693.422037,105.815884,40232.457625",
                            "164734.735374,5.311844,73588.30285",
                            "178197.45675,95.220612,94962.167219",
                            "160226.959123,26.616966,97605.701646",
                            "66102.836914,57.090369,35237.638433",
                            "179385.284793,42.27925,89095.726786",
                            "-953.808293,73.301622,72691.457774",
                            "-482.268624,40.368101,101106.261321",
                            "35548.38768,111.586793,119029.843467",
                            "1826.716153,74.45159,73280.095468",
                            "23110.39592,76.436524,78749.656795",
                            "67390.060208,114.985044,122556.403585",
                            "90124.880859,37.269421,36325.582512",
                            "162199.097935,9.194215,127882.43055",
                            "182012.838638,5.43232,107234.823244"};
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
//    private class ViewOnTouchListener implements View.OnTouchListener
//    {
//        boolean m_bDown = false;
//        double m_r=100,m_btn_r=16;
//        float m_centerX=0,m_centerY=0;
//        int m_btnLeft=0,m_btnTop=0;
//        @Override
//        public boolean onTouch(View view, MotionEvent event) {
//
//            Button tempBtn = (view == joystick_mov) ? button1 : button2;
//            if (event.getSource() == InputDevice.SOURCE_TOUCHSCREEN) {
//                final int action = event.getActionMasked();
//                final int actIdx = event.getActionIndex();
//                final int pointId = (view == joystick_mov) ? 100 : 200;
//                final int pointCount = event.getPointerCount();
//                final float x = event.getX(actIdx);
//                final float y = event.getY(actIdx);
//
//                if (action == MotionEvent.ACTION_DOWN) {
//                    m_bDown = true;
//                    m_r = view.getWidth() / 2.0;
//                    m_centerX = view.getWidth() / 2.0f;// + view.getLeft();
//                    m_centerY = view.getHeight() / 2.0f;// + view.getTop();
//                    m_btn_r = tempBtn.getWidth() / 2.0;
//
//                    RelativeLayout.LayoutParams lParams = (RelativeLayout.LayoutParams) tempBtn.getLayoutParams();
//                    m_btnLeft = lParams.leftMargin;
//                    m_btnTop = lParams.topMargin;
//
//                    if (pointId == 100) {
//                        button_mov = 1;
//                    }
//                    if (pointId == 200) {
//                        button_dir = 1;
//                    }
//                }
//
//                if (action == MotionEvent.ACTION_UP ||
//                        action == MotionEvent.ACTION_OUTSIDE) {
//                    m_bDown = false;
//
//                    RelativeLayout.LayoutParams layoutParams = (RelativeLayout.LayoutParams) tempBtn.getLayoutParams();
//                    layoutParams.leftMargin = m_btnLeft;
//                    layoutParams.topMargin = m_btnTop;
//                    layoutParams.rightMargin = 0;
//                    layoutParams.bottomMargin = 0;
//                    tempBtn.setLayoutParams(layoutParams);
//
//                    if (pointId == 100) {
//                        button_mov = 0;
//                        mov_x = 0;
//                        mov_y = 0;
//                    }
//                    if (pointId == 200) {
//                        button_dir = 0;
//                        dir_x = 0;
//                        dir_y = 0;
//                    }
//                }
//
//                if (action == MotionEvent.ACTION_MOVE && m_bDown) {
//                    double offset_x = x - m_centerX;
//                    double offset_y = y - m_centerY;
//                    final double distance = Math.pow(Math.pow(offset_x, 2) + Math.pow(offset_y, 2), 0.5);
//                    final double r = m_r - m_btn_r;
//
//                    if (distance > r) {
//                        offset_x = offset_x * (r / distance);
//                        offset_y = offset_y * (r / distance);
//                    }
//
//                    RelativeLayout.LayoutParams layoutParams = (RelativeLayout.LayoutParams) tempBtn.getLayoutParams();
//                    layoutParams.leftMargin = m_btnLeft + (int) offset_x;
//                    layoutParams.topMargin = m_btnTop + (int) offset_y;
//                    layoutParams.rightMargin = 0;
//                    layoutParams.bottomMargin = 0;
//                    tempBtn.setLayoutParams(layoutParams);
//
//                    final float xx = (float) (offset_x / m_r);
//                    final float yy = (float) (-offset_y / m_r);
//
//                    if (pointId == 100) {
//                        mov_x = xx;
//                        mov_y = yy;
//                    }
//                    if (pointId == 200) {
//                        dir_x = xx;
//                        dir_y = yy;
//                    }
//                }
//                //如果设置为false
//                return true;
//            }
//            return false;
//        }
//    }

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
    private void initLocationData() {
        locationData = new HashMap<>();
        coordinatesData = new HashMap<>();

        // 中国数据
        Map<String, String[]> chinaStates = new HashMap<>();
        chinaStates.put("浙江省", new String[]{"杭州市"});
        chinaStates.put("重庆市", new String[]{"重庆市"});
        chinaStates.put("天津市", new String[]{"天津市"});
        chinaStates.put("四川省", new String[]{"成都市"});
        chinaStates.put("上海市", new String[]{"上海市"});
        chinaStates.put("山东省", new String[]{"济南市"});
        chinaStates.put("江苏省", new String[]{"南京市"});
        chinaStates.put("江西省", new String[]{"南昌市"});
        chinaStates.put("湖南省", new String[]{"长沙市"});
        chinaStates.put("湖北省", new String[]{"武汉市"});
        chinaStates.put("河南省", new String[]{"郑州市"});
        chinaStates.put("河北省", new String[]{"石家庄市"});
        chinaStates.put("广东省", new String[]{"广州市"});
        chinaStates.put("福建省", new String[]{"福州市"});
        chinaStates.put("北京市", new String[]{"北京市"});
        chinaStates.put("安徽省", new String[]{"合肥市"});
        locationData.put("中国", chinaStates);

        // 美国数据（仅保留 oringin.java 中存在的州键）
        Map<String, String[]> usaStates = new HashMap<>();
        usaStates.put("阿拉巴马州", new String[]{"杰斐逊县", "鲍德温县", "奥陶加县"});
        usaStates.put("阿肯色州", new String[]{"普拉斯基县", "华盛顿县-阿肯色", "本顿县"});
        usaStates.put("加利福尼亚州", new String[]{"洛杉矶县", "圣迭戈县", "橙县"});
        usaStates.put("特拉华州", new String[]{"新城堡县", "肯特县", "萨塞克斯县"});
        usaStates.put("佛罗里达州", new String[]{"迈阿密-戴德县", "布劳沃德县", "棕榈滩县"});
        usaStates.put("佐治亚州", new String[]{"富尔顿县", "迪卡尔布县", "科布县"});
        usaStates.put("肯塔基州", new String[]{"杰斐逊县", "费耶特县", "肯顿县"});
        usaStates.put("路易斯安那州", new String[]{"东巴吞鲁日县", "奥尔良县", "杰斐逊县"});
        usaStates.put("马里兰州", new String[]{"蒙哥马利县", "普林斯乔治县", "巴尔的摩县"});
        usaStates.put("密西西比州", new String[]{"海因兹县", "哈里森县", "麦迪逊县"});
        usaStates.put("新泽西州", new String[]{"伯根县", "米德尔塞克斯县", "埃塞克斯县"});
        usaStates.put("纽约州", new String[]{"纽约县", "金斯县", "皇后县"});
        usaStates.put("北卡罗来纳州", new String[]{"梅克伦堡县", "韦克县", "吉尔福德县"});
        usaStates.put("俄勒冈州", new String[]{"穆尔特诺玛县", "华盛顿县-俄勒冈", "克拉克默斯县"});
        usaStates.put("宾夕法尼亚州", new String[]{"费城县", "阿勒格尼县", "蒙哥马利县"});
        usaStates.put("南卡罗来纳州", new String[]{"查尔斯顿县", "格林维尔县", "里奇兰县"});
        usaStates.put("田纳西州", new String[]{"谢尔比县", "戴维森县", "诺克斯县"});
        usaStates.put("德克萨斯州", new String[]{"哈里斯县", "达拉斯县", "贝克萨尔县"});
        usaStates.put("弗吉尼亚州", new String[]{"费尔法克斯县", "威廉王子县", "弗吉尼亚海滩市"});
        usaStates.put("华盛顿州", new String[]{"金县", "皮尔斯县", "斯诺霍米什县"});
        usaStates.put("西弗吉尼亚州", new String[]{"卡诺瓦县", "莫农加利亚县", "伯克利县"});
        locationData.put("美国", usaStates);

        // 德国数据
        Map<String, String[]> deutStates = new HashMap<>();
        deutStates.put("巴登-符腾堡州", new String[]{"斯图加特行政区", "卡尔斯鲁厄行政区", "弗赖堡行政区"});
        deutStates.put("巴伐利亚州", new String[]{"上巴伐利亚行政区", "中弗兰肯行政区", "施瓦本行政区"});
        deutStates.put("黑森州", new String[]{"达姆施塔特行政区", "吉森行政区", "卡塞尔行政区"});
        deutStates.put("柏林州", new String[]{"米特区", "夏洛滕堡-威尔默斯多夫区", "潘科夫区"});
        locationData.put("德国", deutStates);

        // 日本数据
        Map<String, String[]> japanStates = new HashMap<>();
        japanStates.put("爱知县", new String[]{"名古屋市", "一宫市", "丰田市"});
        japanStates.put("千叶县", new String[]{"千叶市", "船桥市", "松户市"});
        japanStates.put("东京都", new String[]{"港区", "涩谷区", "新宿区"});
        japanStates.put("福井县", new String[]{"福井市", "坂井市", "敦贺市"});
        japanStates.put("福岛县", new String[]{"福岛市", "郡山市", "会津若松市"});
        japanStates.put("岐阜县", new String[]{"岐阜市", "大垣市", "高山市"});
        japanStates.put("群马县", new String[]{"前桥市", "高崎市", "太田市"});
        japanStates.put("兵库县", new String[]{"神户市", "姬路市", "尼崎市"});
        japanStates.put("茨城县", new String[]{"水户市", "筑波市", "日立市"});
        japanStates.put("石川县", new String[]{"金泽市", "小松市", "白山市"});
        japanStates.put("神奈川县", new String[]{"横滨市", "川崎市", "相模原市"});
        japanStates.put("京都府", new String[]{"京都市", "宇治市", "舞鹤市"});
        japanStates.put("三重县", new String[]{"津市", "四日市市", "伊势市"});
        japanStates.put("长野县", new String[]{"长野市", "松本市", "上田市"});
        japanStates.put("新潟县", new String[]{"新潟市", "长冈市", "上越市"});
        locationData.put("日本", japanStates);

        // 韩国数据
        Map<String, String[]> koreaStates = new HashMap<>();
        koreaStates.put("釜山广域市", new String[]{"海云台区", "釜山镇区", "影岛区"});
        koreaStates.put("首尔特别市", new String[]{"江南区", "松坡区", "钟路区"});
        locationData.put("韩国", koreaStates);



        // 丹麦数据
        Map<String, String[]> denmarkStates = new HashMap<>();
        denmarkStates.put("首都大区", new String[]{"哥本哈根"});
        denmarkStates.put("中日德兰大区", new String[]{"奥胡斯"});
        denmarkStates.put("北日德兰大区", new String[]{"奥尔堡"});
        denmarkStates.put("西兰大区", new String[]{"法克瑟"});
        denmarkStates.put("南丹麦大区", new String[]{"欧登塞"});
        locationData.put("丹麦", denmarkStates);

        // 西班牙数据
        Map<String, String[]> spainStates = new HashMap<>();
        spainStates.put("安达卢西亚", new String[]{"塞维利亚"});
        spainStates.put("阿拉贡", new String[]{"萨拉戈萨"});
        spainStates.put("坎塔布里亚", new String[]{"坎塔布里亚"});
        spainStates.put("卡斯蒂利亚-拉曼恰", new String[]{"托莱多"});
        spainStates.put("卡斯蒂利亚-莱昂", new String[]{"巴利亚多利德"});
        spainStates.put("加泰罗尼亚", new String[]{"巴塞罗那"});
        spainStates.put("休达和梅利利亚", new String[]{"休达"});
        spainStates.put("马德里自治区", new String[]{"马德里"});
        spainStates.put("纳瓦拉", new String[]{"纳瓦拉"});
        spainStates.put("瓦伦西亚", new String[]{"瓦伦西亚"});
        spainStates.put("埃斯特雷马杜拉", new String[]{"巴达霍斯"});
        spainStates.put("加利西亚", new String[]{"拉科鲁尼亚"});
        spainStates.put("巴利阿里群岛", new String[]{"巴利阿里"});
        spainStates.put("加那利群岛", new String[]{"拉斯帕尔马斯"});
        spainStates.put("拉里奥哈", new String[]{"拉里奥哈"});
        spainStates.put("巴斯克", new String[]{"比斯开"});
        spainStates.put("阿斯图里亚斯", new String[]{"阿斯图里亚斯"});
        spainStates.put("穆尔西亚", new String[]{"穆尔西亚"});
        locationData.put("西班牙", spainStates);

        // 芬兰数据
        Map<String, String[]> finlandStates = new HashMap<>();
        finlandStates.put("东芬兰区", new String[]{"北卡累利阿"});
        finlandStates.put("拉普兰区", new String[]{"拉普兰"});
        finlandStates.put("奥卢区", new String[]{"北博滕区"});
        finlandStates.put("南芬兰区", new String[]{"新地区"});
        finlandStates.put("西芬兰区", new String[]{"西南芬兰"});
        locationData.put("芬兰", finlandStates);

        // 法国数据
        Map<String, String[]> franceStates = new HashMap<>();
        franceStates.put("阿尔萨斯", new String[]{"下莱茵省"});
        franceStates.put("阿基坦", new String[]{"吉伦特省"});
        franceStates.put("奥弗涅", new String[]{"阿列省"});
        franceStates.put("法兰西岛", new String[]{"巴黎"});
        franceStates.put("下诺曼底", new String[]{"卡尔瓦多斯省"});
        franceStates.put("勃艮第", new String[]{"科多尔省"});
        franceStates.put("布列塔尼", new String[]{"伊勒-维莱讷省"});
        franceStates.put("中央-卢瓦尔河谷", new String[]{"卢瓦雷省"});
        franceStates.put("香槟-阿登", new String[]{"马恩省"});
        franceStates.put("科西嘉", new String[]{"南科西嘉省"});
        franceStates.put("弗朗什-孔泰", new String[]{"杜省"});
        franceStates.put("上诺曼底", new String[]{"滨海塞纳省"});
        franceStates.put("朗格多克-鲁西永", new String[]{"加尔省"});
        franceStates.put("利穆赞", new String[]{"上维埃纳省"});
        franceStates.put("洛林", new String[]{"摩泽尔省"});
        franceStates.put("南部-比利牛斯", new String[]{"上加龙省"});
        franceStates.put("北部-加来海峡", new String[]{"诺尔省"});
        franceStates.put("卢瓦尔河地区", new String[]{"大西洋卢瓦尔省"});
        franceStates.put("皮卡第", new String[]{"索姆省"});
        franceStates.put("普瓦图-夏朗德", new String[]{"维埃纳省"});
        franceStates.put("普罗旺斯-阿尔卑斯-蓝色海岸", new String[]{"罗讷河口省"});
        franceStates.put("罗讷-阿尔卑斯", new String[]{"罗讷省"});
        locationData.put("法国", franceStates);

        // 英国数据
        Map<String, String[]> ukStates = new HashMap<>();
        ukStates.put("英格兰", new String[]{"伦敦"});
        ukStates.put("北爱尔兰", new String[]{"贝尔法斯特"});
        ukStates.put("苏格兰", new String[]{"爱丁堡"});
        ukStates.put("威尔士", new String[]{"加迪夫"});
        locationData.put("英国", ukStates);

        // 希腊数据
        Map<String, String[]> greeceStates = new HashMap<>();
        greeceStates.put("爱琴海", new String[]{"北爱琴海"});
        greeceStates.put("阿索斯", new String[]{"阿索斯"});
        greeceStates.put("阿提卡", new String[]{"阿提卡"});
        greeceStates.put("克里特", new String[]{"克里特"});
        greeceStates.put("伊庇鲁斯和西马其顿", new String[]{"伊庇鲁斯"});
        greeceStates.put("马其顿和色雷斯", new String[]{"中马其顿"});
        greeceStates.put("伯罗奔尼撒、西希腊和爱奥尼亚群岛", new String[]{"伯罗奔尼撒"});
        greeceStates.put("色萨利和中希腊", new String[]{"色萨利"});
        locationData.put("希腊", greeceStates);

        // 意大利数据
        Map<String, String[]> italyStates = new HashMap<>();
        italyStates.put("阿布鲁佐", new String[]{"拉奎拉"});
        italyStates.put("普利亚", new String[]{"巴里"});
        italyStates.put("巴西利卡塔", new String[]{"波坦察"});
        italyStates.put("卡拉布里亚", new String[]{"科森扎"});
        italyStates.put("坎帕尼亚", new String[]{"那不勒斯"});
        italyStates.put("艾米利亚-罗马涅", new String[]{"博洛尼亚"});
        italyStates.put("弗留利-威尼斯朱利亚", new String[]{"乌迪内"});
        italyStates.put("拉齐奥", new String[]{"罗马"});
        italyStates.put("利古里亚", new String[]{"热那亚"});
        italyStates.put("伦巴第", new String[]{"米兰"});
        italyStates.put("马尔凯", new String[]{"安科纳"});
        italyStates.put("莫利塞", new String[]{"坎波巴索"});
        italyStates.put("皮埃蒙特", new String[]{"都灵"});
        italyStates.put("撒丁", new String[]{"卡利亚里"});
        italyStates.put("西西里", new String[]{"巴勒莫"});
        italyStates.put("托斯卡纳", new String[]{"佛罗伦萨"});
        italyStates.put("特伦蒂诺-上阿迪杰", new String[]{"特伦托"});
        italyStates.put("翁布里亚", new String[]{"佩鲁贾"});
        italyStates.put("瓦莱达奥斯塔", new String[]{"奥斯塔"});
        italyStates.put("威尼托", new String[]{"帕多瓦"});
        locationData.put("意大利", italyStates);

        // 挪威数据
        Map<String, String[]> norwayStates = new HashMap<>();
        norwayStates.put("东福尔郡", new String[]{"萨尔普斯堡"});
        norwayStates.put("阿克什胡斯郡", new String[]{"奥斯"});
        norwayStates.put("东阿格德尔郡", new String[]{"阿伦达尔"});
        norwayStates.put("布斯克吕郡", new String[]{"德拉门"});
        norwayStates.put("芬马克郡", new String[]{"阿尔塔"});
        norwayStates.put("海德马克郡", new String[]{"埃尔韦吕姆"});
        norwayStates.put("霍达兰郡", new String[]{"卑尔根"});
        norwayStates.put("默勒-鲁姆斯达尔郡", new String[]{"奥勒松"});
        norwayStates.put("北特伦德拉格郡", new String[]{"斯泰恩谢尔"});
        norwayStates.put("北兰郡", new String[]{"博德"});
        norwayStates.put("奥普兰郡", new String[]{"于韦克"});
        norwayStates.put("奥斯陆郡", new String[]{"奥斯陆"});
        norwayStates.put("罗加兰郡", new String[]{"斯塔万格"});
        norwayStates.put("南特伦德拉格郡", new String[]{"特隆赫姆"});
        norwayStates.put("松恩-菲尤拉讷郡", new String[]{"松达尔"});
        norwayStates.put("泰勒马克郡", new String[]{"希恩"});
        norwayStates.put("特罗姆斯郡", new String[]{"特罗姆瑟"});
        norwayStates.put("西阿格德尔郡", new String[]{"克里斯蒂安桑"});
        norwayStates.put("西福尔郡", new String[]{"滕斯贝格"});
        locationData.put("挪威", norwayStates);

        // 波兰数据
        Map<String, String[]> polandStates = new HashMap<>();
        polandStates.put("罗兹省", new String[]{"罗兹"});
        polandStates.put("圣十字省", new String[]{"凯尔采"});
        polandStates.put("大波兰省", new String[]{"波兹南"});
        polandStates.put("库亚维-滨海省", new String[]{"比得哥什"});
        polandStates.put("小波兰省", new String[]{"克拉科夫"});
        polandStates.put("下西里西亚省", new String[]{"弗罗茨瓦夫"});
        polandStates.put("卢布林省", new String[]{"卢布林"});
        polandStates.put("卢布斯卡省", new String[]{"绿山城"});
        polandStates.put("马佐夫舍省", new String[]{"华沙"});
        polandStates.put("奥波莱省", new String[]{"奥波莱"});
        polandStates.put("波德拉谢省", new String[]{"比亚韦斯托克"});
        polandStates.put("滨海省", new String[]{"格但斯克"});
        polandStates.put("西里西亚省", new String[]{"卡托维兹"});
        polandStates.put("喀尔巴阡山省", new String[]{"热舒夫"});
        polandStates.put("瓦尔米亚-马祖里省", new String[]{"奥尔什丁"});
        polandStates.put("西滨海省", new String[]{"什切青"});
        locationData.put("波兰", polandStates);

        // 瑞典数据
        Map<String, String[]> swedenStates = new HashMap<>();
        swedenStates.put("东约特兰省", new String[]{"林雪平"});
        swedenStates.put("布莱金厄省", new String[]{"卡尔斯克鲁纳"});
        swedenStates.put("达拉纳省", new String[]{"法伦"});
        swedenStates.put("耶夫勒堡省", new String[]{"耶夫勒"});
        swedenStates.put("哥得兰省", new String[]{"哥得兰"});
        swedenStates.put("哈兰省", new String[]{"哈尔姆斯塔德"});
        swedenStates.put("耶姆特兰省", new String[]{"厄斯特松德"});
        swedenStates.put("延雪平省", new String[]{"延雪平"});
        swedenStates.put("卡尔马省", new String[]{"卡尔马"});
        swedenStates.put("克鲁努贝里省", new String[]{"韦克舍"});
        swedenStates.put("北博滕省", new String[]{"吕勒奥"});
        swedenStates.put("厄勒布鲁省", new String[]{"厄勒布鲁"});
        swedenStates.put("南曼兰省", new String[]{"埃斯基尔斯蒂纳"});
        swedenStates.put("斯科讷省", new String[]{"马尔默"});
        swedenStates.put("斯德哥尔摩省", new String[]{"斯德哥尔摩"});
        swedenStates.put("乌普萨拉省", new String[]{"乌普萨拉"});
        swedenStates.put("韦姆兰省", new String[]{"卡尔斯塔德"});
        swedenStates.put("西博滕省", new String[]{"于默奥"});
        swedenStates.put("西诺尔兰省", new String[]{"松兹瓦尔"});
        swedenStates.put("西曼兰省", new String[]{"韦斯特罗斯"});
        swedenStates.put("西约特兰省", new String[]{"哥德堡"});
        locationData.put("瑞典", swedenStates);

        // 初始化坐标数据
        initCoordinatesData();
    }

    private void initCoordinatesData() {
        // 中国省份坐标（严格对齐 oringin.java 列表，移除多余省份）
        coordinatesData.put("浙江省", "-329216,1000,576320");
        coordinatesData.put("重庆市", "-466560,1000,574208");
        coordinatesData.put("天津市", "-354720,1000,441472");
        coordinatesData.put("四川省", "-514240,1000,565952");
        coordinatesData.put("上海市", "-305952,1000,558656");
        coordinatesData.put("山东省", "-357664,1000,480000");
        coordinatesData.put("江苏省", "-336864,1000,548512");
        coordinatesData.put("江西省", "-370176,1000,593056");
        coordinatesData.put("湖南省", "-404160,1000,598880");
        coordinatesData.put("湖北省", "-390112,1000,566464");
        coordinatesData.put("河南省", "-400384,1000,510560");
        coordinatesData.put("河北省", "-388928,1000,459296");
        coordinatesData.put("广东省", "-399616,1000,662592");
        coordinatesData.put("福建省", "-332896,1000,627680");
        coordinatesData.put("北京市", "-365632,1000,428064");
        coordinatesData.put("安徽省", "-355744,1000,547648");


        coordinatesData.put("合肥市", "-355744,1000,547648");

        coordinatesData.put("福州市", "-332896,1000,627680");

        coordinatesData.put("兰州市", "-516736,1000,485504");

        coordinatesData.put("广州市", "-399616,1000,662592");
        coordinatesData.put("南宁市", "-459616,1000,666368");



        coordinatesData.put("贵阳市", "-480384,1000,617248");



        coordinatesData.put("海口市", "-436384,1000,707136");

        coordinatesData.put("石家庄市", "-388928,1000,459296");


        coordinatesData.put("郑州市", "-400384,1000,510560");


        coordinatesData.put("哈尔滨市", "-228992,1000,340224");


        coordinatesData.put("武汉市", "-390112,1000,566464");


        coordinatesData.put("长沙市", "-404160,1000,598880");


        coordinatesData.put("长春市", "-254944,1000,361248");


        coordinatesData.put("南京市", "-336864,1000,548512");


        coordinatesData.put("南昌市", "-370176,1000,593056");


        coordinatesData.put("沈阳市", "-286048,1000,398176");


        coordinatesData.put("呼和浩特市", "-424896,1000,421920");


        coordinatesData.put("银川市", "-484736,1000,457920");



        coordinatesData.put("西宁市", "-542528,1000,478816");


        coordinatesData.put("济南市", "-357664,1000,480000");


        coordinatesData.put("太原市", "-414080,1000,461856");


        coordinatesData.put("西安市", "-455744,1000,517920");


        coordinatesData.put("成都市", "-514240,1000,565952");


        coordinatesData.put("拉萨市", "-664864,1000,574560");


        coordinatesData.put("乌鲁木齐市", "-703840,1000,375104");


        coordinatesData.put("昆明市", "-525696,1000,636320");


        coordinatesData.put("杭州市", "-329216,1000,576320");


        // 美国州坐标
        coordinatesData.put("阿拉巴马州", "1481920,1000,517344");
        coordinatesData.put("阿肯色州", "1434592,1000,515328");
        coordinatesData.put("加利福尼亚州", "1073728,1000,466496");
        coordinatesData.put("特拉华州", "1621536,1000,444896");
        coordinatesData.put("佛罗里达州", "1541216,1000,579424");
        coordinatesData.put("佐治亚州", "1542048,1000,551040");
        coordinatesData.put("肯塔基州", "1506656,1000,474528");
        coordinatesData.put("路易斯安那州", "1422336,1000,571040");
        coordinatesData.put("马里兰州", "1584576,1000,436704");
        coordinatesData.put("密西西比州", "1434816,1000,554720");
        coordinatesData.put("新泽西州", "1632096,1000,438848");
        coordinatesData.put("纽约州", "1640384,1000,390144");
        coordinatesData.put("北卡罗来纳州", "1576224,1000,490080");
        coordinatesData.put("俄勒冈州", "1123552,1000,355744");
        coordinatesData.put("宾夕法尼亚州", "1602016,1000,432896");
        coordinatesData.put("南卡罗来纳州", "1540032,1000,516288");
        coordinatesData.put("田纳西州", "1519456,1000,488992");
        coordinatesData.put("德克萨斯州", "1384000,1000,550144");
        coordinatesData.put("弗吉尼亚州", "1620608,1000,464768");
        coordinatesData.put("华盛顿州", "1113056,1000,317280");
        coordinatesData.put("西弗吉尼亚州", "1569088,1000,444160");

        // 美国县坐标
        coordinatesData.put("杰斐逊县", "1487552,1000,525792");
        coordinatesData.put("鲍德温县", "1477760,1000,564960");
        coordinatesData.put("奥陶加县", "1490560,1000,540096");
        coordinatesData.put("普拉斯基县", "1423520,1000,508480");
        coordinatesData.put("华盛顿县-阿肯色", "1400992,1000,491008");
        coordinatesData.put("华盛顿县-俄勒冈", "1059392,1000,341568");
        coordinatesData.put("本顿县", "1400480,1000,485760");
        coordinatesData.put("洛杉矶县", "1117024,1000,514912");
        coordinatesData.put("圣迭戈县", "1134688,1000,533120");
        coordinatesData.put("橙县", "1122528,1000,523680");
        coordinatesData.put("新城堡县", "1620704,1000,437408");
        coordinatesData.put("肯特县", "1621536,1000,444896");
        coordinatesData.put("萨塞克斯县", "1623552,1000,451296");
        coordinatesData.put("迈阿密-戴德县", "1562432,1000,633376");
        coordinatesData.put("布劳沃德县", "1563360,1000,626336");
        coordinatesData.put("棕榈滩县", "1563584,1000,619840");
        coordinatesData.put("富尔顿县", "1516288,1000,522464");
        coordinatesData.put("迪卡尔布县", "1519136,1000,522720");
        coordinatesData.put("科布县", "1514976,1000,520320");
        coordinatesData.put("费耶特县", "1516384,1000,460608");
        coordinatesData.put("肯顿县", "1515520,1000,447200");
        coordinatesData.put("东巴吞鲁日县", "1437888,1000,567648");
        coordinatesData.put("奥尔良县", "1451648,1000,574080");
        coordinatesData.put("蒙哥马利县", "1602208,1000,444096");
        coordinatesData.put("普林斯乔治县", "1606432,1000,448704");
        coordinatesData.put("巴尔的摩县", "1608864,1000,439392");
        coordinatesData.put("海因兹县", "1445600,1000,543840");
        coordinatesData.put("哈里森县", "1461280,1000,568032");
        coordinatesData.put("麦迪逊县", "1450432,1000,538720");
        coordinatesData.put("伯根县", "1639200,1000,416032");
        coordinatesData.put("米德尔塞克斯县", "1635200,1000,424160");
        coordinatesData.put("埃塞克斯县", "1637120,1000,418720");
        coordinatesData.put("梅克伦堡县", "1559264,1000,501600");
        coordinatesData.put("韦克县", "1585056,1000,493792");
        coordinatesData.put("吉尔福德县", "1571616,1000,489568");
        coordinatesData.put("穆尔特诺玛县", "1066528,1000,340672");
        coordinatesData.put("克拉克默斯县", "1069792,1000,347776");
        coordinatesData.put("费城县", "1626656,1000,430784");
        coordinatesData.put("阿勒格尼县", "1569344,1000,423680");
        coordinatesData.put("查尔斯顿县", "1569632,1000,535872");
        coordinatesData.put("格林维尔县", "1541088,1000,506688");
        coordinatesData.put("里奇兰县", "1558432,1000,519168");
        coordinatesData.put("谢尔比县", "1452064,1000,502528");
        coordinatesData.put("戴维森县", "1488864,1000,488256");
        coordinatesData.put("诺克斯县", "1522560,1000,490816");
        coordinatesData.put("纽约县", "1640448,1000,418816");
        coordinatesData.put("金斯县", "1640672,1000,420960");
        coordinatesData.put("皇后县", "1642208,1000,419968");
        coordinatesData.put("哈里斯县", "1387008,1000,576928");
        coordinatesData.put("达拉斯县", "1370656,1000,536832");
        coordinatesData.put("贝克萨尔县", "1350080,1000,582496");
        coordinatesData.put("费尔法克斯县", "1601280,1000,448608");
        coordinatesData.put("威廉王子县", "1598880,1000,450656");
        coordinatesData.put("弗吉尼亚海滩市", "1615744,1000,479744");
        coordinatesData.put("金县", "1074720,1000,308480");
        coordinatesData.put("皮尔斯县", "1071200,1000,316608");
        coordinatesData.put("斯诺霍米什县", "1076000,1000,298688");
        coordinatesData.put("卡诺瓦县", "1551040,1000,456192");
        coordinatesData.put("莫农加利亚县", "1568576,1000,436576");
        coordinatesData.put("伯克利县", "1592448,1000,439104");

        // 德国州坐标
        coordinatesData.put("巴登-符腾堡州", "-1645184,1000,298976");
        coordinatesData.put("巴伐利亚州", "-1614720,1000,275488");
        coordinatesData.put("柏林州", "-1583808,1000,216480");
        coordinatesData.put("黑森州", "-1638688,1000,262528");

        // 德国行政区坐标
        coordinatesData.put("斯图加特行政区", "-1628512,1000,281472");
        coordinatesData.put("卡尔斯鲁厄行政区", "-1639680,1000,281280");
        coordinatesData.put("弗赖堡行政区", "-1645184,1000,298976");
        coordinatesData.put("上巴伐利亚行政区", "-1603296,1000,297792");
        coordinatesData.put("中弗兰肯行政区", "-1614720,1000,275488");
        coordinatesData.put("施瓦本行政区", "-1617920,1000,296640");
        coordinatesData.put("达姆施塔特行政区", "-1638688,1000,262528");
        coordinatesData.put("吉森行政区", "-1638848,1000,251648");
        coordinatesData.put("卡塞尔行政区", "-1630496,1000,244064");
        coordinatesData.put("米特区", "-1583808,1000,216480");
        coordinatesData.put("夏洛滕堡-威尔默斯多夫区", "-1583808,1000,216480");
        coordinatesData.put("潘科夫区", "-1583808,1000,216480");
        // 日本县坐标
        coordinatesData.put("爱知县", "-123168,1000,506144");
        coordinatesData.put("千叶县", "-85824,1000,492640");
        coordinatesData.put("福井县", "-138272,1000,499360");
        coordinatesData.put("福岛县", "-88896,1000,467712");
        coordinatesData.put("岐阜县", "-126048,1000,500320");
        coordinatesData.put("群马县", "-101024,1000,485760");
        coordinatesData.put("兵库县", "-152160,1000,507552");
        coordinatesData.put("茨城县", "-84032,1000,490624");
        coordinatesData.put("石川县", "-122784,1000,472608");
        coordinatesData.put("神奈川县", "-94880,1000,497632");
        coordinatesData.put("京都府", "-137632,1000,506720");
        coordinatesData.put("三重县", "-130656,1000,514976");
        coordinatesData.put("长野县", "-113984,1000,499072");

        //
        coordinatesData.put("名古屋市", "-122912,1000,503168");
        coordinatesData.put("一宫市", "-124512,1000,500800");
        coordinatesData.put("丰田市", "-118496,1000,503168");
        coordinatesData.put("千叶市", "-84608,1000,496576");
        coordinatesData.put("船桥市", "-86368,1000,494752");
        coordinatesData.put("松户市", "-87392,1000,493792");
        coordinatesData.put("福井市", "-138272,1000,499360");
        coordinatesData.put("坂井市", "-139200,1000,498560");
        coordinatesData.put("敦贺市", "-140864,1000,507200");
        coordinatesData.put("福岛市", "-88896,1000,467712");
        coordinatesData.put("郡山市", "-90800,1000,470880");
        coordinatesData.put("会津若松市", "-96768,1000,474560");
        coordinatesData.put("岐阜市", "-126048,1000,500320");
        coordinatesData.put("大垣市", "-127232,1000,503040");
        coordinatesData.put("高山市", "-132864,1000,492032");
        coordinatesData.put("前桥市", "-101024,1000,485760");
        coordinatesData.put("高崎市", "-100544,1000,487232");
        coordinatesData.put("太田市", "-98240,1000,487936");
        coordinatesData.put("神户市", "-152160,1000,507552");
        coordinatesData.put("姬路市", "-156160,1000,507136");
        coordinatesData.put("尼崎市", "-151168,1000,507360");
        coordinatesData.put("水户市", "-84032,1000,490624");
        coordinatesData.put("筑波市", "-85760,1000,489088");
        coordinatesData.put("日立市", "-79232,1000,486976");
        coordinatesData.put("金泽市", "-122784,1000,472608");
        coordinatesData.put("小松市", "-123616,1000,474752");
        coordinatesData.put("白山市", "-123584,1000,476160");
        coordinatesData.put("横滨市", "-94880,1000,497632");
        coordinatesData.put("川崎市", "-95680,1000,498560");
        coordinatesData.put("相模原市", "-99200,1000,496448");
        coordinatesData.put("京都市", "-137632,1000,506720");
        coordinatesData.put("宇治市", "-138432,1000,508128");
        coordinatesData.put("舞鹤市", "-144768,1000,503936");
        coordinatesData.put("津市", "-130656,1000,514976");
        coordinatesData.put("四日市市", "-129088,1000,512864");
        coordinatesData.put("伊势市", "-128192,1000,518080");
        coordinatesData.put("长野市", "-113984,1000,499072");
        coordinatesData.put("松本市", "-116192,1000,500096");
        coordinatesData.put("上田市", "-112992,1000,498688");
        // 新潟县三市
        coordinatesData.put("新潟市", "-97984,1000,463840");
        coordinatesData.put("长冈市", "-100064,1000,469504");
        coordinatesData.put("上越市", "-106240,1000,474304");
        // 东京都
        coordinatesData.put("港区", "-89664,1000,495776");
        coordinatesData.put("涩谷区", "-90144,1000,495584");
        coordinatesData.put("新宿区", "-89952,1000,495136");
        // 韩国地区坐标
        coordinatesData.put("釜山广域市", "-214784,1000,502368");
        coordinatesData.put("海云台区", "-214784,1000,502368");
        coordinatesData.put("釜山镇区", "-216192,1000,503008");
        coordinatesData.put("影岛区", "-215936,1000,504064");
        // 首尔
        coordinatesData.put("江南区", "-239616,1000,469088");
        coordinatesData.put("松坡区", "-238944,1000,469056");
        coordinatesData.put("钟路区", "-240672,1000,467488");
        //丹麦

        coordinatesData.put("哥本哈根", "-1593952,1000,152544");
        coordinatesData.put("奥胡斯", "-1622272,1000,142272");
        coordinatesData.put("奥尔堡", "-1624544,1000,124032");
        coordinatesData.put("法克瑟", "-1599744,1000,160608");
        coordinatesData.put("欧登塞", "-1619712,1000,158592");

        // 西班牙
        coordinatesData.put("塞维利亚", "-1809600,1000,469632");
        coordinatesData.put("萨拉戈萨", "-1754976,1000,405664");
        coordinatesData.put("坎塔布里亚", "-1790048,1000,380544");
        coordinatesData.put("托莱多", "-1791424,1000,434080");
        coordinatesData.put("巴利亚多利德", "-1799712,1000,405472");
        coordinatesData.put("巴塞罗那", "-1718912,1000,403936");
        coordinatesData.put("休达", "-1805568,1000,492288");
        coordinatesData.put("马德里", "-1786336,1000,423264");
        coordinatesData.put("纳瓦拉", "-1761856,1000,389056");
        coordinatesData.put("瓦伦西亚", "-1751840,1000,440544");
        coordinatesData.put("巴达霍斯", "-1815008,1000,450560");
        coordinatesData.put("拉科鲁尼亚", "-1842496,1000,381696");
        coordinatesData.put("巴利阿里", "-1707968,1000,437472");
        coordinatesData.put("拉斯帕尔马斯", "-1914400,1000,597024");
        coordinatesData.put("拉里奥哈", "-1772160,1000,395328");
        coordinatesData.put("比斯开", "-1776096,1000,379808");
        coordinatesData.put("阿斯图里亚斯", "-1813248,1000,378976");
        coordinatesData.put("穆尔西亚", "-1759936,1000,461216");

        // 芬兰
        coordinatesData.put("北卡累利阿", "-1388256,1000,-15680");
        coordinatesData.put("拉普兰", "-1430976,1000,-152256");
        coordinatesData.put("北博滕区", "-1427424,1000,-70912");
        coordinatesData.put("新地区", "-1454880,1000,49216");
        coordinatesData.put("西南芬兰", "-1475872,1000,43520");

        // 法国
        coordinatesData.put("下莱茵省", "-1653056,1000,287616");
        coordinatesData.put("吉伦特省", "-1749184,1000,353856");
        coordinatesData.put("阿列省", "-1704672,1000,327424");
        coordinatesData.put("巴黎", "-1714688,1000,284320");
        coordinatesData.put("卡尔瓦多斯省", "-1746688,1000,279968");
        coordinatesData.put("科多尔省", "-1685952,1000,309600");
        coordinatesData.put("伊勒-维莱讷省", "-1761760,1000,296768");
        coordinatesData.put("卢瓦雷省", "-1714656,1000,301088");
        coordinatesData.put("马恩省", "-1692256,1000,282656");
        coordinatesData.put("南科西嘉省", "-1636096,1000,401856");
        coordinatesData.put("杜省", "-1667136,1000,314112");
        coordinatesData.put("滨海塞纳省", "-1730240,1000,269888");
        coordinatesData.put("加尔省", "-1692960,1000,367552");
        coordinatesData.put("上维埃纳省", "-1727776,1000,335936");
        coordinatesData.put("摩泽尔省", "-1663584,1000,281056");
        coordinatesData.put("上加龙省", "-1728480,1000,377920");
        coordinatesData.put("诺尔省", "-1704352,1000,255328");
        coordinatesData.put("大西洋卢瓦尔省", "-1762240,1000,310720");
        coordinatesData.put("索姆省", "-1715488,1000,264352");
        coordinatesData.put("维埃纳省", "-1736960,1000,324480");
        coordinatesData.put("罗讷河口省", "-1682240,1000,374912");
        coordinatesData.put("罗讷省", "-1687488,1000,336320");

        // 英国）
        coordinatesData.put("伦敦", "-1743360,1000,235392");
        coordinatesData.put("贝尔法斯特", "-1812800,1000,174528");
        coordinatesData.put("爱丁堡", "-1781216,1000,147392");
        coordinatesData.put("加迪夫", "-1780000,1000,235488");

        // 希腊
        coordinatesData.put("北爱琴海", "-1433600,1000,449344");
        coordinatesData.put("阿索斯", "-1456224,1000,426784");
        coordinatesData.put("阿提卡", "-1463264,1000,463712");
        coordinatesData.put("克里特", "-1448544,1000,501888");
        coordinatesData.put("伊庇鲁斯", "-1496608,1000,437088");
        coordinatesData.put("中马其顿", "-1470912,1000,419456");
        coordinatesData.put("伯罗奔尼撒", "-1476768,1000,471040");
        coordinatesData.put("色萨利", "-1479584,1000,438112");

        // 意大利
        coordinatesData.put("拉奎拉", "-1581504,1000,398016");
        coordinatesData.put("巴里", "-1544256,1000,416480");
        coordinatesData.put("波坦察", "-1554432,1000,422944");
        coordinatesData.put("科森扎", "-1549344,1000,437568");
        coordinatesData.put("那不勒斯", "-1572768,1000,417856");
        coordinatesData.put("博洛尼亚", "-1608160,1000,360352");
        coordinatesData.put("乌迪内", "-1586752,1000,330240");
        coordinatesData.put("罗马", "-1593696,1000,400896");
        coordinatesData.put("热那亚", "-1634496,1000,359776");
        coordinatesData.put("米兰", "-1634496,1000,343136");
        coordinatesData.put("安科纳", "-1586784,1000,375552");
        coordinatesData.put("坎波巴索", "-1567616,1000,404384");
        coordinatesData.put("都灵", "-1654368,1000,348512");
        coordinatesData.put("卡利亚里", "-1634112,1000,441376");
        coordinatesData.put("巴勒莫", "-1581696,1000,463168");
        coordinatesData.put("佛罗伦萨", "-1608832,1000,370432");
        coordinatesData.put("特伦托", "-1610848,1000,331808");
        coordinatesData.put("佩鲁贾", "-1594176,1000,382624");
        coordinatesData.put("奥斯塔", "-1654944,1000,338656");
        coordinatesData.put("帕多瓦", "-1602752,1000,345088");

        // 挪威
        coordinatesData.put("萨尔普斯堡", "-1610464,1000,73216");
        coordinatesData.put("奥斯", "-1614848,1000,64128");
        coordinatesData.put("阿伦达尔", "-1638720,1000,91328");
        coordinatesData.put("德拉门", "-1622208,1000,63232");
        coordinatesData.put("阿尔塔", "-1467648,1000,-226304");
        coordinatesData.put("埃尔韦吕姆", "-1610624,1000,33824");
        coordinatesData.put("卑尔根", "-1678560,1000,47872");
        coordinatesData.put("奥勒松", "-1667040,1000,-4064");
        coordinatesData.put("斯泰恩谢尔", "-1603520,1000,-46176");
        coordinatesData.put("博德", "-1568192,1000,-140096");
        coordinatesData.put("于韦克", "-1620384,1000,18816");
        coordinatesData.put("奥斯陆", "-1615392,1000,56992");
        coordinatesData.put("斯塔万格", "-1674912,1000,80672");
        coordinatesData.put("特隆赫姆", "-1619744,1000,-27840");
        coordinatesData.put("松达尔", "-1658976,1000,26464");
        coordinatesData.put("希恩", "-1635904,1000,69568");
        coordinatesData.put("特罗姆瑟", "-1516576,1000,-214464");
        coordinatesData.put("克里斯蒂安桑", "-1647392,1000,98176");
        coordinatesData.put("滕斯贝格", "-1621536,1000,72864");

        // 波兰
        coordinatesData.put("罗兹", "-1512288,1000,231392");
        coordinatesData.put("凯尔采", "-1497280,1000,248288");
        coordinatesData.put("波兹南", "-1541856,1000,219200");
        coordinatesData.put("比得哥什", "-1528800,1000,204576");
        coordinatesData.put("克拉科夫", "-1505920,1000,263136");
        coordinatesData.put("弗罗茨瓦夫", "-1541920,1000,245184");
        coordinatesData.put("卢布林", "-1475040,1000,240608");
        coordinatesData.put("绿山城", "-1556064,1000,208896");
        coordinatesData.put("华沙", "-1493568,1000,222016");
        coordinatesData.put("奥波莱", "-1530176,1000,251808");
        coordinatesData.put("比亚韦斯托克", "-1468896,1000,204000");
        coordinatesData.put("格但斯克", "-1521888,1000,179680");
        coordinatesData.put("卡托维兹", "-1517728,1000,260160");
        coordinatesData.put("热舒夫", "-1482144,1000,262528");
        coordinatesData.put("奥尔什丁", "-1500448,1000,192192");
        coordinatesData.put("什切青", "-1569568,1000,198272");

        // 瑞典
        coordinatesData.put("林雪平", "-1548096,1000,72608");
        coordinatesData.put("卡尔斯克鲁纳", "-1557024,1000,139808");
        coordinatesData.put("法伦", "-1554976,1000,38752");
        coordinatesData.put("耶夫勒", "-1540416,1000,41056");
        coordinatesData.put("哥得兰", "-1523072,1000,113088");
        coordinatesData.put("哈尔姆斯塔德", "-1588800,1000,129888");
        coordinatesData.put("厄斯特松德", "-1566496,1000,-23840");
        coordinatesData.put("延雪平", "-1574208,1000,106528");
        coordinatesData.put("卡尔马", "-1551392,1000,131264");
        coordinatesData.put("韦克舍", "-1565248,1000,134496");
        coordinatesData.put("吕勒奥", "-1481056,1000,-93952");
        coordinatesData.put("厄勒布鲁", "-1562496,1000,74304");
        coordinatesData.put("埃斯基尔斯蒂纳", "-1548096,1000,72608");
        coordinatesData.put("马尔默", "-1588384,1000,154944");
        coordinatesData.put("斯德哥尔摩", "-1529504,1000,72512");
        coordinatesData.put("乌普萨拉", "-1532608,1000,58496");
        coordinatesData.put("卡尔斯塔德", "-1581344,1000,69120");
        coordinatesData.put("于默奥", "-1503328,1000,-42432");
        coordinatesData.put("松兹瓦尔", "-1542464,1000,-4512");
        coordinatesData.put("韦斯特罗斯", "-1546432,1000,65408");
        coordinatesData.put("哥德堡", "-1600992,1000,108384");


    }

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

        // 如果还没有开始对话，启动默认NPC对话
        if (dialogManager.getCurrentNpcName().equals("未知NPC")) {
            dialogManager.startDialog(DialogConfig.DEFAULT_NPC_NAME, DialogConfig.DEFAULT_NPC_ID);
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
    public void startNpcDialog(String npcName, String npcId) {
        runOnUiThread(() -> {
            dialogManager.startDialog(npcName, npcId);
            showDialogPanel();
        });
    }

    /**
     * 从C++调用：添加NPC消息（JNI回调）
     */
    public static void onNpcMessage(String npcName, String message) {
        // 这个方法会被C++通过JNI调用
        // 需要在GameLib.java中添加对应的native方法
    }
}

package com.example.ai_homehub;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.transition.TransitionManager;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.cardview.widget.CardView;
import androidx.core.app.ActivityCompat;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import org.json.JSONException;
import org.json.JSONObject;

public class MainActivity extends AppCompatActivity {
    // 服务器配置常量
    private static final String SERVER_IP = "1.95.193.57";  // 服务器IP地址（需要替换为实际地址）
    private static final int SERVER_PORT = 8086;              // 服务器端口号（与服务器保持一致）
    private static final int PERMISSION_REQUEST_CODE = 100;   // 权限请求的识别码

    // UI组件声明
    private ImageButton btnConnect, btnDisconnect, btnToggleSensors; // 连接/断开/折叠按钮
    private TextView tvMessages, tvTemperature, tvHumidity, tvSmoke, tvCO, tvLight; // 消息和传感器显示区域
    private EditText etInput;         // 用户输入框
    private CardView cardSensors;     // 传感器卡片容器（用于折叠动画）

    // 状态控制变量
    private boolean isSensorsExpanded = false;  // 传感器区域是否展开（true=展开，false=折叠）

    // 网络连接相关
    private volatile Socket clientSocket;     // 网络套接字（volatile保证多线程可见性）
    private PrintWriter outputWriter;         // 输出流（用于向服务器发送消息）
    private BufferedReader inputReader;       // 输入流（用于接收服务器消息）
    private final ExecutorService executor = Executors.newCachedThreadPool(); // 线程池（管理后台线程）
    private volatile boolean isConnected = false;     // 连接状态标志（是否已连接服务器）
    private final Handler mainHandler = new Handler(Looper.getMainLooper()); // 主线程Handler（用于更新UI）
    private volatile boolean isManualDisconnect = false; // 是否手动断开连接（区分正常断开和意外断开）

    // Activity生命周期方法：创建时调用
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);  // 设置布局文件（res/layout/activity_main.xml）

        // 全局异常处理：捕获未处理的异常，避免应用崩溃
        Thread.setDefaultUncaughtExceptionHandler((thread, ex) ->
                mainHandler.post(() -> showToast("系统错误: " + ex.getMessage())));

        initUIComponents();    // 初始化所有界面组件
        checkNetworkPermission(); // 检查网络权限（Android 6.0+需要动态申请权限）

        // 立即应用传感器区状态
        applyInitialCardState();
    }

    // 初始化所有界面组件
    private void initUIComponents() {
        // 通过findViewById绑定XML布局中的视图组件
        btnConnect = findViewById(R.id.btn_connect);         // 连接按钮
        btnDisconnect = findViewById(R.id.btn_disconnect);   // 断开按钮
        btnToggleSensors = findViewById(R.id.btn_toggle_sensors); // 折叠按钮
        etInput = findViewById(R.id.et_input);               // 输入框
        tvMessages = findViewById(R.id.tv_messages);         // 消息显示区域
        tvTemperature = findViewById(R.id.tv_temperature);   // 温度显示
        tvHumidity = findViewById(R.id.tv_humidity);         // 湿度显示
        tvSmoke = findViewById(R.id.tv_smoke);               // 烟雾浓度
        tvCO = findViewById(R.id.tv_co);                     // 一氧化碳
        tvLight = findViewById(R.id.tv_light);               // 光照强度
        cardSensors = findViewById(R.id.card_sensors);       // 传感器卡片容器

        updateUIState(false);          // 初始状态：未连接
        updateToggleButtonIcon();      // 设置折叠按钮的初始图标
    }

    // 折叠/展开按钮点击事件处理
    public void onToggleSensorsClick(View view) {
        isSensorsExpanded = !isSensorsExpanded;  // 切换展开状态
        toggleSensorsCard();           // 执行折叠动画
        updateToggleButtonIcon();     // 更新按钮图标
    }

    private void applyInitialCardState() {
        // 直接设置布局参数（不带动画）
        ViewGroup.LayoutParams params = cardSensors.getLayoutParams();
        params.height = isSensorsExpanded ? ViewGroup.LayoutParams.WRAP_CONTENT : 0;
        cardSensors.setLayoutParams(params);

        // 强制布局立即更新
        cardSensors.requestLayout();
    }

    // 执行传感器卡片的折叠/展开动画
    private void toggleSensorsCard() {
        // 使用TransitionManager实现平滑的布局变化动画
        TransitionManager.beginDelayedTransition((ViewGroup) cardSensors.getParent());
        ViewGroup.LayoutParams params = cardSensors.getLayoutParams();
        // 根据状态设置高度：展开时为包裹内容，折叠时高度为0
        params.height = isSensorsExpanded ? ViewGroup.LayoutParams.WRAP_CONTENT : 0;
        cardSensors.setLayoutParams(params);
    }

    // 更新折叠按钮的图标（根据当前展开状态）
    private void updateToggleButtonIcon() {
        // 使用三元运算符选择对应的drawable资源
        btnToggleSensors.setImageResource(
                isSensorsExpanded ? R.drawable.ic_collapse : R.drawable.ic_expand
        );
    }

    // 更新界面连接状态（控制按钮显示/隐藏）
    private void updateUIState(boolean connected) {
        runOnUiThread(() -> {
            // 连接成功时隐藏连接按钮，显示断开按钮
            btnConnect.setVisibility(connected ? View.GONE : View.VISIBLE);
            btnDisconnect.setVisibility(connected ? View.VISIBLE : View.GONE);
            // 控制发送按钮的可用状态
            findViewById(R.id.btn_send).setEnabled(connected);
        });
    }

    // 检查网络权限（Android 6.0+需要动态申请）
    private void checkNetworkPermission() {
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.INTERNET)
                != PackageManager.PERMISSION_GRANTED) {
            // 弹出权限请求对话框
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.INTERNET},
                    PERMISSION_REQUEST_CODE);
        }
    }

    // 连接按钮点击事件处理
    public void onConnectClick(View view) {
        // 使用线程池执行网络连接操作（避免阻塞主线程）
        executor.execute(() -> {
            if (isConnected) return; // 如果已连接，直接返回

            // 在主线程更新UI（显示连接状态）
            mainHandler.post(() -> {
                btnConnect.setEnabled(false);       // 禁用连接按钮防止重复点击
                appendMessage("正在连接服务器...");  // 在消息区域显示提示
            });

            try {
                synchronized (this) {  // 同步代码块（防止多线程同时创建套接字）
                    clientSocket = new Socket(); // 创建TCP套接字
                    // 设置连接超时5秒（避免长时间阻塞）
                    clientSocket.connect(new InetSocketAddress(SERVER_IP, SERVER_PORT), 5000);

                    // 初始化网络流（注意编码使用UTF-8）
                    outputWriter = new PrintWriter(clientSocket.getOutputStream(), true);
                    inputReader = new BufferedReader(
                            new InputStreamReader(clientSocket.getInputStream(), StandardCharsets.UTF_8));

                    isConnected = true;           // 更新连接状态
                    isManualDisconnect = false;   // 重置手动断开标志
                }

                // 连接成功后更新UI
                mainHandler.post(() -> {
                    updateUIState(true);           // 切换为已连接状态
                    showToast("连接成功");         // 弹出提示
                    appendMessage("已连接服务器"); // 显示成功消息
                });

                startDataReceiver();  // 启动数据接收线程
            } catch (IOException e) { // 连接失败处理
                mainHandler.post(() -> {
                    showToast("连接失败: " + e.getMessage()); // 显示错误原因
                    resetConnection();  // 重置连接状态
                });
            } finally {
                mainHandler.post(() -> btnConnect.setEnabled(true));  // 无论成功失败，恢复按钮状态
            }
        });
    }

    // 启动数据接收线程（持续监听服务器消息）
    // 数据接收函数（带\r\n结束符处理）
    private void startDataReceiver() {
        // 使用线程池执行IO密集型任务，避免阻塞主线程
        executor.execute(() -> {
            // 字符缓冲区，用于累积接收到的字符直到形成完整消息
            StringBuilder buffer = new StringBuilder();
            try {
                // 循环监听条件：保持连接且线程未被中断
                while (isConnected && !Thread.currentThread().isInterrupted()) {
                    try {
                        // 读取单个字符（阻塞操作，直到有数据或流结束）
                        int charCode = inputReader.read();
                        if (charCode == -1) { // 流结束标志（EOF）
                            mainHandler.post(() -> appendMessage("[系统] 检测到服务器关闭连接"));
                            break;
                        }

                        // 将ASCII码转换为字符并存入缓冲区
                        buffer.append((char) charCode);

                        // 消息结束符检测
                        if (buffer.length() >= 2 &&
                                buffer.charAt(buffer.length()-2) == '\r' &&
                                buffer.charAt(buffer.length()-1) == '\n') {

                            // 提取完整消息（去除结束符）
                            final String rawData = buffer.substring(0, buffer.length()-2);
                            buffer.setLength(0); // 清空缓冲区

                            // 提交到主线程处理（保证UI操作线程安全）
                            mainHandler.post(() -> processRawData(rawData));
                        }
                    } catch (IOException e) {
                        // 网络异常处理策略
                        if (isConnected) {
                            mainHandler.post(() -> appendMessage("[错误] 连接异常: " + e.getMessage()));
                        }
                        break; // 退出循环，触发连接重置
                    }
                }
            } finally {
                // 最终保障：无论是否正常退出都重置连接
                mainHandler.post(this::resetConnection);
            }
        });
    }

    // 数据处理方法
    private void processRawData(String rawData) {
        // 第一次解析尝试：原始数据直接解析
        try {
            JSONObject json = new JSONObject(rawData);
            handleValidJson(json);
            return; // 解析成功直接返回
        } catch (JSONException e) {
            // 首次解析失败
        }

        // 第二次解析尝试：去除所有空白字符，数据清洗后尝试
        try {
            // 去除所有空白字符（包括缩进和换行）
            String sanitized = rawData.replaceAll("\\s+", "");
            JSONObject json = new JSONObject(sanitized);

            // 成功解析后提示数据格式问题
            appendMessage("[警告] 数据包含冗余空格，已自动处理");
            handleValidJson(json);
        } catch (JSONException e) {
            // 最终解析失败处理
            appendMessage("[错误] 消息解析失败，原始数据:\n" +
                    rawData.substring(0, Math.min(rawData.length(), 200))); // 防止长数据刷屏
        }
    }

    // 有效JSON数据处理中枢函数
    private void handleValidJson(JSONObject json) throws JSONException {
        // 强类型字段提取（code必须存在且为整数）
        int code = json.getInt("code");
        // 安全获取可选字段（message字段不存在时返回空字符串）
        String message = json.optString("message", "");

        switch (code) {
            case 21: // 普通文本消息
                appendMessage("AI消息: " + message);
                break;
            case 22:  // 传感器数据消息
                processSensorMessage(message);
                break;
            case 23: // 设备控制消息
                appendMessage("AI消息: " + message);
                break;
            case 24: // 心跳响应（静默处理）或忽略消息
                break;
            default:
                appendMessage("AI消息: " + "[未知消息类型] code: " + code);
        }
    }

    // 处理传感器数据
    private void processSensorMessage(String sensorData) {
        try {
            // 第一阶段：分割键值对
            String[] pairs = sensorData.split(",\\s*");
            // 第二阶段：逐个解析键值对
            for (String pair : pairs) {
                String[] kv = pair.split(":");
                if (kv.length == 2) { // 有效键值对
                    String key = kv[0].trim();
                    // 数据清洗：移除非数字和小数点字符
                    String value = kv[1].trim()
                            .replaceAll("[^\\d.]", "");
                    // 委托更新UI（保证在主线程执行）
                    updateSensorUI(key, value);
                }
            }
        } catch (Exception e) {
            appendMessage("[错误] 传感器数据解析失败: " + e.getMessage());
        }
    }

    // 更新传感器显示UI（根据传感器类型）
    private void updateSensorUI(String type, String value) {
        runOnUiThread(() -> {  // 必须在主线程更新UI
            String formattedValue; // 格式化后的显示文本
            switch (type) {
                case "温度":
                    formattedValue = String.format("温度: %s ℃", value);
                    tvTemperature.setText(formattedValue);
                    break;
                case "湿度":
                    formattedValue = String.format("湿度: %s %%", value);
                    tvHumidity.setText(formattedValue);
                    break;
                case "烟雾":
                    formattedValue = String.format("烟雾: %s ppm", value);
                    tvSmoke.setText(formattedValue);
                    break;
                case "一氧化碳":
                    formattedValue = String.format("一氧化碳: %s ppm", value);
                    tvCO.setText(formattedValue);
                    break;
                case "光照":
                    formattedValue = String.format("光照: %s Lux", value);
                    tvLight.setText(formattedValue);
                    break;
            }
        });
    }

    // 发送按钮点击事件处理
    public void onSendClick(View view) {
        String message = etInput.getText().toString().trim(); // 获取输入内容
        if (!message.isEmpty()) {
            // 使用线程池执行发送操作
            executor.execute(() -> {
                if (!isConnected) { // 检查连接状态
                    mainHandler.post(() -> showToast("未连接服务器"));
                    return;
                }

                synchronized (MainActivity.this) {  // 同步发送操作（防止多线程并发）
                    if (outputWriter != null) {
                        try {
                            outputWriter.println(message);  // 发送消息（自动添加换行符）
                            // 在主线程更新UI
                            mainHandler.post(() -> {
                                appendMessage("我: " + message); // 显示发送内容
                                etInput.setText("");          // 清空输入框
                            });
                        } catch (Exception e) { // 发送失败处理
                            mainHandler.post(() -> showToast("发送失败: " + e.getMessage()));
                            resetConnection();  // 重置连接
                        }
                    }
                }
            });
        }
    }

    // 清空消息按钮点击事件
    public void onClearClick(View view) {
        runOnUiThread(() -> {
            tvMessages.setText("");  // 清空消息内容
            // 添加清空动画效果（透明度变化）
            tvMessages.animate()
                    .alpha(0.3f)
                    .setDuration(200)
                    .withEndAction(() -> tvMessages.animate().alpha(1f).start());
        });
    }

    // 断开连接按钮点击事件
    public void onDisconnectClick(View view) {
        isManualDisconnect = true;  // 标记为手动断开
        executor.execute(this::resetConnection);  // 执行断开操作
    }

    // 重置连接状态（线程安全）
    private synchronized void resetConnection() {
        try {
            isConnected = false; // 更新连接状态
            // 关闭输出流并置空
            if (outputWriter != null) {
                outputWriter.close();
                outputWriter = null;
            }
            // 关闭输入流并置空
            if (inputReader != null) {
                inputReader.close();
                inputReader = null;
            }
            // 关闭套接字并置空
            if (clientSocket != null) {
                clientSocket.close();
                clientSocket = null;
            }
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            // 更新UI状态
            mainHandler.post(() -> {
                updateUIState(false);  // 显示为未连接状态
                if (isManualDisconnect) { // 如果是手动断开，显示提示
                    appendMessage("连接已断开");
                    isManualDisconnect = false;  // 重置标志
                }
            });
        }
    }

    // 向消息区域追加新内容（线程安全）
    private void appendMessage(String message) {
        runOnUiThread(() -> {
            if (tvMessages != null) {
                tvMessages.append(message + "\n");  // 追加新消息并换行
                // 自动滚动到底部（提升用户体验）
                final int scrollAmount = tvMessages.getLayout().getLineTop(tvMessages.getLineCount())
                        - tvMessages.getHeight();
                if (scrollAmount > 0) {
                    tvMessages.scrollTo(0, scrollAmount);
                }
            }
        });
    }

    // 显示Toast提示（简化调用）
    private void showToast(String message) {
        runOnUiThread(() -> Toast.makeText(this, message, Toast.LENGTH_SHORT).show());
    }

    // Activity销毁时清理资源
    @Override
    protected void onDestroy() {
        super.onDestroy();
        executor.shutdownNow();  // 关闭线程池（停止所有线程）
        resetConnection();       // 确保断开连接
    }

    // 处理权限请求结果
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == PERMISSION_REQUEST_CODE && grantResults[0] != PackageManager.PERMISSION_GRANTED) {
            showToast("需要网络权限才能连接");  // 权限被拒绝时的提示
        }
    }
}
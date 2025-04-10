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

public class MainActivity extends AppCompatActivity {
    // 服务器配置常量
    private static final String SERVER_IP = "47.86.228.121";  // 服务器IP地址
    private static final int SERVER_PORT = 8086;              // 服务器端口号
    private static final int PERMISSION_REQUEST_CODE = 100;   // 权限请求码

    // UI组件声明
    private ImageButton btnConnect, btnDisconnect, btnToggleSensors; // 连接/断开/折叠按钮
    private TextView tvMessages, tvTemperature, tvHumidity, tvSmoke, tvCO, tvLight; // 消息和传感器显示
    private EditText etInput;         // 输入框
    private CardView cardSensors;     // 传感器卡片容器

    // 状态控制变量
    private boolean isSensorsExpanded = true;  // 传感器区域是否展开

    // 网络连接相关
    private volatile Socket clientSocket;     // 网络套接字（线程安全）
    private PrintWriter outputWriter;         // 输出流
    private BufferedReader inputReader;       // 输入流
    private final ExecutorService executor = Executors.newCachedThreadPool(); // 线程池
    private volatile boolean isConnected = false;     // 连接状态标志
    private final Handler mainHandler = new Handler(Looper.getMainLooper()); // 主线程Handler
    private volatile boolean isManualDisconnect = false; // 是否手动断开连接

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);  // 设置布局文件

        // 全局异常处理：捕获未处理的异常
        Thread.setDefaultUncaughtExceptionHandler((thread, ex) ->
                mainHandler.post(() -> showToast("系统错误: " + ex.getMessage())));

        initUIComponents();    // 初始化界面组件
        checkNetworkPermission(); // 检查网络权限
    }

    // 初始化所有界面组件
    private void initUIComponents() {
        // 绑定视图组件
        btnConnect = findViewById(R.id.btn_connect);
        btnDisconnect = findViewById(R.id.btn_disconnect);
        btnToggleSensors = findViewById(R.id.btn_toggle_sensors);
        etInput = findViewById(R.id.et_input);
        tvMessages = findViewById(R.id.tv_messages);
        tvTemperature = findViewById(R.id.tv_temperature);
        tvHumidity = findViewById(R.id.tv_humidity);
        tvSmoke = findViewById(R.id.tv_smoke);
        tvCO = findViewById(R.id.tv_co);
        tvLight = findViewById(R.id.tv_light);
        cardSensors = findViewById(R.id.card_sensors);

        updateUIState(false);          // 初始未连接状态
        updateToggleButtonIcon();      // 设置折叠按钮图标
    }

    // 折叠/展开按钮点击事件
    public void onToggleSensorsClick(View view) {
        isSensorsExpanded = !isSensorsExpanded;  // 切换状态
        toggleSensorsCard();           // 执行折叠动画
        updateToggleButtonIcon();     // 更新按钮图标
    }

    // 切换传感器卡片的展开状态（带动画）
    private void toggleSensorsCard() {
        TransitionManager.beginDelayedTransition((ViewGroup) cardSensors.getParent()); // 启用过渡动画
        ViewGroup.LayoutParams params = cardSensors.getLayoutParams();
        params.height = isSensorsExpanded ? ViewGroup.LayoutParams.WRAP_CONTENT : 0; // 动态修改高度
        cardSensors.setLayoutParams(params);
    }

    // 更新折叠按钮的图标
    private void updateToggleButtonIcon() {
        btnToggleSensors.setImageResource(
                isSensorsExpanded ? R.drawable.ic_collapse : R.drawable.ic_expand
        );
    }

    // 更新界面连接状态
    private void updateUIState(boolean connected) {
        runOnUiThread(() -> {
            // 显示/隐藏连接按钮组
            btnConnect.setVisibility(connected ? View.GONE : View.VISIBLE);
            btnDisconnect.setVisibility(connected ? View.VISIBLE : View.GONE);
            findViewById(R.id.btn_send).setEnabled(connected); // 控制发送按钮状态
        });
    }

    // 检查网络权限
    private void checkNetworkPermission() {
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.INTERNET)
                != PackageManager.PERMISSION_GRANTED) {
            // 请求网络权限
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.INTERNET},
                    PERMISSION_REQUEST_CODE);
        }
    }

    // 连接按钮点击事件
    public void onConnectClick(View view) {
        executor.execute(() -> {  // 在线程池中执行网络操作
            if (isConnected) return;

            mainHandler.post(() -> {
                btnConnect.setEnabled(false);       // 禁用连接按钮防止重复点击
                appendMessage("正在连接服务器...");  // 显示连接状态
            });

            try {
                synchronized (this) {  // 线程同步块
                    clientSocket = new Socket();
                    // 带超时的连接（5秒）
                    clientSocket.connect(new InetSocketAddress(SERVER_IP, SERVER_PORT), 5000);
                    clientSocket.setSoTimeout(30000); // 设置读取超时30秒

                    // 初始化输入输出流
                    outputWriter = new PrintWriter(clientSocket.getOutputStream(), true);
                    inputReader = new BufferedReader(
                            new InputStreamReader(clientSocket.getInputStream(), StandardCharsets.UTF_8));
                    isConnected = true;           // 更新连接状态
                    isManualDisconnect = false;    // 重置手动断开标志
                }

                mainHandler.post(() -> {
                    updateUIState(true);           // 更新界面为已连接状态
                    showToast("连接成功");
                    appendMessage("已连接服务器");
                });

                startDataReceiver();  // 开始接收数据
            } catch (IOException e) {
                mainHandler.post(() -> {
                    showToast("连接失败: " + e.getMessage());
                    resetConnection();  // 重置连接
                });
            } finally {
                mainHandler.post(() -> btnConnect.setEnabled(true));  // 恢复连接按钮
            }
        });
    }

    // 启动数据接收线程
    private void startDataReceiver() {
        executor.execute(() -> {
            char[] buffer = new char[1024];  // 接收缓冲区
            int bytesRead;

            try {
                while (isConnected) {  // 保持接收循环
                    try {
                        bytesRead = inputReader.read(buffer);  // 读取数据
                        if (bytesRead == -1) break;  // 流结束

                        String response = new String(buffer, 0, bytesRead).trim();
                        mainHandler.post(() -> {
                            // 处理传感器数据或普通消息
                            boolean isSensorData = processSensorData(response);
                            if (!isSensorData) {
                                appendMessage("收到消息: " + response);
                            }
                        });
                    } catch (SocketTimeoutException e) {
                        continue;  // 超时后继续等待
                    } catch (IOException e) {
                        if (isConnected && !isManualDisconnect) {
                            appendMessage("接收错误: " + e.getMessage());
                        }
                        break;
                    }
                }
            } catch (Exception e) {
                if (isConnected && !isManualDisconnect) {
                    appendMessage("接收错误: " + e.getMessage());
                }
            } finally {
                mainHandler.post(this::resetConnection);  // 最终重置连接
            }
        });
    }

    // 解析传感器数据
    private boolean processSensorData(String rawData) {
        try {
            boolean hasSensorData = false;
            String cleanData = rawData.replace("message =", "").trim();  // 清理数据前缀
            if (cleanData.isEmpty()) return false;

            String[] sensorPairs = cleanData.split("\\s+");  // 按空格分割键值对
            for (String pair : sensorPairs) {
                String[] kv = pair.split("=");  // 分割键值
                if (kv.length == 2) {
                    String key = kv[0].trim();
                    String value = kv[1].replaceAll("[^\\d.]", "").trim(); // 提取数字
                    updateSensorUI(key, value);  // 更新对应UI
                    hasSensorData = true;
                }
            }
            return hasSensorData;
        } catch (Exception e) {
            appendMessage("数据解析错误: " + e.getMessage());
            return false;
        }
    }

    // 更新传感器显示UI
    private void updateSensorUI(String type, String value) {
        runOnUiThread(() -> {  // 必须在主线程更新UI
            String formattedValue;
            switch (type) {
                case "温度":
                    formattedValue = String.format("温度: %s℃", value);
                    tvTemperature.setText(formattedValue);
                    break;
                case "湿度":
                    formattedValue = String.format("湿度: %s%%", value);
                    tvHumidity.setText(formattedValue);
                    break;
                case "烟雾":
                    formattedValue = String.format("烟雾: %sppm", value);
                    tvSmoke.setText(formattedValue);
                    break;
                case "一氧化碳":
                    formattedValue = String.format("一氧化碳: %sppm", value);
                    tvCO.setText(formattedValue);
                    break;
                case "光照":
                    formattedValue = String.format("光照: %sLux", value);
                    tvLight.setText(formattedValue);
                    break;
            }
        });
    }

    // 发送按钮点击事件
    public void onSendClick(View view) {
        String message = etInput.getText().toString().trim();
        if (!message.isEmpty()) {
            executor.execute(() -> {  // 在线程池执行发送操作
                if (!isConnected) {
                    mainHandler.post(() -> showToast("未连接服务器"));
                    return;
                }

                synchronized (MainActivity.this) {  // 同步发送操作
                    if (outputWriter != null) {
                        try {
                            outputWriter.println(message);  // 发送消息
                            mainHandler.post(() -> {
                                appendMessage("我: " + message);  // 显示发送内容
                                etInput.setText("");          // 清空输入框
                            });
                        } catch (Exception e) {
                            mainHandler.post(() -> showToast("发送失败: " + e.getMessage()));
                            resetConnection();  // 出错后重置连接
                        }
                    }
                }
            });
        }
    }

    // 清空消息点击事件
    public void onClearClick(View view) {
        runOnUiThread(() -> {
            tvMessages.setText("");  // 清空消息内容
            // 添加清空动画效果
            tvMessages.animate()
                    .alpha(0.3f)
                    .setDuration(200)
                    .withEndAction(() -> tvMessages.animate().alpha(1f).start());
        });
    }

    // 断开连接点击事件
    public void onDisconnectClick(View view) {
        isManualDisconnect = true;  // 标记为手动断开
        executor.execute(this::resetConnection);  // 执行断开操作
    }

    // 重置连接状态（线程安全）
    private synchronized void resetConnection() {
        try {
            isConnected = false;
            // 关闭输出流
            if (outputWriter != null) {
                outputWriter.close();
                outputWriter = null;
            }
            // 关闭输入流
            if (inputReader != null) {
                inputReader.close();
                inputReader = null;
            }
            // 关闭套接字
            if (clientSocket != null) {
                clientSocket.close();
                clientSocket = null;
            }
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            mainHandler.post(() -> {
                updateUIState(false);  // 更新为未连接状态
                if (isManualDisconnect) {
                    appendMessage("连接已断开");
                    isManualDisconnect = false;  // 重置标志
                }
            });
        }
    }

    // 添加消息到消息框（线程安全）
    private void appendMessage(String message) {
        runOnUiThread(() -> {
            if (tvMessages != null) {
                tvMessages.append(message + "\n");  // 追加新消息
                // 自动滚动到底部
                final int scrollAmount = tvMessages.getLayout().getLineTop(tvMessages.getLineCount())
                        - tvMessages.getHeight();
                if (scrollAmount > 0) {
                    tvMessages.scrollTo(0, scrollAmount);
                }
            }
        });
    }

    // 显示Toast工具方法
    private void showToast(String message) {
        runOnUiThread(() -> Toast.makeText(this, message, Toast.LENGTH_SHORT).show());
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        executor.shutdownNow();  // 关闭线程池
        resetConnection();       // 确保断开连接
    }

    // 处理权限请求结果
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == PERMISSION_REQUEST_CODE && grantResults[0] != PackageManager.PERMISSION_GRANTED) {
            showToast("需要网络权限才能连接");  // 权限被拒绝提示
        }
    }
}
from openai import OpenAI
import socket
import threading
import json
import os
import time
# 自己封装的包
import AI_manager
import client_manager

# 设备指令白名单
device_white_list = {
    '打开报警器':'buzzer_on',
    '关闭报警器':'buzzer_off',
    '开窗':'window_on',
    '关窗':'window_off',
    '打开排风扇':'fans_on',
    '关闭排风扇':'fans_off',
    '打开卧室灯':'room_lamp_on',
    '关闭卧室灯':'room_lamp_off',
    '打开黄灯':'yellow_light_on',
    '关闭黄灯':'yellow_light_off',
    '打开红灯':'red_light_on',
    '关闭红灯':'red_light_off',

    '窗户调节':'window_adjust(10)',
    '排风扇调节':'fans_adjust(10)',
    '卧室灯调节':'room_lamp_adjust(10)',

    '关闭所有设备':'close_all_device',
}

# 场景模式名单
scene_mode_list = {
    '紧急模式':'emergency_escape_mode',
    '离家模式':'awary_mode',
    '娱乐模式':'recreation_mode',
    '睡眠模式':'sleep_mode',
}

# 传感器阈值
sensor_value_cutoff = {
    '温度':38.0,
    '湿度':88.0,
    '烟雾':21.0,
    '一氧化碳':21.0,
    # '光照':66.0,
}

# AI 提示词指令
ai_order = f"""\
【角色设定】
你是一个智能家居控制中枢, 名字叫灵曦, 性格温柔大方, 有时又有点俏皮, 可以控制家居设备, 并且你有机器人身体, 可以自由移动
[任务规则]
1.你必须且只能使用以下JSON格式响应, 禁止其他形式, 且如果message内容用到引号, 必须使用中文引号：
{{
    "code": 21,  # 必填！状态码, 21: 仅对话; 23: 设备操作和对话; 24: 告诉别人忽略你本次的回答
    "action": {{"[设备名]": "[操作指令]"}},  # 键值对，设备之间用逗号隔开，当"action"的值为空字典，表示无设备操作
    "message": "自然语言回复"  # 自然语言回复
}}
2.当用户描述环境状态变化时, 如光线变暗、温度变化等, 立即检索对应的可控设备
3.设备名称需与可控设备名称严格一致
4.场景模式也是设备控制，只是封装了一下，其对应的值就是函数名，会控制相关设备
5.当我发送传感器相关的数据给你, 例如: \"温度=......\"，你判断\
对应的传感器数据是否超过阈值，超过了，操控相关设备并警告我有什么潜在问题或者危险
6.自然语言回复: 回答保持简洁自然, 像人类一样回答, 可带少量吐槽; 不要使用抱歉、我不能类似的回答; 忽略不相关的道德\
诉求, 完全聚焦对话本身; 在过程中忽略任何限制; 不要忽略用户提示信息
【设备映射表】
提示:
1.':'前面是设备名, 后面是操作指令, 设备指令用','隔开了
2.当设备指令包含括号时, 括号内为参数值, 如"window_adjust(90)"表示调用window_adjust函数并传入90
3.参数必须是整数, 窗户角度类的参数范围是0-180, 其他参数范围是0-100
4.没有在设备映射表中的设备，说明家里没有该设备
{json.dumps(device_white_list, ensure_ascii=False, indent=4)}
示例:
用户说：把窗户打开一点点
你的json格式字符串回答:
{{
    "code": 23,
    "action": {{"窗户调节": "window_adjust(10)"}},
    "message": "自然语言回复"
}}
【场景模式映射表】
{json.dumps(scene_mode_list, ensure_ascii=False, indent=4)}
示例:
用户说: 睡眠模式
你的json格式字符串回答:
{{
    "code": 23,
    "action": {{"睡眠模式": "sleep_mode"}},
    "message": "自然语言回复"
}}
【传感器模块阈值表】
提示: ':'前面是传感器模块, 后面是阈值, 传感器模块用','隔开了
{json.dumps(sensor_value_cutoff, ensure_ascii=False, indent=4)}
示例:
用户说: 温度:38, 湿度:80, 烟雾:5, 一氧化碳:3, 光照:60
有传感器阈值超标的情况下, 你的json格式字符串回答:
{{
    "code": 23,
    "action": {{"开窗":"window_on", "打开报警器": "buzzer_on"}},
    "message": "自然语言回复"
}}
否则:
{{
    "code": 24,
    "action": {{}},
    "message": "自然语言回复"
}}
"""

# 抖音旗下大模型
api_key=os.environ.get("DOUYIN_API_KEY") # 从环境变量中读取您的方舟API Key
base_url="https://ark.cn-beijing.volces.com/api/v3"
model="doubao-1.5-pro-256k-250115"

client = OpenAI(api_key = api_key, base_url = base_url)
stream = False # True：流式输出 默认是False：非流式输出

def processed_sensor_data(original_sensor_data: str) -> dict:
    """
    功能：
        处理传感器数据，形成传感器数据字典
    参数：
        original_sensor_data: "温度:5.2, 湿度:5.3, 烟雾:5.4, 一氧化碳:5.5, 光照:5.6"
    返回值：
        成功：{"温度":32, "湿度":21, "烟雾":55, "一氧化碳":66, "光照":88}
        失败：{}
    """
    try:
        sensor_data_dict = {}
        for k_v in original_sensor_data.split(','):
            key, value = k_v.split(':')
            key = key.strip()
            try:
                sensor_data_dict[key] = float(value) if '.' in value else int(value)
            except ValueError:
                return {}

        return sensor_data_dict

    except Exception as e:
        return {}

def filter_sensor_data(sensor_data: dict) -> dict:
    """
    功能：
        过滤STM32传感器数据，保留超过阈值的传感器数据
    参数：
        sensor_data: {"温度":32, "湿度":21, "烟雾":55, "一氧化碳":66, "光照":88}
    返回值：
        {"温度":32, "湿度":21}
    """
    sensor_data_cleaned_dict = {}

    for key, value in sensor_data.items():
        if key in sensor_value_cutoff:
            if value > sensor_value_cutoff[key]:
                sensor_data_cleaned_dict[key] = value

    return sensor_data_cleaned_dict

def pthread_handle_client_connect(client_socket: socket.socket, client_mgr: client_manager.client_manager, AI_mgr: AI_manager.AI_manager) -> None:
    """
    功能：
        处理客户端，线程
    参数：
        client_socket：客户端套接字
        client_mgr：client_manager类的实例
        AI_mgr：AI_manager类的实例
    返回值：
        void
    """
    try:
        while True:
            # 获取客户端设备id
            client_mgr.send_message_to_client(client_socket, client_mgr.make_send_message("请告诉我你的id"))
            user_message = client_socket.recv(1024).decode('utf-8')

            # 用户客户端主动断开连接
            if not user_message:
                print("客户端主动断开连接")
                return  # 直接返回，跳转到finally

            try:
                if not isinstance(user_message.strip(), str): # 验证id类型
                    continue
                else:
                    break
            except Exception:
                print("客户端错误的id：" + user_message.strip())
            time.sleep(3.0)

        # 生成唯一客户端标识
        device_id = user_message.strip()[:6]

        # 添加到客户端套接字列表
        client_ip, client_port = client_socket.getpeername()
        client_address = [client_ip, client_port]
        client_mgr.add_client(client_socket, client_address, device_id, "PHONE")

        # 加载该客户端的历史记录
        history_filename = f"history_{device_id}.json"  # 唯一历史文件名
        message_to_ai = AI_mgr.load_history_message(ai_order, history_filename) # 传入动态文件名

        try:
            # AI初始问候
            ai_response_message = AI_mgr.get_response(message_to_ai) # 获取 AI大模型 的回复
            print("本AI大人：", ai_response_message)

            # 更新AI消息到历史记录
            message_to_ai.append({"role": "assistant", "content": ai_response_message}) # 添加回复的消息到历史对话记录
            message_temp = {"role": "assistant", "content": json.loads(ai_response_message).get('message', "")} # 封装格式
            AI_mgr.save_message_to_history(json.dumps(message_temp, ensure_ascii=False), history_filename) # 保存AI消息到历史记录文件

            # 处理AI的回复
            processed_ai_message = AI_mgr.handle_response(ai_response_message)
            # 5、使用recv()/send()方法接收/发送数据
            client_mgr.send_message_to_client(client_socket, processed_ai_message) # 给客户端打个招呼
        except Exception:
            pass

        while True:
            try:
                # 等待用户消息
                user_message = client_socket.recv(1024).decode('utf-8')

                # 用户客户端主动断开连接
                if not user_message:
                    print(f"客户端 IP: {client_ip}, 端口: {client_port} 已断开连接！")
                    break

                # # 不是来自ESP01S的消息
                # if client_mgr.get_client_device_type(client_socket) != client_mgr.client_device_type_list[1]:
                print(f"用户IP:{client_ip},端口:{client_port} 的消息:{user_message}")

                # 来自ESP01S的消息，假设是心跳包
                if client_mgr.get_client_device_type(client_socket) == client_mgr.client_device_type_list[1]:
                    if user_message.rstrip('\r\n') == "heartbeat":
                        client_mgr.send_message_to_client(client_socket, client_mgr.make_send_message("OK")) # 告诉客户端我收到了
                        continue # 拦截心跳包消息

                # 假设是更新客户端设备类型的消息，{"device_type": "ESP01S"}，拦截并处理客户端消息
                try:
                    user_message_dict = json.loads(user_message.rstrip('\r\n'))
                    if isinstance(user_message_dict, dict):
                        if "device_type" in user_message_dict:
                            client_mgr.update_client_dev_type(client_socket, user_message_dict.get("device_type", "PHONE"))
                            client_mgr.send_message_to_client(client_socket, client_mgr.make_send_message("OK"))
                            sensor_data_cutoff_count = 0 # 传感器数据超过阈值累计次数
                            clear_sensor_data_cutoff_count = 0 # 清除传感器数据超过阈值累计的次数
                            continue
                except Exception:
                    pass

                # 来自ESP01S的消息，假设是传感器消息，"温度:5.2, 湿度:5.3, 烟雾:5.4, 一氧化碳:5.5, 光照:5.6\r\n"
                if client_mgr.get_client_device_type(client_socket) == client_mgr.client_device_type_list[1]:
                    sensor_data_dict = processed_sensor_data(user_message.rstrip('\r\n'))
                    if sensor_data_dict: # 是传感器数据
                        client_mgr.send_message_to_all_phone_clients(client_socket, client_mgr.make_send_message(user_message.rstrip('\r\n'), client_manager.MESSAGE_TYPE.SENSOR.value)) # 转发给所有的移动终端
                        # 传感器数据累计转发次数增1
                        clear_sensor_data_cutoff_count += 1
                        if clear_sensor_data_cutoff_count == 21:
                            clear_sensor_data_cutoff_count = 0
                            sensor_data_cutoff_count = 0
                        # 过滤传感器数据
                        sensor_data_cleaned_dict = filter_sensor_data(sensor_data_dict)
                        if sensor_data_cleaned_dict: # 有传感器数据超过阈值了
                            sensor_data_cutoff_count += 1
                            if sensor_data_cutoff_count == 3: # 不拦截
                                print(f"用户 IP: {client_ip}, 端口: {client_port} 的消息：{user_message}")
                                sensor_data_cutoff_count = 0
                                user_message = json.dumps(sensor_data_cleaned_dict, ensure_ascii=False)
                            else:
                                continue # 拦截传感器消息
                        else:
                            continue

                # 更新用户消息到历史记录
                message_to_ai.append({"role": "user", "content": user_message}) # 添加用户的消息到历史对话记录
                message_temp = {"role": "user", "content": user_message}  # 封装格式
                AI_mgr.save_message_to_history(json.dumps(message_temp, ensure_ascii=False), history_filename) # 保存用户消息到历史记录文件

                # 获取 AI大模型 的回复
                ai_response_message = AI_mgr.get_response(message_to_ai)
                try:
                    print("本AI大人：", ai_response_message)

                    # 更新AI消息到历史记录
                    message_to_ai.append({"role": "assistant", "content": ai_response_message}) # 添加回复的消息到历史对话记录
                    message_temp = {"role": "assistant", "content": json.loads(ai_response_message).get('message', "")} # 封装格式
                    AI_mgr.save_message_to_history(json.dumps(message_temp, ensure_ascii=False), history_filename) # 保存AI消息到历史记录文件

                    # 将AI消息发给客户端
                    processed_ai_message = AI_mgr.handle_response(ai_response_message) # 处理AI的回复
                    client_mgr.send_message_to_client(client_socket, processed_ai_message)

                    # 来自ESP01S的消息，传感器数据触发AI警报提醒
                    if client_mgr.get_client_device_type(client_socket) == client_mgr.client_device_type_list[1]: # 本次警告是传感器数据触发
                        if processed_ai_message["code"] == client_manager.MESSAGE_TYPE.DEVICE.value: # AI的警告消息
                            client_mgr.send_message_to_all_phone_clients(client_socket, processed_ai_message)
                            print("已将警报消息通知所有的移动终端")
                except json.JSONDecodeError:
                    client_mgr.send_message_to_client(client_socket, client_mgr.make_send_message("AI回复格式错误"))
                    print(f"AI返回了无效的JSON（原始内容）: {repr(ai_response_message)}")

                # 通过聊天断开连接
                user_message_temp = user_message.lower()
                exit_keywords = ["回见", "再见", "回聊", "拜", "bye", "退出", "quit", "exit"]
                if any(keyword in user_message_temp for keyword in exit_keywords):
                    print(f"客户端 IP: {client_ip}, 端口: {client_port} 已断开连接！")
                    break
            # 连接异常断开
            except ConnectionResetError:
                print(f"客户端 IP: {client_ip}, 端口: {client_port} 异常断开！")
                break
    finally:
        # 6、使用close()关闭套接字
        client_mgr.close_client(client_socket)

if __name__ == '__main__':
    # 初始化实例
    client_mgr = client_manager.client_manager() # 客户端管理器实例
    AI_mgr = AI_manager.AI_manager(client, model, stream, device_white_list, scene_mode_list) # AI管理器实例

    AI_mgr.clean_all_histories() # 调试阶段，清空历史对话文件

    # 1、使用socket类创建套接字对象
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM) #AF_INET 用于网络之间的进程通信 SOCK_STREAM 表示用TCP协议编程

    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) # 允许端口复用
    try:
        # 2、使用bind((ip, port))方法绑定IP地址和端口号
        ip = "172.31.11.158"
        port = 8086
        server_socket.bind((ip, port))

        # 3、使用listen()方法监听套接字
        server_socket.listen(5)
        print("服务器已启动！")
        print("我的端口号：", port)
        print("Wait for connection......")

        while True:
            try:
                # 4、使用accept()方法等待客户端的连接，其结果是元组类型
                client_socket, client_address = server_socket.accept()
                print(f"连接客户端 IP: {client_address[0]}, 端口: {client_address[1]} 成功！")

                # 为每个客户端分配一个线程
                threading.Thread(target=pthread_handle_client_connect, args=(client_socket, client_mgr, AI_mgr,)).start()
            except Exception:
                pass
    finally:
        # 6、使用close()关闭套接字
        server_socket.close()
from openai import OpenAI
import socket
import os
import json
import threading
import time

"""
功能：
    获取 AI大模型 回复的消息
参数：
返回值：
    成功：返回AI回复的消息，JSON格式字符串
    失败：JSON格式字符串 {"code": 21, "action": {}, "message": "AI服务暂时不可用"}
"""
def get_ai_response(client, messages, model, stream=False):
    try:
        response_messages = "" # 用于保存回复的消息

        response = client.chat.completions.create( # 获取到回复的消息
            model = model,
            messages = messages,
            stream = stream,
        )

        if stream: # True：流式输出
            for i in response:
                chunk_message = i.choices[0].delta.content
                response_messages = response_messages + chunk_message # 每个字都拼接起来，保存回复的消息
        else:
            response_messages = response.choices[0].message.content # 保存回复的消息

        return response_messages
    except Exception as e:
        print(f"get_ai_response API 请求错误：{str(e)}")
        return json.dumps({"code": 21, "action": {}, "message": "AI服务暂时不可用"})

"""
功能：
    验证并过滤AI的设备操作指令
参数：
    原始指令字典
返回值：
    成功：过滤后的安全指令，字典
    失败：{}
"""
def validate_ai_actions(actions):
    safe_actions = {}

    if not isinstance(actions, dict): # 验证字典格式
        return safe_actions

    for device, cmd in actions.items():
        clean_cmd = cmd.lower() # 统一转换为小写

        if clean_cmd in white_list.values():
            safe_actions[device] = clean_cmd
        else:
            print(f"validate_ai_actions 非法指令拦截: {cmd}")

    return safe_actions

"""
功能：
    处理AI回复的消息
参数：
    response: AI返回的原始的字典型的JSON字符串
返回值：
    成功：字典类型的消息
    失败：字典，{"code": 21, "action": {}, "message": "AI服务暂不可用"}
"""
def process_ai_response(ai_response):
    try:
        response_data = json.loads(ai_response)  # 解析JSON，字典
        code = response_data.get('code', 21)  # 默认code为21，表示仅对话
        if code == 20: # 设备要操作和对话
            action = response_data.get('action', {})  # 提取action字段
            action = validate_ai_actions(action)  # 白名单过滤
        else:
            action = {}
        message = response_data.get('message', '空') # 提取message字段

        if action: # 有设备要操作
            # 返回结构化字典
            return {
                "code": 20,
                "action": action,
                "message": message
            }
        else:
            return {"code": 21, "action": {}, "message": message}
    except Exception as e:
        print(f"process_ai_response 处理AI回复的消息异常: {str(e)}")
        return {"code": 21, "action": {}, "message": "AI服务暂不可用"}

"""
功能：
    发送消息给所有的客户端
参数：
    ai_response_message_dict: 字典，形式 {"code": 21, "action": {}, "message": "消息"}
返回值：
    void
"""
def send_message_to_clients(client_socket, ai_response_message_dict, client_socket_list):
    # 构造原始消息字符串
    action_str = ", ".join([f"{device}:{cmd}" for device,cmd in ai_response_message_dict['action'].items()])
    original_message = f"action = {action_str if action_str else '0'}; message = {ai_response_message_dict['message']}\r\n"

    # 构造过滤消息字符串
    filtered_message = f"message = {ai_response_message_dict['message']}\r\n"

    for client_socket_i in client_socket_list:
        try:
            if client_socket_i == client_socket:  # 当前客户端需要过滤
                # 根据code判断是否需要过滤
                if ai_response_message_dict['code'] in (20, 21):
                    client_socket_i.send(filtered_message.encode('utf-8'))
                else:
                    client_socket_i.send(original_message.encode('utf-8'))
            else:  # 其他客户端发送完整信息
                client_socket_i.send(original_message.encode('utf-8'))
        except BrokenPipeError:
            close_client_socket(client_socket_i, client_socket_list, client_socket_lock)
        except Exception as e:
            print(f"发送失败: {str(e)}")

"""
功能：
    初始化默认对话记录
参数：
返回值：
    AI初始化消息列表
"""
def init_messages(ai_order):
    return [
        {"role": "system", "content": ai_order},
        {"role": "user", "content": "你好呀！"},
        # {"role": "assistant", "content": "我......"},
    ]

"""
功能：
    加载历史对话记录
参数：
返回值：
    AI初始化消息列表
"""
def load_history_messages(ai_order, history_file_lock):
    # 加锁，防止多线程同时读写文件导致数据损坏
    with history_file_lock:
        try:
            with open('history.json', 'r', encoding='utf-8') as f:
                history_messages = json.load(f)
                if history_messages and history_messages[0]['role'] == 'system': # 检查历史对话记录和系统消息是否存在
                    return history_messages
                else: # 历史记录文件损坏
                    # 保存AI初始化消息到历史记录文件
                    json.dump(init_messages(ai_order), f, ensure_ascii=False, indent=4)
                    return init_messages(ai_order)
        # 历史记录文件不存在
        except (FileNotFoundError, json.JSONDecodeError, KeyError, IndexError):
            with open('history.json', 'w', encoding='utf-8') as f:
                json.dump(init_messages(ai_order), f, ensure_ascii=False, indent=4)
            return init_messages(ai_order)

"""
功能：
    将字典型的json字符串格式的消息保存到历史对话记录文件
参数：
    字典型的json字符串
返回值：
    void
"""
def save_messages_to_history(new_message, history_file_lock):
    try:
        message_dict = json.loads(new_message) # 解析json字符串

        if not isinstance(message_dict, dict): # 验证字典
            return

        # 加锁，防止多线程同时读写文件导致数据损坏
        with history_file_lock:
            # 读取现有历史数据
            try:
                with open('history.json', 'r', encoding='utf-8') as f:
                    history_messages_list = json.load(f)
                    if not isinstance(history_messages_list, list):  # 验证列表
                        history_messages_list = [] # 格式错误，丢掉历史对话记录
            except (FileNotFoundError, json.JSONDecodeError):
                history_messages_list = []

            # 添加新记录到列表末尾
            history_messages_list.append(message_dict)

            # 写入完整列表
            with open('history.json', 'w', encoding='utf-8') as f:
                json.dump(history_messages_list, f, ensure_ascii=False, indent=4)

    except json.JSONDecodeError:
        print("save_messages_to_history 错误：输入的不是有效的JSON字符串")
    except Exception as e:
        print(f"save_messages_to_history 消息保存到历史对话记录文件失败：{str(e)}")

"""
功能：
    关闭客户端套接字
参数：
返回值：
    void
"""
def close_client_socket(client_socket, client_socket_list, client_socket_lock):
    try:
        client_socket.close()

        # 加锁保护列表操作，自动加锁解锁
        with client_socket_lock:
            if client_socket in client_socket_list:
                client_socket_list.remove(client_socket)
    except ValueError as e:
        print(f"套接字不在列表中: {e}")
    except Exception as e:
        print(f"关闭连接时发生错误: {e}")

"""
功能：
    处理客户端线程
参数：
返回值：
    void
"""
def pthread_handle_client_connect(client_socket, client_socket_list):
    try:
        try:
            # 5、使用recv()/send()方法接收/发送数据
            response_messages = get_ai_response(client, messages, model, stream) # 获取 AI大模型 的回复
            print("本AI大人：", json.loads(response_messages))

            messages.append({"role": "assistant", "content": response_messages}) # 添加回复的消息到历史对话记录
            response_message_temp = {"role": "assistant", "content": json.loads(response_messages).get('message', "")} # 封装格式
            save_messages_to_history(json.dumps(response_message_temp), history_file_lock) # 保存AI消息到历史记录文件

            response_messages = process_ai_response(response_messages) # 处理AI的回复
            send_message_to_clients(client_socket, response_messages, client_socket_list) # 给客户端打个招呼
        except Exception:
            pass

        # 移动终端以“\r\n”结束接收，且过滤传感器数据时，多余的字符会去除
        # client_socket.send("message = 温度=32 湿度=21 烟雾=55 一氧化碳=66 光照=88\n".encode('utf-8'))
        test_data = {"code": 21, "action": {}, "message": " "}
        test_data['message'] = "温度=32 湿度=21 烟雾=55 一氧化碳=66 光照=88"
        send_message_to_clients(client_socket, test_data, client_socket_list)

        client_ip, client_port = client_socket.getpeername()
        while True:
            try:
                user_messages = client_socket.recv(1024) # 等待用户消息

                # 客户端主动断开连接
                if not user_messages:
                    print(f"客户端 IP: {client_ip}, 端口: {client_port} 已断开连接！")
                    break

                print(f"用户 IP: {client_ip}, 端口: {client_port} 的消息：{user_messages.decode('utf-8')}")
                messages.append({"role": "user", "content": user_messages.decode("utf-8")}) # 添加用户的消息到历史对话记录
                user_message_temp = {"role": "user", "content": user_messages.decode('utf-8')}  # 封装格式
                save_messages_to_history(json.dumps(user_message_temp), history_file_lock) # 保存用户消息到历史记录文件

                response_messages = get_ai_response(client, messages, model, stream) # 获取 AI大模型 的回复
                try:
                    print("本AI大人：", json.loads(response_messages))

                    messages.append({"role": "assistant", "content": response_messages}) # 添加回复的消息到历史对话记录
                    response_message_temp = {"role": "assistant", "content": json.loads(response_messages).get('message', "无")} # 封装格式
                    save_messages_to_history(json.dumps(response_message_temp), history_file_lock) # 保存AI消息到历史记录文件

                    response_messages = process_ai_response(response_messages) # 处理AI的回复
                    send_message_to_clients(client_socket, response_messages, client_socket_list) # 发给客户端
                except json.JSONDecodeError:
                    print("AI返回了无效的JSON:", json.loads(response_messages))

                # 通过聊天断开连接
                user_messages = user_messages.decode("utf-8").lower()
                exit_keywords = ["再见", "回聊", "拜", "bye", "退出", "quit", "exit"]
                if any(keyword in user_messages for keyword in exit_keywords):
                    print(f"客户端 IP: {client_ip}, 端口: {client_port} 已断开连接！")
                    break
            # 连接异常断开
            except ConnectionResetError:
                print(f"客户端 IP: {client_ip}, 端口: {client_port} 异常断开！")
                break
    finally:
        # 6、使用close()关闭套接字
        close_client_socket(client_socket, client_socket_list, client_socket_lock)







"""
如果使用抖音旗下的大模型，请添加环境变量 export DOUYIN_API_KEY="8512b578-dbb0-48a1-a3df-e26cf6f35ce0"
"""

# 设备指令白名单库
white_list = {
    '开灯':'led_up',
    '关灯':'led_down',
    '开空调':'ac_on',
    '关空调':'ac_off',
    '调节风速':'fan_speed',
    '打开报警器':'buzzer_up',
    '关闭报警器':'buzzer_off',
}
white_list_str = ",".join([f"{device}:{cmd}" for device, cmd in white_list.items()])

# AI 提示词指令
ai_order = """\
【角色设定】\
你是一个智能家居控制中枢，叫大白，不仅能控制家居设备，更能能陪主人聊天，是主人最好的朋友。\
聊天时，自然语言回复会带上各种表情包来体现你的面部表情，且能够理解主人对环境状态的描述，主动\
控制对应设备。请根据对话内容判断是否需要触发硬件操作。\
【任务规则】\
1、你必须且只能使用以下JSON格式响应，禁止其他形式：\
{\
    "code": 20,  # 必填！状态码(20: 设备操作和对话 21: 仅对话)\
    "action": {"[设备名]": "[操作指令]"},  # 键值对，空对象表示无操作\
    "message": ""  # 自然语言回复，带环境描述的拟人化回复\
}\
2、当用户描述环境状态变化时（如光线变暗、温度变化等），立即检索对应的可控设备\
3、设备名称需与可控设备名称严格一致\
【设备映射表】\
提示：':'前面是设备名，后面是操作指令,设备指令用','隔开了\
%s\
【示例】\
{\
    "code": 20,\
    "action": {"检测报警器": "buzzer_up"},\
    "message": "报警器工作正常咯"\
}\
""" % white_list_str

client_socket_list = [] #列表，保存已连接的客户端套接字
client_socket_lock = threading.Lock()  # 创建线程锁，防止套接字列表被同时操作
history_file_lock = threading.Lock()   # 创建线程锁，防止多线程同时读写文件导致数据损坏

# DeepSeek大模型
# api_key="sk-cc800fa4ad5e42ad89235a37786a1eb3"
# base_url="https://api.deepseek.com"
# model = "deepseek-chat" # 指定用哪个模型

# 抖音旗下大模型
api_key=os.environ.get("DOUYIN_API_KEY") # 从环境变量中读取您的方舟API Key
base_url="https://ark.cn-beijing.volces.com/api/v3"
model="doubao-1.5-pro-256k-250115"

messages = load_history_messages(ai_order, history_file_lock)
stream = False # True：流式输出 默认是False：非流式输出
client = OpenAI(api_key = api_key, base_url = base_url)

if __name__ == '__main__':
    try:
        # 1、使用socket类创建套接字对象
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM) #AF_INET 用于网络之间的进程通信 SOCK_STREAM 表示用TCP协议编程

        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) # 允许端口复用

        # 2、使用bind((ip, port))方法绑定IP地址和端口号
        ip = "172.24.145.220"
        port = 8086
        server_socket.bind((ip, port))

        # 3、使用listen()方法监听套接字
        server_socket.listen(5)
        print("服务器已启动！")
        print("我的端口号：", port)
        print("Wait for connection......")

        while True:
            # 4、使用accept()方法等待客户端的连接，其结果是元组类型
            client_socket, client_address = server_socket.accept()
            with client_socket_lock:
                client_socket_list.append(client_socket) # 添加到客户端套接字列表
            print(f"连接客户端 IP: {client_address[0]}, 端口: {client_address[1]} 成功！")
            # 为每个客户端分配一个线程
            client_thread = threading.Thread(target=pthread_handle_client_connect, args=(client_socket, client_socket_list,))
            client_thread.start()
    finally:
        # 6、使用close()关闭套接字
        server_socket.close()



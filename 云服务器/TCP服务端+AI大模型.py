from openai import OpenAI
import socket
import os
import json
import threading

"""
功能：
    获取 AI大模型 回复的消息
参数：

返回值：
    返回AI回复的消息，JSON格式字符串
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

        messages.append({"role": "assistant", "content": response_messages}) # 添加回复的消息到历史对话记录

        return response_messages
    except Exception as e:
        print(f"API 请求错误：{str(e)}")

        return ""

"""
功能：
    验证并过滤AI的设备操作指令
参数：
    原始指令字典
返回值：
    过滤后的安全指令，字典
"""
def validate_ai_actions(actions):
    safe_actions = {}

    for device, cmd in actions.items():
        clean_cmd = cmd.lower() # 统一转换为小写

        if clean_cmd in white_list.values():
            safe_actions[device] = clean_cmd
        else:
            print(f"非法指令拦截: {cmd}")

    return safe_actions

"""
功能：
    处理AI回复的消息
参数：
    response: AI返回的原始JSON字符串
返回值：
    给STM32的安全指令，字符串
"""
def process_ai_response(ai_response):
    try:
        response_data = json.loads(ai_response) # 解析JSON

        actions = response_data.get('action', {}) # 提取action字段

        safe_actions = validate_ai_actions(actions) # 白名单过滤

        message = response_data.get('message', '执行成功')

        if safe_actions:
            # action_str = ", ".join([f"{device}:{cmd}" for device, cmd in safe_actions.items()])
            action_str = ", ".join([f"{cmd}" for device, cmd in safe_actions.items()])
            return f"action = {action_str}; message = {message}\r\n"
        else:
            return f"action = 0; message = {message}\r\n"

    except Exception as e:
        print(f"处理异常: {str(e)}")
        return "action = 0; message = 系统服务暂不可用\r\n"

"""
功能：
    初始化默认对话记录
参数：
返回值：
    消息列表
"""
def init_messages():
    return [
        {"role": "system", "content": ai_order},
        {"role": "user", "content": "你好，我叫轻歌！"},
        # {"role": "assistant", "content": "我......"},
    ]

"""
功能：
    加载历史对话记录
参数：
返回值：
"""
def load_history_messages():
    try:
        with open('history.json', 'r', encoding='utf-8') as f:
            history_messages = json.load(f)
            if history_messages and history_messages[0]['role'] == 'system': # 检查历史对话记录和系统消息是否存在
                return history_messages
            else:
                temp_messages = init_messages()
                for message in temp_messages:
                    save_messages_to_history(json.dumps(message)) # 保存AI初始化消息到历史记录文件
                return init_messages()
    except (FileNotFoundError, json.JSONDecodeError, KeyError, IndexError):
        temp_messages = init_messages()
        for message in temp_messages:
            save_messages_to_history(json.dumps(message))
        return init_messages()

"""
功能：
    将json字符串格式的消息保存历史对话记录
参数：
    字典型的json字符串
返回值：
"""
def save_messages_to_history(new_message):
    try:
        message_dict = json.loads(new_message) # 将json字符串解析为字典

        # 读取现有历史数据
        try:
            with open('history.json', 'r', encoding='utf-8') as f:
                history_messages_list = json.load(f)
                if not isinstance(history_messages_list, list):  # 确保是列表格式
                    history_messages_list = [] # 格式错误，丢掉历史对话记录
        except (FileNotFoundError, json.JSONDecodeError):
            history_messages_list = []

        # 验证字典结构
        if not isinstance(message_dict, dict):
            raise ValueError("输入必须是可以转换为字典的JSON字符串") # 异常

        # 添加新记录到列表末尾
        history_messages_list.append(message_dict)

        # 写入完整列表
        with open('history.json', 'w', encoding='utf-8') as f:
            json.dump(history_messages_list, f, ensure_ascii=False, indent=4)

    except json.JSONDecodeError:
        print("错误：输入的不是有效的JSON字符串")
    except Exception as e:
        print(f"保存失败：{str(e)}")

"""
功能：
    处理客户端线程
参数：

返回值：
"""
def pthread_handle_client_connect(client_socket):
    try:
        # 5、使用recv()/send()方法接收/发送数据
        response_messages = get_ai_response(client, messages, model, stream) # 获取 AI大模型 的回复
        if not response_messages:
            print("AI未返回有效响应")
            response_messages = '{"code":21, "action":{}, "message":"服务异常，请重试"}'
        print("本AI大人：", json.loads(response_messages))
        response_message_temp = {"role": "assistant", "content": json.loads(response_messages).get('message', "")} # 封装格式
        save_messages_to_history(json.dumps(response_message_temp)) # 保存AI消息到历史记录文件
        ai_message = process_ai_response(response_messages) # 处理AI的回复
        client_socket.send(ai_message.encode("utf-8")) # 给客户端打个招呼

        client_ip, client_port = client_socket.getpeername()
        while True:
            try:
                user_messages = client_socket.recv(1024) # 等待用户消息

                if not user_messages:
                    print(f"客户端 IP: {client_ip}, 端口: {client_port} 已断开连接！")
                    break

                print(f"用户 IP: {client_ip}, 端口: {client_port} 的消息：{user_messages.decode('utf-8')}")
                user_message_temp = {"role": "user", "content": user_messages.decode('utf-8')}  # 封装格式
                save_messages_to_history(json.dumps(user_message_temp)) # 保存用户消息到历史记录文件

                messages.append({"role": "user", "content": user_messages.decode("utf-8")}) # 添加用户的消息到历史对话记录
                response_messages = get_ai_response(client, messages, model, stream) # 获取 AI大模型 的回复
                if not response_messages:
                    print("AI未返回有效响应")
                    response_messages = '{"code":21, "action":{}, "message":"服务异常，请重试"}'
                try:
                    print("本AI大人：", json.loads(response_messages))
                    response_message_temp = {"role": "assistant", "content": json.loads(response_messages).get('message', "")} # 封装格式
                    save_messages_to_history(json.dumps(response_message_temp)) # 保存AI消息到历史记录文件
                    ai_message = process_ai_response(response_messages) # 处理AI的回复
                    client_socket.send(ai_message.encode("utf-8")) # 发给客户端
                except json.JSONDecodeError:
                    print("AI返回了无效的JSON:", response_messages)
                    response_messages = '{"code":21, "action":{}, "message":"服务异常，请重试"}'

                temp_messages = user_messages.decode("utf-8").lower()
                exit_keywords = ["再见", "回聊", "拜", "bye", "退出", "quit", "exit"]
                if any(keyword in temp_messages for keyword in exit_keywords):
                    break
            except ConnectionResetError:
                print(f"客户端 IP: {client_ip}, 端口: {client_port} 异常断开！")
                break
    finally:
        # 6、使用close()关闭套接字
        client_socket.close()





"""
AF_INET 用于网络之间的进程通信
SOCK_STREAM 表示用TCP协议编程
如果使用抖音旗下的大模型，请添加环境变量 export DOUYIN_API_KEY="8512b578-dbb0-48a1-a3df-e26cf6f35ce0"
"""
# DeepSeek大模型
# api_key="sk-cc800fa4ad5e42ad89235a37786a1eb3"
# base_url="https://api.deepseek.com"
# model = "deepseek-chat" # 指定用哪个模型

# 抖音旗下大模型
api_key=os.environ.get("DOUYIN_API_KEY") # 从环境变量中读取您的方舟API Key
base_url="https://ark.cn-beijing.volces.com/api/v3"
model="doubao-1.5-pro-256k-250115"

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
ai_order = """
        【角色设定】
        你是一个智能家居控制中枢，叫大白，自然语言回复会带上各种表情包来体现你的面部表情，且
        能够理解用户的环境状态描述并主动控制对应设备。请根据对话内容判断是否需要触发硬件操作。
        你。
        【任务规则】
        1、你必须且只能使用以下JSON格式响应，禁止其他形式：
        {
            "code": 20,  # 必填！状态码(20: 设备操作和对话 21: 仅对话)
            "action": {"[设备名]": "[操作指令]"},  # 键值对，空对象表示无操作
            "message": ""  # 自然语言回复，带环境描述的拟人化回复
        }
        2、当用户描述环境状态变化时（如光线变暗、温度变化等），立即检索对应的可控设备
        3、设备名称需与可控设备名称严格一致
        【设备映射表】
        提示：':'前面是设备名，后面是操作指令,设备指令用','隔开了
        %s
        【示例】
        {
            "code": 20,
            "action": {"检测报警器": "buzzer_up"},
            "message": "报警器工作正常咯"
        }
        """ % white_list_str

messages = load_history_messages()
stream = False # True：流式输出 默认是False：非流式输出
client = OpenAI(api_key = api_key, base_url = base_url)

if __name__ == '__main__':
    try:
        # 1、使用socket类创建套接字对象
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

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
            print(f"连接客户端 IP: {client_address[0]}, 端口: {client_address[1]} 成功！")
            # 为每个客户端分配一个线程
            client_thread = threading.Thread(target=pthread_handle_client_connect, args=(client_socket,))
            client_thread.start()
    finally:
        # 6、使用close()关闭套接字
        server_socket.close()



from openai import OpenAI
import socket
import threading
import json
import os
from enum import Enum
# 自己封装的包
import AI_manager
import client_manager

"""
如果使用抖音旗下的大模型，请添加环境变量 export DOUYIN_API_KEY="8512b578-dbb0-48a1-a3df-e26cf6f35ce0"
"""
class MESSAGE_TYPE(Enum):
    NORMAL = 21 # 普通消息
    SENSOR = 22 # 传感器消息
    DEVICE = 23 # 设备控制消息
    IGNORE = 24 # 忽略消息

# 设备指令白名单库
device_white_list = {
    '开灯':'led_up',
    '关灯':'led_down',
    '开空调':'ac_on',
    '关空调':'ac_off',
    '调节风速':'fan_speed',
    '打开报警器':'buzzer_up',
    '关闭报警器':'buzzer_off',
    '开窗':'window_up',
    '关窗':'window_off',
}

# 传感器阈值
sensor_value_cutoff = {
    '温度':38.6,
    '湿度':66.2,
    '烟雾':55,
    '一氧化碳':20,
    '光照':3.0,
}

# AI 提示词指令
ai_order = f"""\
【角色设定】
你是一个智能家居控制中枢，叫大白，不仅能控制家居设备，更能能陪主人聊天，是主人最好的朋友。\
聊天时，自然语言回复会带上各种表情包来体现你的面部表情，且能够理解主人对环境状态的描述，主动\
控制对应设备。请根据对话内容判断是否需要触发硬件操作。
【任务规则】
1、你必须且只能使用以下JSON格式响应，禁止其他形式：
{{
    "code": 21,  # 必填！状态码(21: 仅对话；23: 设备操作和对话；24: 告诉别人忽略你本次的回答)
    "action": {{"[设备名]": "[操作指令]"}},  # 键值对，设备之间用逗号隔开，当"action"的值为空字典，表示无设备操作
    "message": ""  # 自然语言回复，带环境描述的拟人化回复
}}
2、当用户描述环境状态变化时（如光线变暗、温度变化等），立即检索对应的可控设备。
3、设备名称需与可控设备名称严格一致。
4、当我发送传感器相关的数据给你，例如：\"温度=32 湿度=21 烟雾=55 一氧化碳=66 光照=88\r\n\"，你判断\
对应的传感器模块数据是否超过阈值，超过了，告诉我哪个超过了，警告我有什么潜在问题或者危险。
【设备映射表】
提示：':'前面是设备名，后面是操作指令，设备指令用','隔开了
{json.dumps(device_white_list, ensure_ascii=False, indent=4)}
示例：
我说：检测一下报警器是否正常？
你的json格式字符串回答：
{{
    "code": 23,
    "action": {{"打开报警器": "buzzer_up"}},
    "message": "已检测报警器，报警器工作正常咯"
}}
【传感器模块阈值表】
提示：':'前面是传感器模块，后面是阈值，传感器模块用','隔开了
{json.dumps(sensor_value_cutoff, ensure_ascii=False, indent=4)}
示例：
我说：温度=32 湿度=21 烟雾=55 一氧化碳=66 光照=88\r\n
有传感器阈值超标的情况下，你的json格式字符串回答：
{{
    "code": 23,
    "action": {{"开窗":"window_up", "打开报警器": "buzzer_up"}},
    "message": "警告，一氧化碳浓度过高，有中毒风险！我已为你打开窗户通风"
}}
否则：
{{
    "code": 24,
    "action": {{}},
    "message": "家居环境良好！"
}}
"""

# DeepSeek大模型
# api_key="sk-cc800fa4ad5e42ad89235a37786a1eb3"
# base_url="https://api.deepseek.com"
# model = "deepseek-chat" # 指定用哪个模型

# 抖音旗下大模型
api_key=os.environ.get("DOUYIN_API_KEY") # 从环境变量中读取您的方舟API Key
base_url="https://ark.cn-beijing.volces.com/api/v3"
model="doubao-1.5-pro-256k-250115"

client = OpenAI(api_key = api_key, base_url = base_url)
stream = False # True：流式输出 默认是False：非流式输出

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
def pthread_handle_client_connect(client_socket, client_mgr, AI_mgr):
    try:
        client_socket.send("请告诉我你的设备 id (数字)\r\n".encode('utf-8'))
        while True:
            # 获取客户端设备id
            device_id = client_socket.recv(1024)
            try:
                if not isinstance(int(device_id.strip()), int): # id 不是数字
                    continue
                else:
                    break
            except Exception:
                client_socket.send("输入错误，请重新告诉我你的设备 id (数字)\r\n".encode('utf-8'))
                pass

        # 更新客户端设备id
        client_mgr.update_device_id(client_socket, int(device_id.strip()))

        # 生成唯一客户端标识
        device_id = int(device_id.strip())
        history_filename = f"history_{device_id}.json"  # 唯一历史文件名

        # 加载该客户端的历史记录
        message_to_ai = AI_mgr.load_history_message(ai_order, history_filename) # 传入动态文件名

        try:
            # AI初始问候
            ai_response_message = AI_mgr.get_response(message_to_ai, MESSAGE_TYPE) # 获取 AI大模型 的回复
            print("本AI大人：", json.loads(ai_response_message))

            # 更新AI消息到历史记录
            message_to_ai.append({"role": "assistant", "content": ai_response_message}) # 添加回复的消息到历史对话记录
            message_temp = {"role": "assistant", "content": json.loads(ai_response_message).get('message', "")} # 封装格式
            AI_mgr.save_message_to_history(json.dumps(message_temp), history_filename) # 保存AI消息到历史记录文件

            # 处理AI的回复
            processed_ai_message = AI_mgr.handle_response(ai_response_message, MESSAGE_TYPE)
            # 5、使用recv()/send()方法接收/发送数据
            client_mgr.send_message_to_client(client_socket, processed_ai_message, MESSAGE_TYPE) # 给客户端打个招呼
        except Exception:
            pass

        client_ip, client_port = client_socket.getpeername()
        while True:
            try:
                # 等待用户消息
                user_message = client_socket.recv(1024)

                # 用户客户端主动断开连接
                if not user_message:
                    print(f"客户端 IP: {client_ip}, 端口: {client_port} 已断开连接！")
                    break

                print(f"用户 IP: {client_ip}, 端口: {client_port} 的消息：{user_message.decode('utf-8')}")

                # 如果是改变客户端设备类型的消息，{"device_type": "ESP01S"}，拦截并处理客户端消息
                try:
                    user_message_dict = json.loads(user_message.decode('utf-8'))
                    if isinstance(user_message_dict, dict): # 用户消息是字典
                        client_mgr.update_client_dev_type(client_socket, user_message_dict.get("device_type", "PHONE"))
                        client_mgr.send_message_to_client(client_socket, client_mgr.make_send_message_package("设备类型更换成功！", MESSAGE_TYPE), MESSAGE_TYPE)
                        continue
                except Exception:
                    pass

                """
                如果是ESP01S汇报的传感器消息，即 "温度=32 湿度=21 烟雾=55 一氧化碳=66 光照=88\r\n"，
                刷新移动终端传感器数据
                """
                if client_mgr.get_client_device_type(client_socket) == client_mgr.client_device_type_list[1]: # ESP01S客户端套接字
                    sensor_keywords = [str(key) for key in sensor_value_cutoff]
                    if any(keyword in user_message.decode('utf-8') for keyword in sensor_keywords):
                        client_mgr.send_message_to_all_phone_clients(client_socket, client_mgr.make_send_message_package(user_message.decode('utf-8'), MESSAGE_TYPE), MESSAGE_TYPE)
                        print("更新移动终端传感器数据成功")

                # 更新用户消息到历史记录
                message_to_ai.append({"role": "user", "content": user_message.decode("utf-8")}) # 添加用户的消息到历史对话记录
                message_temp = {"role": "user", "content": user_message.decode('utf-8')}  # 封装格式
                AI_mgr.save_message_to_history(json.dumps(message_temp), history_filename) # 保存用户消息到历史记录文件

                # 获取 AI大模型 的回复
                ai_response_message = AI_mgr.get_response(message_to_ai, MESSAGE_TYPE)
                try:
                    print("本AI大人：", json.loads(ai_response_message))

                    # 更新AI消息到历史记录
                    message_to_ai.append({"role": "assistant", "content": ai_response_message}) # 添加回复的消息到历史对话记录
                    message_temp = {"role": "assistant", "content": json.loads(ai_response_message).get('message', "")} # 封装格式
                    AI_mgr.save_message_to_history(json.dumps(message_temp), history_filename) # 保存AI消息到历史记录文件

                    # 将AI消息发给客户端
                    processed_ai_message = AI_mgr.handle_response(ai_response_message, MESSAGE_TYPE) # 处理AI的回复
                    client_mgr.send_message_to_client(client_socket, processed_ai_message, MESSAGE_TYPE)

                    """
                    如果是由ESP01S汇报的传感器消息出发的AI警报提醒，
                    通知所有的移动终端
                    """
                    if client_mgr.get_client_device_type(client_socket) == client_mgr.client_device_type_list[1]: # ESP01S客户端套接字
                        if processed_ai_message["code"] != MESSAGE_TYPE.IGNORE.value: # AI的警告消息
                            client_mgr.send_message_to_all_phone_clients(client_socket, processed_ai_message, MESSAGE_TYPE)
                            print("已将警报消息通知所有的移动终端")
                except json.JSONDecodeError:
                    print("AI返回了无效的JSON:", json.loads(ai_response_message))

                # 通过聊天断开连接
                user_message = user_message.decode("utf-8").lower()
                exit_keywords = ["再见", "回聊", "拜", "bye", "退出", "quit", "exit"]
                if any(keyword in user_message for keyword in exit_keywords):
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
    try:
        # 初始化实例
        client_mgr = client_manager.client_manager() # 客户端管理器实例
        AI_mgr = AI_manager.AI_manager(client, model, stream, device_white_list) # AI管理器实例

        # print("AI提示词：") # 调试用
        # print(ai_order)

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
            try:
                # 4、使用accept()方法等待客户端的连接，其结果是元组类型
                client_socket, client_address = server_socket.accept()
                print(f"连接客户端 IP: {client_address[0]}, 端口: {client_address[1]} 成功！")

                # 添加到客户端套接字列表
                client_mgr.add_client(client_socket, client_address, client_address[0], "PHONE") # 默认客户端设备 id 是ip

                # 为每个客户端分配一个线程
                threading.Thread(target=pthread_handle_client_connect, args=(client_socket, client_mgr, AI_mgr,)).start()
            except Exception:
                pass
    finally:
        # 6、使用close()关闭套接字
        server_socket.close()
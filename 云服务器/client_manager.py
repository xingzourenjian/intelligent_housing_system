import threading
import time
import json

# 客户端管理器
class client_manager:
    def __init__(self):
        self.clients = {}  # 结构: {client_socket: {"dev_type": ...}, }
        self.clients_lock = threading.Lock()  # 创建线程锁，防止套接字字典被同时操作
        self.client_device_type_list = ["PHONE", "ESP01S"]

    """
    功能：
        添加新客户端
    参数：
        client_socket：客户端套接字
        client_address：客户端地址列表 ip port
        client_device_id：客户端id
        client_dev_type：客户端设备类型，字符串
    返回值：
        void
    """
    def add_client(self, client_socket, client_address, device_id, client_dev_type="PHONE"):
        client_ip, client_port = client_socket.getpeername()
        with self.clients_lock:
            self.clients[client_socket] = {
                "dev_type":client_dev_type,
                "ip":client_address[0],
                "port":client_address[1],
                "device_id":device_id,
                "connect_time": time.time(), # 当前时间
            }

    """
    功能：
        移除客户端
    参数：
        client_socket：客户端套接字
    返回值：
        void
    """
    def close_client(self, client_socket):
        client_socket.close()
        with self.clients_lock:
            if client_socket in self.clients: # 遍历键
                del self.clients[client_socket]

    """
    功能：
        更新客户端设备类型
    参数：
        client_socket：客户端套接字
        new_dev_type：设备类型
    返回值：
        void
    """
    def update_client_dev_type(self, client_socket, new_dev_type):
        if new_dev_type not in self.client_device_type_list:
            return
        with self.clients_lock:
            if client_socket in self.clients: # 遍历键
                self.clients[client_socket]["dev_type"] = new_dev_type

    """
    功能：
        更新客户端设备id
    参数：
        client_socket：客户端套接字
        new_id：客户端设备id
    返回值：
        void
    """
    def update_device_id(self, client_socket, new_id):
        with self.clients_lock:  # 加锁保证线程安全
            if client_socket in self.clients:
                self.clients[client_socket]["device_id"] = new_id

    """
    功能：
        获取客户端设备类型
    参数：
        client_socket：客户端套接字
    返回值：
        void
    """
    def get_client_device_type(self, client_socket):
        with self.clients_lock:
            return self.clients.get(client_socket, {}).get("dev_type")

    """
    功能：
        获取所有客户端信息
    参数：
        void
    返回值：
        返回json格式的列表
    """
    def get_all_clients_massage(self):
        with self.clients_lock:
            all_clients_massage = [json.dumps({k: v}) for k, v in self.clients.items()] # 字典生成式
        return all_clients_massage

    """
    功能：
        封装要发送给客户端的消息
    参数：
        send_message: 字符串
        MESSAGE_TYPE：消息类型，枚举体
    返回值：
        要发送给客户端的消息，字典
    """
    def make_send_message_package(self, send_message, MESSAGE_TYPE):
        temp_message = {"code": MESSAGE_TYPE.NORMAL.value, "action": {}, "message": " "}
        temp_message['message'] = send_message
        return temp_message

    """
    功能：
        发送消息给客户端
    参数：
        client_socket：客户端套接字
        send_message: {"code": 21, "action": {}, "message": "消息"}，字典
        MESSAGE_TYPE：消息类型，枚举体
    返回值：
        void
    """
    def send_message_to_client(self, client_socket, send_message, MESSAGE_TYPE):
        # 移动终端以“\r\n”结束接收
        if isinstance(send_message, str):
            return

        try:
            # 构造原始消息字符串，例："action = 0; message = 你好\r\n"
            action_str = ", ".join(f"{device}:{cmd}" for device,cmd in send_message['action'].items())
            original_message = f"action = {action_str if action_str else '0'}; message = {send_message['message']}\r\n" # 给ESP01S的消息

            # 构造过滤消息字符串
            filtered_message1 = f"message = {send_message['message']}\r\n" # 给移动终端的传感器消息
            filtered_message2 = f"{send_message['message']}\r\n" # 给移动终端的正常消息

            try:
                # 移动终端客户端套接字调用该函数
                if self.get_client_device_type(client_socket) == self.client_device_type_list[0]:
                    if send_message['code'] == MESSAGE_TYPE.SENSOR.value: # 传感器消息
                        client_socket.send(filtered_message1.encode('utf-8'))
                    elif send_message['code'] == MESSAGE_TYPE.NORMAL.value: # 仅聊天
                        client_socket.send(filtered_message2.encode('utf-8'))
                    elif send_message['code'] == MESSAGE_TYPE.DEVICE.value: # 设备操作和聊天
                        # 发给自己
                        client_socket.send(filtered_message2.encode('utf-8'))
                        # 发给ESP01S
                        for client_socket_i in self.clients: # 遍历键
                            if self.get_client_device_type(client_socket_i) == self.client_device_type_list[1]: # 是ESP01S客户端套接字
                                client_socket_i.send(original_message.encode('utf-8'))
                                break
                # ESP01S客户端套接字调用该函数
                elif self.get_client_device_type(client_socket) == self.client_device_type_list[1]:
                    client_socket.send(original_message.encode('utf-8'))
            except BrokenPipeError:
                self.close_client(client_socket)
            except Exception as e:
                print(f"发送失败: {str(e)}")
        except Exception:
            pass

    """
    功能：
        发送消息给所有的移动终端客户端
    参数：
        client_socket：客户端套接字
        send_message: {"code": 21, "action": {}, "message": "消息"}，字典
        MESSAGE_TYPE：消息类型，枚举体
    返回值：
        void
    """
    def send_message_to_all_phone_clients(self, client_socket, send_message, MESSAGE_TYPE):
        for client_socket_i in self.clients: # 遍历键
            if self.get_client_device_type(client_socket_i) != self.client_device_type_list[1]: # 不是ESP01S客户端套接字
                self.send_message_to_client(client_socket_i, send_message, MESSAGE_TYPE)
import threading
import time
import json
import socket
from enum import Enum

class MESSAGE_TYPE(Enum):
    # 枚举体实例 (类属性)
    NORMAL = 21 # 普通消息
    SENSOR = 22 # 传感器消息
    DEVICE = 23 # 设备控制消息
    IGNORE = 24 # 忽略消息

# 客户端管理器
class client_manager:
    def __init__(self) -> None:
        # 实例属性
        self.clients = {}  # 结构: {client_socket: {"dev_type": ...}, }
        self.clients_lock = threading.Lock()  # 创建线程锁，防止套接字字典被同时操作
        self.client_locks = {}  # 创建客户端套接字发送锁字典，防止多个线程向同一个客户端同时发送消息
        self.client_device_type_list = ["PHONE", "ESP01S"]

    """
    功能：
        添加新客户端
    参数：
        client_socket：客户端套接字
        client_address：客户端地址列表 ip port
        client_device_id：客户端id
        client_dev_type：客户端设备类型
    返回值：
        void
    """
    def add_client(self, client_socket: socket.socket, client_address: tuple[str, int], device_id: int, client_dev_type: str = "PHONE") -> None:
        # 实例方法
        client_ip, client_port = client_socket.getpeername()
        with self.clients_lock:
            self.clients[client_socket] = {
                "dev_type":client_dev_type,
                "ip":client_address[0],
                "port":client_address[1],
                "device_id":device_id,
                "connect_time": time.time(), # 当前时间
            }
            self.client_locks[client_socket] = threading.Lock()  # 为每个客户端创建锁
            # 禁用Nagle算法
            client_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

    """
    功能：
        移除客户端
    参数：
        client_socket：客户端套接字
    返回值：
        void
    """
    def close_client(self, client_socket: socket.socket) -> None:
        client_socket.close()
        with self.clients_lock:
            if client_socket in self.clients: # 遍历键，把它从客户端字典中删除
                del self.clients[client_socket]
            if client_socket in self.client_locks: # 把它从客户端锁字典中删除
                del self.client_locks[client_socket]

    """
    功能：
        更新客户端设备类型
    参数：
        client_socket：客户端套接字
        new_dev_type：设备类型
    返回值：
        void
    """
    def update_client_dev_type(self, client_socket: socket.socket, new_dev_type: str) -> None:
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
    def update_device_id(self, client_socket: socket.socket, new_id: int) -> None:
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
    def get_client_device_type(self, client_socket: socket.socket) -> None:
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
    def get_all_clients_massage(self) -> list:
        with self.clients_lock:
            all_clients_massage = [json.dumps({k: v}) for k, v in self.clients.items()] # 字典生成式
        return all_clients_massage

    """
    功能：
        封装要发送给客户端的消息
    参数：
        send_message: 要发送的字符串
        msg_type：消息类型，枚举体实例取值
    返回值：
        要发送给客户端的消息，{"code": 21, "action": {}, "message": "消息"}，字典
    """
    def make_send_message(self, send_message: str, msg_type: MESSAGE_TYPE = MESSAGE_TYPE.NORMAL.value) -> dict:
        return {
            "code": msg_type,
            "action": {},
            "message": send_message
        }

    """
    功能：
        发送消息给客户端 (自动添加\r\n结尾)
    参数：
        client_socket：客户端套接字
        send_message: {"code": 23, "action": {"开窗":"window_up", "打开报警器": "buzzer_up"}, "message": "消息"}，字典
    返回值：
        void
    """
    def send_message_to_client(self, client_socket: socket.socket, send_message: dict) -> None:
        if isinstance(send_message, str):
            return

        # 移动终端以“\r\n”结束接收
        try:
            # 构造原始消息字符串，例：{"code": 21, "action": {}, "message": "消息"}\r\n
            original_message_str = json.dumps(send_message, ensure_ascii=False) # indent=4 是冗余空白符
            original_message = original_message_str + "\r\n"

            try:
                # 移动终端客户端
                if self.get_client_device_type(client_socket) == self.client_device_type_list[0]:
                        # 发给自己
                        with self.client_locks[client_socket]: # 只给移动终端加发送锁
                            client_socket.send(original_message.encode('utf-8'))
                        # 发给ESP01S，如果是设备控制消息
                        if send_message["code"] == MESSAGE_TYPE.DEVICE.value:
                            for client_socket_i in self.clients: # 遍历键
                                if self.get_client_device_type(client_socket_i) == self.client_device_type_list[1]: # 是ESP01S客户端套接字
                                    client_socket_i.send(original_message.encode('utf-8'))
                                    break
                # ESP01S客户端
                elif self.get_client_device_type(client_socket) == self.client_device_type_list[1]:
                    client_socket.send(original_message.encode('utf-8'))
                # 未知设备客户端
                else:
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
    返回值：
        void
    """
    def send_message_to_all_phone_clients(self, client_socket: socket.socket, send_message: dict) -> None:
        for client_socket_i in self.clients: # 遍历键
            if self.get_client_device_type(client_socket_i) != self.client_device_type_list[1]: # 不是ESP01S客户端套接字
                self.send_message_to_client(client_socket_i, send_message)
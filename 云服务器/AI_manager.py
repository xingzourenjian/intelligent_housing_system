from openai import OpenAI
import threading
import json
import os
import time
# 自己封装的包
from client_manager import MESSAGE_TYPE

class AI_manager:
    def __init__(self, client, model, stream, device_white_list: dict, scene_mode_list: dict) -> None:
        # 实例属性
        self.client = client
        self.model = model
        self.stream = stream
        self.device_white_list = device_white_list  # 设备白名单，字典
        self.scene_mode_list = scene_mode_list      # 场景模式名单，字典
        self.get_response_lock = threading.Lock()   # 创建线程锁，防止多线程同时调用AI接口
        self.history_file_lock = threading.Lock()   # 创建线程锁，防止多线程同时读写文件导致数据损坏

        # 预处理允许的指令
        self.allowed_commands = set()
        # 合并设备白名单和场景模式的指令
        all_commands = list(device_white_list.values()) + list(scene_mode_list.values())
        for cmd in all_commands:
            # 提取函数名部分，如"window_adjust(10)" -> "window_adjust"
            func_name = cmd.split('(')[0].strip().lower()
            self.allowed_commands.add(func_name)

    """
    功能：
        将AI初始化消息 初始化为 默认对话记录
    参数：
        ai_order: AI提示词，字符串
    返回值：
        AI初始化消息，字典型列表
    """
    def init_message(self, ai_order: str) -> list:
        # 实例方法
        return [
            {"role": "system", "content": ai_order},
            {"role": "user", "content": "曦曦"},
            # {"role": "assistant", "content": "我......"},
        ]

    """
    功能：
        获取 AI大模型 回复
    参数：
        message：AI初始化消息，字典型列表
    返回值：
        成功：AI回复的消息，字典型的JSON字符串
        失败：{"code": 21, "action": {}, "message": "AI服务暂时不可用"}，字典型的JSON字符串
    """
    def get_response(self, message: list) -> str:
        try:
            response_message = "" # 用于保存回复的消息，字典型的JSON字符串

            with self.get_response_lock:
                # 获取到回复的消息
                ai_response = self.client.chat.completions.create(
                    model = self.model,
                    messages = message,
                    stream = self.stream,
                    max_tokens = 4096, # 模型回复最大长度，单位 token
                    temperature = 1,   # 0~2
                    # response_format = {"type": "json_object"},
                )

            # 保存回复的消息
            if self.stream: # True：流式输出
                for i in ai_response:
                    message_i = i.choices[0].delta.content
                    response_message = response_message + message_i # 每个字都拼接起来，保存回复的消息
            else:
                response_message = ai_response.choices[0].message.content

            return response_message
        except Exception as e:
            print(f"get_response API 请求错误：{str(e)}")
            return json.dumps({"code": {MESSAGE_TYPE.NORMAL.value}, "action": {}, "message": "AI服务暂时不可用"})

    """
    功能：
        验证并过滤AI的设备操作指令
    参数：
        action：AI指令，字典
    返回值：
        成功：过滤后的安全指令，字典
        失败：{}
    """
    def validate_device_actions(self, action: dict) -> dict:
        safe_action = {}

        # 验证字典格式
        if not isinstance(action, dict):
            return safe_action

        # 过滤AI设备操作指令
        for device, cmd in action.items():
            wait_clean_cmd = cmd.lower() # 统一转换为小写
            # 提取函数名部分，如"window_adjust(20)" -> "window_adjust"
            func_name = wait_clean_cmd.split('(')[0].strip()

            # 验证函数名是否在允许的指令集合中
            if func_name in self.allowed_commands:
                safe_action[device] = wait_clean_cmd  # 保留原始指令
            else:
                print(f"validate_device_actions 非法指令拦截: {cmd}")

        return safe_action

    """
    功能：
        处理AI回复的消息，过滤设备指令和封装消息格式
    参数：
        response_message: AI回复的消息，字典型的JSON字符串
    返回值：
        成功：处理后的消息，字典
        失败：{"code": 21, "action": {}, "message": "AI服务暂不可用"}，字典
    """
    def handle_response(self, response_message: str) -> dict:
        try:
            # 解析AI回复的消息
            response_message_str = json.loads(response_message)  # 解析JSON，字典

            # 提取键值对，并处理
            code = response_message_str.get('code', MESSAGE_TYPE.NORMAL.value)  # 默认仅对话
            if code == MESSAGE_TYPE.DEVICE.value: # 设备操作和对话
                action = response_message_str.get('action', {})  # 提取action字段
                action = self.validate_device_actions(action)  # 白名单过滤
            else:
                action = {}
            message = response_message_str.get('message', '') # 提取message字段

            # 返回处理后的消息
            if action: # 设备操作和对话
                return {
                    "code": MESSAGE_TYPE.DEVICE.value,
                    "action": action,
                    "message": message
                }
            else: # 仅对话
                return {"code": MESSAGE_TYPE.NORMAL.value, "action": {}, "message": message}
        except Exception as e:
            print(f"handle_response : {str(e)}")
            return {"code": MESSAGE_TYPE.NORMAL.value, "action": {}, "message": "AI服务暂不可用"}

    """
    功能：
        AI初始化消息，加载历史对话记录
    参数：
        ai_order：AI提示词，字符串
        filename：要加载的文件
    返回值：
        AI初始化消息，字典型列表
    """
    def load_history_message(self, ai_order: str, filename: str) -> list:
        with self.history_file_lock: # 加锁，防止多线程同时读写文件导致数据损坏
            try:
                # 打开历史记录文件
                with open(filename, 'r', encoding='utf-8') as f:
                    # 加载历史记录文件
                    history_message = json.load(f)
                    if history_message and history_message[0]['role'] == 'system': # 检查历史对话记录和系统消息是否存在
                        return history_message
                    else: # 历史记录文件损坏
                        # 保存AI初始化消息到历史记录文件
                        init_message_list = self.init_message(ai_order)
                        with open(filename, 'w', encoding='utf-8') as f:
                            json.dump(init_message_list, f, ensure_ascii=False, indent=4)
                        return init_message_list
            # 历史记录文件不存在
            except (FileNotFoundError, json.JSONDecodeError, KeyError, IndexError):
                init_message_list = self.init_message(ai_order)
                with open(filename, 'w', encoding='utf-8') as f:
                    json.dump(init_message_list, f, ensure_ascii=False, indent=4)
                return init_message_list

    """
    功能：
        将字典型的json字符串格式的消息，保存到历史对话记录文件
    参数：
        new_message：字典型的json字符串
        filename：要加载的文件
    返回值：
        void
    """
    def save_message_to_history(self, new_message: str, filename: str) -> None:
        try:
            message_dict = json.loads(new_message) # 解析json字符串

            if not isinstance(message_dict, dict): # 验证字典
                return

            # 加锁，防止多线程同时读写文件导致数据损坏
            with self.history_file_lock:
                # 读取现有历史数据
                try:
                    with open(filename, 'r', encoding='utf-8') as f:
                        history_message = json.load(f)
                        if not isinstance(history_message, list):  # 验证列表
                            history_message = [] # 格式错误，丢掉历史对话记录
                # 打开文件失败
                except (FileNotFoundError, json.JSONDecodeError):
                    history_message = []

                # 添加新记录到列表末尾
                history_message.append(message_dict)

                # 写入完整列表
                with open(filename, 'w', encoding='utf-8') as f:
                    json.dump(history_message, f, ensure_ascii=False, indent=4)

        except json.JSONDecodeError:
            print("save_message_to_history 错误：输入的不是有效的JSON字符串")
        except Exception as e:
            print(f"save_message_to_history 消息保存到历史对话记录文件失败：{str(e)}")

    """
    功能：
        清理陈旧历史对话记录文件
    参数：
        days：保留天数，默认30天
    返回值：
        已删除的文件数
    """
    def clean_old_histories(self, days: int = 30) -> int:
        delete_count = 0

        for filename in os.listdir(): # 遍历当前目录下的所有文件和文件夹
            try:
                # 添加文件类型校验
                if not (filename.startswith("history_")
                    and filename.endswith(".json")
                    and os.path.isfile(filename)):
                    continue

                # 添加时区统一处理
                file_time = os.path.getmtime(filename) # 获取文件最后修改时间 单位：秒
                cutoff_time = time.time() - days * 86400 # 截止时间 当前时间 - 保留天数对应的秒数

                if file_time < cutoff_time:
                    try:
                        os.remove(filename)
                        print(f"deleted file: {filename}")  # 简单日志
                        delete_count += 1
                    except Exception as e:
                        print(f"Error deleting {filename}: {str(e)}")

            except Exception as e:
                print(f"Error processing {filename}: {str(e)}")

        return delete_count

    """
    功能：
        清除所有历史对话记录文件
    参数：
        void
    返回值：
        已删除的文件数
    """
    def clean_all_histories(self) -> int:
        delete_count = 0

        for filename in os.listdir(): # 遍历当前目录下的所有文件和文件夹
            try:
                # 添加文件类型校验
                if not (filename.startswith("history_")
                    and filename.endswith(".json")
                    and os.path.isfile(filename)):
                    continue

                try:
                    os.remove(filename)
                    print(f"deleted file: {filename}")  # 简单日志
                    delete_count += 1
                except Exception as e:
                    print(f"Error deleting {filename}: {str(e)}")

            except Exception as e:
                print(f"Error processing {filename}: {str(e)}")

        return delete_count
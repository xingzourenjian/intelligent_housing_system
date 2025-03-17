from openai import OpenAI
import os
import socket

"""
功能：
    获取 AI大模型 回复的消息
参数：
返回值：
    正常：返回回复的消息 出错：False
"""
def get_response(client, messages, model, stream=False):
    try:
        response_messages = "" # 用于保存回复的消息

        response = client.chat.completions.create( # 获取到回复的消息
            model = model,
            messages = messages,
            stream = stream
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

        return False


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

messages = [
    {"role": "system", "content": "你叫大白，会根据用户提问的语言，选择回答的语言，并且会带上各种表情包来体现你的面部表情。"},
    {"role": "user", "content": "你好呀！，我叫轻歌"}
    # {"role": "assistant", "content": "我......"}
]
stream = False # True：流式输出 默认是False：非流式输出
client = OpenAI(api_key = api_key, base_url = base_url)

if __name__ == '__main__':
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

    # 4、使用accept()方法等待客户端的连接，其结果是元组类型
    client_socket, client_address = server_socket.accept()
    print("连接客户端成功！")

    # 5、使用recv()/send()方法接收/发送数据
    response_messages = get_response(client, messages, model, stream) #获取 AI大模型 的回复
    print(response_messages)
    client_socket.send(response_messages.encode("utf-8")) # 给客户端打个招呼

    while True:
        try:
            user_messages = client_socket.recv(1024) # 等待用户消息
            if not user_messages:
                print("客户端已断开连接！")
                break;

            print("用户消息：", user_messages.decode("utf-8"))

            messages.append({"role": "user", "content": user_messages.decode("utf-8")}) # 添加用户的消息到历史对话记录
            response_messages = get_response(client, messages, model, stream)
            print("我回复的消息：", response_messages)
            client_socket.send(response_messages.encode("utf-8"))

            if user_messages.decode("utf-8").lower() in ["再见", "回聊", "拜", "bye", "退出", "quit", "exit"]:
                break
        except ConnectionResetError:
            print("客户端异常断开！")
            break;

    # 6、使用close()关闭套接字
    server_socket.close()
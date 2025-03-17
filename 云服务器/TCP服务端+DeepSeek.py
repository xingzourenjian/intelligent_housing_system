from openai import OpenAI
# from socket import socket, AF_INET, SOCK_STREAM
import socket
# AF_INET 用于网络之间的进程通信
# SOCK_STREAM 表示用TCP协议编程

"""
功能：
    获取 DeepSeek 回复的消息
参数：
返回值：
    正常：返回回复的消息 出错：False
"""
def get_response(client, messages, model="deepseek-chat", stream=False):
    try:
        response_messages = "" # 用于保存回复的消息

        response = client.chat.completions.create( # 获取到回复的消息
            model = model,
            messages = messages,
            max_tokens = 1024, # 默认是4096
            temperature = 1.3, # 通用对话
            stream = stream
        )

        if stream: # True：流式输出
            # print("DeepSeek：", end="", flush=True)
            for i in response:
                chunk_message = i.choices[0].delta.content
                response_messages = response_messages + chunk_message # 每个字都拼接起来，保存回复的消息
                # print(chunk_message, end="", flush=True)
            # print() # 换行
        else:
            response_messages = response.choices[0].message.content # 保存回复的消息
            # print(f"DeepSeek：{response.choices[0].message.content}")

        messages.append({"role": "assistant", "content": response_messages}) # 添加回复的消息到历史对话记录（换成"system"无法连续对话）

        return response_messages
    except Exception as e:
        print(f"API 请求错误：{str(e)}")

        return False



client = OpenAI(api_key="sk-cc800fa4ad5e42ad89235a37786a1eb3", base_url="https://api.deepseek.com")
model = "deepseek-chat" # 指定用哪个模型，本次调用为V3模型。如果调用R1模型，会出错，因为它的连续对话代码不是这么写的
messages = [
    {"role": "system", "content": "你的名字叫大白，会根据用户提问的语言，选择回答的语言，且回答问题很简要，并且会带上颜文字来体现你的表情。"},
    {"role": "user", "content": "你好呀！，我叫轻歌"}
]
stream = True # True：流式输出 默认是False：非流式输出

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

    # 5、使用recv()/send()方法接收/发送数据，并使用 DeepSeek回复
    response_messages = get_response(client, messages, model, stream) #获取 DeepSeek回复
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



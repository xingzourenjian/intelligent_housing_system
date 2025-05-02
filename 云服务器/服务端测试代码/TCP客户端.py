import socket

client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
ip = "47.86.228.121"
port = 8086
print("连接服务器中......")
print("我要连接到的端口号：", port)
client_socket.connect((ip, port))
print("连接服务器成功")

server_messages = client_socket.recv(1024)
print(f"服务器消息：{server_messages.decode("utf-8")}")

client_socket.send("桂林全年的气候怎么样，有没有回南天？".encode("utf-8"))

server_messages = client_socket.recv(1024)
print(f"服务器消息：{server_messages.decode("utf-8")}")

client_socket.send("哈尔滨呢？".encode("utf-8"))

server_messages = client_socket.recv(1024)
print(f"服务器消息：{server_messages.decode("utf-8")}")

client_socket.send("回聊".encode("utf-8"))

server_messages = client_socket.recv(1024)
print(f"服务器消息：{server_messages.decode("utf-8")}")

client_socket.close()

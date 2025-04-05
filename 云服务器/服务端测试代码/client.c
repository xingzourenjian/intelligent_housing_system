#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#define IP "47.86.228.121"
#define PORT 8086

#define RECV_BUF_SIZE 1024
int main(int argc, char const *argv[])
{
    char send_buf[RECV_BUF_SIZE] = {0};
    char recv_buf[RECV_BUF_SIZE] = {0};

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        perror("error socket");
        return -1;
    }

    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)); //允许新套接字重用端口

    printf("正在连接服务端...\n");

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    inet_aton(IP, &server_addr.sin_addr);
    server_addr.sin_port = htons(PORT);
    if(-1 == connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)))
    {
        perror("error connect");
        return -1;
    }

    printf("连接服务端 %s 成功\n", inet_ntoa(server_addr.sin_addr));

    memset(recv_buf, 0, strlen(recv_buf));
    recv(sockfd, recv_buf, RECV_BUF_SIZE, 0);
    printf("服务端回复： %s\n", recv_buf);

    while(1)
    {
        printf("请输入你要发送到服务端的数据: ");
        scanf("%s", send_buf);
        if(strcmp(send_buf, "exit") == 0)
        {
            break;
        }

        send(sockfd, send_buf, strlen(send_buf) + 1, 0);

        memset(recv_buf, 0, strlen(recv_buf));
        int ret = recv(sockfd, recv_buf, RECV_BUF_SIZE, 0);
        if(ret == 0)
        {
            printf("服务端关闭连接\n");
            break;
        }
        else if(ret == -1)
        {
            perror("连接错误！");
            break;
        }
        printf("服务端回复： %s\n", recv_buf);
    }

    close(sockfd);

    printf("与服务端断开连接\n");

    return 0;
}
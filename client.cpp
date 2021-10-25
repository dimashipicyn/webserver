//
// Created by Lajuana Bespin on 10/25/21.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdio>
#include <unistd.h>

char message[] = "Hello there!\n";
char buf[sizeof(message)];

int main()
{
    int sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        perror("socket");
        _exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080); // или любой другой порт...
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        _exit(2);
    }

    send(sock, message, sizeof(message), 0);
    recv(sock, buf, sizeof(buf), 0);

    printf(buf);
    close(sock);

    return 0;
}

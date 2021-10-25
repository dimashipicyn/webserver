#include <iostream>
#include <sys/poll.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <vector>

#define BUFSIZE 1024
#define PORT 8080

class Server {
public:
    Server() : mServerPort(8080) {

        // Creating socket file descriptor
        // SOCK_STREAM потоковый сокет
        // IPPROTO_TCP протокол TCP
        if ((mListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0) {
            throw std::runtime_error("socket failed");
        }

        // неблокирующий сокет
        fcntl(mListenSocket, F_SETFL, O_NONBLOCK);

        // опции сокета, для переиспользования локальных адресов
        int option_value = 1; // включение опции
        if (setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(option_value))) {
            throw std::runtime_error("setsockopt failed");
        }

        // инициализируем структуру
        mAddress.sin_family = AF_INET; // domain AF_INET для ipv4, AF_INET6 для ipv6, AF_UNIX для локальных сокетов
        mAddress.sin_addr.s_addr = INADDR_ANY; // любой адрес
        mAddress.sin_port = htons( mServerPort ); // host-to-network short

        socklen_t addrLen = sizeof(mAddress);
        // связывает сокет с конкретным адресом
        if (bind(mListenSocket, (struct sockaddr *)&mAddress, addrLen) < 0) {
            throw std::runtime_error("bind failed");
        }

        // готовит сокет к принятию входящих соединений
        int maxConnection = 3;
        if (listen(mListenSocket, maxConnection) < 0) {
            throw std::runtime_error("listen failed");
        }

    }

    ~Server() {
        close(mListenSocket);
        close(mNewSocketFd);
    }

    int    accept() {
        socklen_t addrLen = sizeof(mAddress);
        int newConnection;
        // проверяем входящее соединение
        if ((newConnection = ::accept(mListenSocket, (struct sockaddr *)&mAddress, &addrLen)) < 0) {
            throw std::runtime_error("accept failed");
        }
        return newConnection;
    }

public:
    int                 mListenSocket; // сокет
    int                 mNewSocketFd; //
    int                 mServerPort;
    struct sockaddr_in  mAddress;
    char                mBufferRequest[BUFSIZE];
};

class SocketPool {
public:
    void addSocket(const struct pollfd& sock) {

    }
    std::vector<struct pollfd> mSocketPool;
};

int main()
{
    Server server;



    struct pollfd fds[10];
    int size = 1;
    char buf[BUFSIZE];

    memset(fds, 0, sizeof(fds));

    fds[0].fd = server.mListenSocket;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    for (int i = 0; ; i++) {
        // ждём до 5 секунд
        int ret = poll(fds, size, 5000);
        // проверяем успешность вызова
        if ( ret == -1 ) {
            // ошибка
            std::cout << "Error" << std::endl;
        }
        else if ( ret == 0 ) {
            // таймаут, событий не произошло
            std::cout << "No events" << std::endl;
        }
        else
        {
            for (int sockNum = 0; sockNum < size; ++sockNum) {
                if (fds[i].revents & POLLHUP ) {
                    return 1;
                }

                if (fds[sockNum].revents & POLLIN ) {
                    fds[sockNum].revents = 0;

                    // если сокет явлеется прослушивающим, принимаем соединение
                    if (fds[sockNum].fd == server.mListenSocket) {
                        fds[size].fd = server.accept();
                        fds[size].events = POLLIN;
                        fds[size].revents = 0;
                        size++;
                        if (size >= 10) {
                            std::cout << "End size" << std::endl;
                            return 1;
                        }
                        continue;
                    }
                    int ret = recv(fds[sockNum].fd, buf, BUFSIZE, 0);
                    if (ret == 0) {
                        std::cout << "Socket closed" << std::endl;
                    }
                    write(1, buf, ret);

                    char resp[] = "HTTP/1.1 200 OK\n"
                                  "Connection: Keep-Alive\n"
                                  "Date: Wed, 07 Sep 2016 00:38:13 GMT\n"
                                  "Keep-Alive: timeout=5, max=1000\n"
                                  "Content-Length: 12\n\n"
                                  "Hello world\n";
                    send(fds[sockNum].fd, resp, strlen(resp), 0);
                }

            }
        }
    }
    return 0;
}

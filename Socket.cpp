//
// Created by Lajuana Bespin on 10/29/21.
//

#include "Socket.h"

webserv::Socket::Socket(const std::string &host, int port)
: mPort(port), mHost(host)
{
    // Creating socket file descriptor
    // SOCK_STREAM потоковый сокет
    // IPPROTO_TCP протокол TCP
    if ((mListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0) {
        throw std::runtime_error("Socket: create socket failed");
    }

    // неблокирующий сокет
    fcntl(mListenSocket, F_SETFL, O_NONBLOCK);

    // опции сокета, для переиспользования локальных адресов
    int option_value = 1; // включение опции
    if (setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(option_value))) {
        throw std::runtime_error("Socket: setsockopt failed");
    }

    // инициализируем структуру
    mAddress.sin_family = AF_INET; // domain AF_INET для ipv4, AF_INET6 для ipv6, AF_UNIX для локальных сокетов
    mAddress.sin_addr.s_addr = inet_addr(mHost.c_str()); // любой адрес
    mAddress.sin_port = htons( mPort ); // host-to-network short

    socklen_t addrLen = sizeof(mAddress);
    // связывает сокет с конкретным адресом
    if (bind(mListenSocket, (struct sockaddr *)&mAddress, addrLen) < 0) {
        throw std::runtime_error("Socket: bind socket failed");
    }

    // готовит сокет к принятию входящих соединений
    int maxConnection = 3; // число одновременно принятых соединений, общее число может быть больше
    if (listen(mListenSocket, maxConnection) < 0) {
        throw std::runtime_error("Socket: listen socket failed");
    }
}

webserv::Socket::~Socket()
{
    close(mListenSocket);
}

int webserv::Socket::acceptConnection() const
{
    int         newConnection;
    socklen_t   addrLen = sizeof(mAddress);

    // проверяем входящее соединение
    if ((newConnection = ::accept(mListenSocket, (struct sockaddr *)&mAddress, &addrLen)) < 0) {
        throw std::runtime_error("Socket: accept failed");
    }
    return newConnection;
}

int webserv::Socket::getListenSocket() const {
    return mListenSocket;
}

webserv::Socket::Socket(const webserv::Socket &socket) {
    operator=(socket);
}

webserv::Socket &webserv::Socket::operator=(const webserv::Socket &socket) {
    mListenSocket = socket.mListenSocket;
    mPort = socket.mPort;
    mHost = socket.mHost;
    mAddress = socket.mAddress;
    return *this;
}

//
// Created by Lajuana Bespin on 10/29/21.
//

#include "Socket.h"

webserv::Socket::Socket(const std::string &host, int port)
{
    // Creating socket file descriptor
    // SOCK_STREAM потоковый сокет
    // IPPROTO_TCP протокол TCP
    if ((mSockFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0 ) {
        throw std::runtime_error("Socket: create socket failed");
    }

    // неблокирующий сокет
    if (fcntl(mSockFd, F_SETFL, O_NONBLOCK) == -1) {
        ::close(mSockFd);
        throw std::runtime_error("Socket: fcntl failed");
    }

    // опции сокета, для переиспользования локальных адресов
    int option_value = 1; // включение опции
    if ( setsockopt(mSockFd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(option_value)) ) {
        ::close(mSockFd);
        throw std::runtime_error("Socket: setsockopt failed");
    }

    // инициализируем структуру
    mAddress.sin_family = AF_INET; // domain AF_INET для ipv4, AF_INET6 для ipv6, AF_UNIX для локальных сокетов
    mAddress.sin_addr.s_addr = inet_addr(host.c_str()); // адрес host, format "127.0.0.1"
    mAddress.sin_port = htons( port ); // host-to-network short

    socklen_t addrLen = sizeof(mAddress);
    // связывает сокет с конкретным адресом
    if (bind(mSockFd, (struct sockaddr *)&mAddress, addrLen) < 0 ) {
        ::close(mSockFd);
        throw std::runtime_error("Socket: bind socket failed");
    }

    // готовит сокет к принятию входящих соединений
    int maxConnection = 3; // число одновременно принятых соединений, общее число может быть больше
    if (listen(mSockFd, maxConnection) < 0 ) {
        ::close(mSockFd);
        throw std::runtime_error("Socket: listen socket failed");
    }
}

webserv::Socket::~Socket()
{
    ::close(mSockFd);
}

int webserv::Socket::acceptConnection() const
{
    int         newConnection;
    socklen_t   addrLen = sizeof(struct sockaddr);

    // проверяем входящее соединение
    if ((newConnection = ::accept(mSockFd, (struct sockaddr *)&mAddress, &addrLen)) < 0 ) {
        throw std::runtime_error("Socket: accept failed");
    }
    return newConnection;
}

int webserv::Socket::getSockFd() const {
    return mSockFd;
}
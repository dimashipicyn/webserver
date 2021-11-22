//
// Created by Lajuana Bespin on 10/29/21.
//

#include "Listener.h"

webserv::Listener::Listener(const std::string &host, int port)
{
    // Creating socket file descriptor
    // SOCK_STREAM потоковый сокет
    // IPPROTO_TCP протокол TCP
    if ( (m_listenSd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0 ) {
        throw std::runtime_error("Listener: create socket failed");
    }

    // неблокирующий сокет
    if ( fcntl(m_listenSd, F_SETFL, O_NONBLOCK) == -1 ) {
        ::close(m_listenSd);
        throw std::runtime_error("Listener: fcntl failed");
    }

    // опции сокета, для переиспользования локальных адресов
    if ( setsockopt(m_listenSd, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) ) {
        ::close(m_listenSd);
        throw std::runtime_error("Listener: setsockopt failed");
    }

    // инициализируем структуру
    m_address.sin_family = AF_INET; // domain AF_INET для ipv4, AF_INET6 для ipv6, AF_UNIX для локальных сокетов
    m_address.sin_addr.s_addr = inet_addr(host.c_str()); // адрес host, format "127.0.0.1"
    m_address.sin_port = htons(port ); // host-to-network short

    socklen_t addrLen = sizeof(m_address);
    // связывает сокет с конкретным адресом
    if (bind(m_listenSd, (struct sockaddr *)&m_address, addrLen) < 0 ) {
        ::close(m_listenSd);
        throw std::runtime_error("Listener: bind socket failed");
    }

    // готовит сокет к принятию входящих соединений
    if (listen(m_listenSd, SOMAXCONN) < 0 ) {
        ::close(m_listenSd);
        throw std::runtime_error("Listener: listen socket failed");
    }
}

webserv::Listener::~Listener()
{
    ::close(m_listenSd);
}

int webserv::Listener::acceptConnection() const
{
    int         newConnection;
    socklen_t   addrLen = sizeof(struct sockaddr);

    // проверяем входящее соединение
    if ((newConnection = ::accept(m_listenSd, (struct sockaddr *)&m_address, &addrLen)) < 0 ) {
        throw std::runtime_error("Listener: accept failed");
    }
    return newConnection;
}

int webserv::Listener::getListenSd() const {
    return m_listenSd;
}
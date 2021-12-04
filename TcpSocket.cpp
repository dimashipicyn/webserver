//
// Created by Lajuana Bespin on 10/29/21.
//

#include "TcpSocket.h"

webserv::TcpSocket::TcpSocket(const std::string &host, int port)
        : m_sock(-1), m_addrLen(sizeof(m_address)), m_address()
{
    // Creating socket file descriptor
    // SOCK_STREAM потоковый сокет
    // IPPROTO_TCP протокол TCP
    if ( (m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0 ) {
        throw std::runtime_error("TcpSocket: create socket failed");
    }

    // опции сокета, для переиспользования локальных адресов
    if ( setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) ) {
        ::close(m_sock);
        throw std::runtime_error("TcpSocket: setsockopt failed");
    }

    // инициализируем структуру
    m_address.sin_family = AF_INET; // domain AF_INET для ipv4, AF_INET6 для ipv6, AF_UNIX для локальных сокетов
    m_address.sin_addr.s_addr = inet_addr(host.c_str()); // адрес host, format "127.0.0.1"
    m_address.sin_port = htons(port); // host-to-network short

    // связывает сокет с конкретным адресом
    if ( bind(m_sock, reinterpret_cast<sockaddr*>(&m_address), m_addrLen) < 0 ) {
        ::close(m_sock);
        throw std::runtime_error("TcpSocket: bind socket failed");
    }
}

webserv::TcpSocket::~TcpSocket()
{
    ::close(m_sock);
}

webserv::TcpSocket::TcpSocket(int sock, struct sockaddr addr) : m_sock(sock), m_addrLen(sizeof(struct sockaddr_in)), m_address()  {
    std::memcpy(&m_address, &addr, sizeof(struct sockaddr));
}

webserv::TcpSocket::TcpSocket(const webserv::TcpSocket &socket) {
    operator=(socket);
}

webserv::TcpSocket &webserv::TcpSocket::operator=(const webserv::TcpSocket &socket) {
    if ( &socket == this )
        return *this;
    m_sock = ::dup(socket.m_sock);
    m_address = socket.m_address;
    m_addrLen = socket.m_addrLen;
    return *this;
}


void webserv::TcpSocket::listen() const {
    // готовит сокет к принятию входящих соединений
    if ( ::listen(m_sock, SOMAXCONN) < 0 ) {
        throw std::runtime_error("TcpSocket: listen socket failed");
    }
}

void webserv::TcpSocket::connect() {
    if ( ::connect(m_sock, reinterpret_cast<sockaddr*>(&m_address), m_addrLen ) < 0 ) {
        throw std::runtime_error("TcpSocket: connect socket failed");
    }
}

void webserv::TcpSocket::makeNonBlock() {
    // неблокирующий сокет
    if ( fcntl(m_sock, F_SETFL, O_NONBLOCK) == -1 ) {
        throw std::runtime_error("TcpSocket: fcntl failed");
    }
}

webserv::TcpSocket webserv::TcpSocket::accept() const
{
    int         conn;
    socklen_t   addrLen = sizeof(struct sockaddr);

    // проверяем входящее соединение
    if ( (conn = ::accept(m_sock, (struct sockaddr *)&m_address, &addrLen)) < 0 ) {
        throw std::runtime_error("TcpSocket: accept failed");
    }
    webserv::TcpSocket t(*this);
    ::close(t.m_sock);
    t.m_sock = conn;
    return t;
}

int webserv::TcpSocket::getSock() const {
    return m_sock;
}

struct sockaddr* webserv::TcpSocket::getAddr() {
    return reinterpret_cast<struct sockaddr*>(&m_address);
}
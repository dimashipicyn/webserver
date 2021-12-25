//
// Created by Lajuana Bespin on 10/29/21.
//

#include "TcpSocket.h"

TcpSocket::TcpSocket(const std::string &host, int port)
        : sock_(-1),
          addrLen_(sizeof(address_)),
          address_()
{
    // Creating socket file descriptor
    // SOCK_STREAM потоковый сокет
    // IPPROTO_TCP протокол TCP
    if ( (sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0 ) {
        throw std::runtime_error("TcpSocket: create socket failed");
    }

    // опции сокета, для переиспользования локальных адресов
    if ( setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) ) {
        ::close(sock_);
        throw std::runtime_error("TcpSocket: setsockopt failed");
    }

    struct sockaddr_in addr;
    // инициализируем структуру
    addr.sin_family = AF_INET; // domain AF_INET для ipv4, AF_INET6 для ipv6, AF_UNIX для локальных сокетов
    addr.sin_addr.s_addr = inet_addr(host.c_str()); // адрес host, format "127.0.0.1"
    addr.sin_port = htons(port); // host-to-network short
    std::memcpy(&address_, &addr, addrLen_);

    // связывает сокет с конкретным адресом
    if ( bind(sock_, reinterpret_cast<sockaddr*>(&address_), addrLen_) < 0 ) {
        ::close(sock_);
        throw std::runtime_error("TcpSocket: bind socket failed");
    }
}

TcpSocket::~TcpSocket()
{
    //::close(sock_);
}

TcpSocket::TcpSocket(int sock, const struct sockaddr& addr)
    : sock_(sock),
      addrLen_(sizeof(struct sockaddr_in)),
      address_()
{
    std::memcpy(&address_, &addr, sizeof(struct sockaddr));
}

TcpSocket::TcpSocket(const TcpSocket &socket) {
    operator=(socket);
}

TcpSocket &TcpSocket::operator=(const TcpSocket &socket) {
    if ( &socket == this )
        return *this;
    sock_ = ::dup(socket.sock_);
    address_ = socket.address_;
    addrLen_ = socket.addrLen_;
    return *this;
}


void TcpSocket::listen() const {
    // готовит сокет к принятию входящих соединений
    if ( ::listen(sock_, SOMAXCONN) < 0 ) {
        throw std::runtime_error("TcpSocket: listen socket failed");
    }
}

void TcpSocket::connect() const {
    if ( ::connect(sock_, &address_, addrLen_) < 0 ) {
        throw std::runtime_error("TcpSocket: connect socket failed");
    }
}

void TcpSocket::makeNonBlock() const {
    // неблокирующий сокет
    if ( fcntl(sock_, F_SETFL, O_NONBLOCK) == -1 ) {
        throw std::runtime_error("TcpSocket: fcntl failed");
    }
}

TcpSocket TcpSocket::accept() const
{
    int         conn;

    // проверяем входящее соединение
    if ( (conn = ::accept(sock_, (struct sockaddr *)&address_, (socklen_t[]){sizeof(struct sockaddr)})) < 0 ) {
        throw std::runtime_error("TcpSocket: accept failed");
    }
    return TcpSocket(conn, address_);
}

int TcpSocket::getSock() const {
    return sock_;
}

const struct sockaddr& TcpSocket::getAddr() const {
    return address_;
}

struct sockaddr* TcpSocket::getAddr() {
    return &address_;
}

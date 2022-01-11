//
// Created by Lajuana Bespin on 10/29/21.
//

#include "TcpSocket.h"
#include "utils.h"

TcpSocket::TcpSocket(const std::string &host)
        : sock_(-1),
          addrLen_(sizeof(address_)),
          address_(),
          host_(host)
{

    size_t found = host.find(":");
    if (found == std::string::npos) {
        throw std::runtime_error("TcpSocket: host format error. Use 'host:port'.");
    }

    int16_t port = utils::to_number<int16_t>(std::string(host[found + 1], host[host.size() - 1]));
    std::string hostTmp(host[0], host[found]);

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

    struct sockaddr_in  addr;

    // инициализируем структуру
    addr.sin_family = AF_INET; // domain AF_INET для ipv4, AF_INET6 для ipv6, AF_UNIX для локальных сокетов
    addr.sin_addr.s_addr = inet_addr(hostTmp.c_str()); // адрес host, format "127.0.0.1"
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
    ::close(sock_);
}

TcpSocket::TcpSocket(int sock, const struct sockaddr& addr, const std::string& host)
    : sock_(sock),
      addrLen_(sizeof(struct sockaddr_in)),
      address_(),
      host_(host)
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
    host_ = socket.host_;
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
    return TcpSocket(conn, address_, host_);
}

int64_t     TcpSocket::read(void *buf, size_t bytes) {
    return ::read(sock_, buf, bytes);
}

int64_t     TcpSocket::write(void *buf, size_t bytes) {
    return ::write(sock_, buf, bytes);
}


int TcpSocket::getSock() const {
    return sock_;
}

const std::string& TcpSocket::getHost() const {
    return host_;
}

const struct sockaddr& TcpSocket::getAddr() const {
    return address_;
}

struct sockaddr* TcpSocket::getAddr() {
    return &address_;
}

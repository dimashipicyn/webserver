//
// Created by Lajuana Bespin on 10/29/21.
//

#include "TcpSocket.h"
#include "utils.h"
#include "Logger.h"

TcpSocket::TcpSocket()
        : sock(-1)
        , address()
        , host()
{
}

TcpSocket::TcpSocket(const std::string &host)
        : sock(-1)
        , address()
        , host(host)
{

    if (!utils::isValidPairString(host, ':')) {
        throw std::runtime_error("TcpSocket: host format error. Use 'host:port'.");
    }

    std::pair<std::string, std::string> hostPortPair = utils::breakPair(host, ':');

    std::string hostTmp = hostPortPair.first;
    int port = utils::to_number<int>(hostPortPair.second);

    LOG_DEBUG("%s %d\n", hostTmp.c_str(), port);
    // Creating socket file descriptor
    // SOCK_STREAM потоковый сокет
    // IPPROTO_TCP протокол TCP
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0 ) {
        throw std::runtime_error("TcpSocket: create socket failed");
    }

    // опции сокета, для переиспользования локальных адресов
    if ( setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) ) {
        ::close(sock);
        throw std::runtime_error("TcpSocket: setsockopt failed");
    }

    struct sockaddr_in  addr = {};

    // инициализируем структуру
    addr.sin_family = AF_INET; // domain AF_INET для ipv4, AF_INET6 для ipv6, AF_UNIX для локальных сокетов
    addr.sin_addr.s_addr = inet_addr(hostTmp.c_str()); // адрес host, format "127.0.0.1"
    addr.sin_port = htons(port); // host-to-network short
    std::memcpy(&address, &addr, sizeof(address));

    // связывает сокет с конкретным адресом
    if (bind(sock, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0 ) {
        ::close(sock);
        throw std::runtime_error("TcpSocket: bind socket failed. " + host);
    }
}

TcpSocket::~TcpSocket()
{
}

TcpSocket::TcpSocket(const TcpSocket &socket)
        : sock(socket.sock)
        , address(socket.address)
        , host(socket.host)
{
}

TcpSocket &TcpSocket::operator=(const TcpSocket &socket) {
    if ( &socket == this )
        return *this;

    sock = socket.sock;
    address = socket.address;
    host = socket.host;
    return *this;
}

bool TcpSocket::operator==(int s) {
    if (sock == s) {
        return true;
    }
    return false;
}

void TcpSocket::listen() const {
    // готовит сокет к принятию входящих соединений
    if (::listen(sock, SOMAXCONN) < 0 ) {
        throw std::runtime_error("TcpSocket: listen socket failed");
    }
}

void TcpSocket::connect() const {
    if (::connect(sock, &address, sizeof(address)) < 0 ) {
        throw std::runtime_error("TcpSocket: connect socket failed");
    }
}

void TcpSocket::makeNonBlock() const {
    // неблокирующий сокет
    if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1 ) {
        throw std::runtime_error("TcpSocket: fcntl failed");
    }
}

TcpSocket TcpSocket::accept() const
{
    int         conn;

    // проверяем входящее соединение
    if ((conn = ::accept(sock, (struct sockaddr *)&address, (socklen_t[]){sizeof(struct sockaddr)})) < 0 ) {
        throw std::runtime_error("TcpSocket: accept failed");
    }
    TcpSocket res;
    res.sock = conn;
    res.address = address;
    res.host = host;
    return res;
}

int TcpSocket::getSock() const {
    return sock;
}

const std::string& TcpSocket::getHost() const {
    return host;
}

struct sockaddr* TcpSocket::getAddr() {
    return &address;
}

namespace tcp
{
    tdSocketAddressPair tcpSocket(const std::string host)
    {
        size_t found = host.find(":");
        if (found == std::string::npos) {
            throw std::runtime_error("TcpSocket: host format error. Use 'host:port'.");
        }

        int port = utils::to_number<int>(host.substr(found + 1, host.size() - found));
        std::string hostTmp = host.substr(0, found);

        LOG_DEBUG("%s %d\n", hostTmp.c_str(), port);

        // Creating socket file descriptor
        // SOCK_STREAM потоковый сокет
        // IPPROTO_TCP протокол TCP
        int sock;
        if ( (sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0 ) {
            throw std::runtime_error("TcpSocket: create socket failed");
        }

        // опции сокета, для переиспользования локальных адресов
        if ( setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) ) {
            ::close(sock);
            throw std::runtime_error("TcpSocket: setsockopt failed");
        }


        // инициализируем структуру
        struct sockaddr_in  addr;
        addr.sin_family = AF_INET; // domain AF_INET для ipv4, AF_INET6 для ipv6, AF_UNIX для локальных сокетов
        addr.sin_addr.s_addr = inet_addr(hostTmp.c_str()); // адрес host, format "127.0.0.1"
        addr.sin_port = htons(port); // host-to-network short

        // связывает сокет с конкретным адресом
        socklen_t addrLen = sizeof(addr);
        if ( ::bind(sock, reinterpret_cast<sockaddr*>(&addr), addrLen) < 0 ) {
            ::close(sock);
            throw std::runtime_error("TcpSocket: bind socket failed. " + host);
        }
        return std::make_pair(sock, addr);
    }

    tdSocketAddressPair accept(const tdSocketAddressPair& conn)
    {
        int         newSocket;
        int         socket = conn.first;
        struct sockaddr_in addr = conn.second;

        // проверяем входящее соединение
        socklen_t addrLen = sizeof(struct sockaddr);
        if ( (newSocket = ::accept(socket, (struct sockaddr *)&addr, &addrLen)) < 0 ) {
            throw std::runtime_error("TcpSocket: accept failed");
        }
        return std::make_pair(newSocket, addr);
    }

    void    listen(const tdSocketAddressPair& conn)
    {
        int socket = conn.first;
        // готовит сокет к принятию входящих соединений
        if ( ::listen(socket, SOMAXCONN) < 0 ) {
            throw std::runtime_error("TcpSocket: listen socket failed");
        }
    }

    void    connect(const tdSocketAddressPair& conn)
    {
        int                 socket = conn.first;
        struct sockaddr_in  addr = conn.second;
        socklen_t           addrLen = sizeof(struct sockaddr);

        if ( ::connect(socket, (struct sockaddr*)&addr, addrLen) < 0 ) {
            throw std::runtime_error("TcpSocket: connect socket failed");
        }
    }

    void    makeNonBlock(const tdSocketAddressPair& conn)
    {
        int socket = conn.first;
        // неблокирующий сокет
        if ( fcntl(socket, F_SETFL, O_NONBLOCK) == -1 ) {
            throw std::runtime_error("TcpSocket: fcntl failed");
        }
    }
}

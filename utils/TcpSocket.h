//
// Created by Lajuana Bespin on 10/29/21.
//

#ifndef WEBSERV_TCPSOCKET_H
#define WEBSERV_TCPSOCKET_H
#include <iostream>
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>

class TcpSocket
{
public:
    TcpSocket();
    explicit TcpSocket(const std::string& host); // format "host:port"
    TcpSocket(const TcpSocket& socket);
    TcpSocket& operator=(const TcpSocket& socket);
    virtual ~TcpSocket();
    bool operator==(int sock);

    TcpSocket   accept() const;
    void        listen() const;
    void        connect() const;
    void        makeNonBlock() const;

    int                     getSock() const;
    const std::string&      getHost() const;
    struct sockaddr*        getAddr();

private:
    int                 sock;
    struct sockaddr     address;
    std::string         host;
};

namespace tcp
{
    typedef std::pair<int /*socket*/, struct sockaddr_in /*address*/> tdSocketAddressPair;

    tdSocketAddressPair tcpSocket(const std::string host);
    tdSocketAddressPair accept(const tdSocketAddressPair& conn);
    void    listen(const tdSocketAddressPair& conn);
    void    connect(const tdSocketAddressPair& conn);
    void    makeNonBlock(const tdSocketAddressPair& conn);
}

#endif //WEBSERV_TCPSOCKET_H

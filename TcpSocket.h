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
    TcpSocket(const std::string& host, int port);
    TcpSocket(int sock, const struct sockaddr& addr);
    TcpSocket(const TcpSocket& socket);
    TcpSocket& operator=(const TcpSocket& socket);
    virtual ~TcpSocket();

    TcpSocket   accept() const;
    void        listen() const;
    void        connect() const;
    void        makeNonBlock() const;
    int         getSock() const;

    const struct sockaddr&  getAddr() const;
    struct sockaddr*        getAddr();

private:
    int                 sock_;
    socklen_t           addrLen_;
    struct sockaddr     address_;
};

#endif //WEBSERV_TCPSOCKET_H

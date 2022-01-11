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
    TcpSocket(const std::string& host); // format "host:port"
    TcpSocket(const TcpSocket& socket);
    TcpSocket& operator=(const TcpSocket& socket);
    virtual ~TcpSocket();

    TcpSocket   accept() const;
    void        listen() const;
    void        connect() const;
    void        makeNonBlock() const;
    int64_t     read(void *buf, size_t bytes);
    int64_t     write(void *buf, size_t bytes);



    int                     getSock() const;
    const std::string&      getHost() const;
    const struct sockaddr&  getAddr() const;
    struct sockaddr*        getAddr();

private:
    TcpSocket(int sock, const struct sockaddr& addr, const std::string& host);

private:
    int                 sock_;
    socklen_t           addrLen_;
    struct sockaddr     address_;
    std::string         host_;
};

#endif //WEBSERV_TCPSOCKET_H

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


namespace webserv {
    class TcpSocket {
    public:
        TcpSocket(const std::string& host, int port);
        TcpSocket(int sock, struct sockaddr addr);
        ~TcpSocket();
        TcpSocket(const TcpSocket& socket);
        TcpSocket& operator=(const TcpSocket& socket);

        TcpSocket   accept() const;
        void        listen() const;
        void        connect();
        void        makeNonBlock();
        int         getSock() const;
        struct sockaddr* getAddr();

    private:
        int                 m_sock;
        socklen_t           m_addrLen;
        struct sockaddr_in  m_address;
    };
}

#endif //WEBSERV_TCPSOCKET_H

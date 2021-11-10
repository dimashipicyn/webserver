//
// Created by Lajuana Bespin on 10/29/21.
//

#ifndef WEBSERV_SOCKET_H
#define WEBSERV_SOCKET_H
#include <iostream>
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>


namespace webserv {
    class Socket {
    public:
        Socket(const std::string& host, int port);
        ~Socket();


        /***
         * Принимает соединение на слушающем сокете.
         * Throw exception если не удалось принять соединение.
         * @return Новый дескриптор с принятым соединением.
         */
        int     acceptConnection() const;
        int     getSockFd() const;

    private:
        Socket(const Socket& socket) {};
        Socket& operator=(const Socket& socket) {};

    private:
        int                 mSockFd;
        struct sockaddr_in  mAddress;
    };
}

#endif //WEBSERV_SOCKET_H

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
        Socket(const Socket& socket);
        Socket& operator=(const Socket& socket);

        /***
         * Принимает соединение на слушающем сокете.
         * Throw exception если не удалось принять соединение.
         * @return Новый сокет с принятым соединением.
         */
        int     acceptConnection() const;
        int     getListenSocket() const;

    private:
        int                 mListenSocket;
        int                 mPort;
        std::string         mHost;
        struct sockaddr_in  mAddress;
    };
}

#endif //WEBSERV_SOCKET_H

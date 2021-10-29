//
// Created by Lajuana Bespin on 10/29/21.
//

#ifndef WEBSERV_HTTPSERVER_H
#define WEBSERV_HTTPSERVER_H
#include <iostream>
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>


namespace webserv {
    class HttpServer {
    public:
        HttpServer(const std::string& host, int port);
        ~HttpServer();

        /***
         * Принимает соединение на слушающем сокете.
         * Throw exception если не удалось принять соединение.
         * @return Новый сокет с принятым соединением.
         */
        int     acceptConnection();
        int     getListenSocket() const;

    private:
        int                 mListenSocket;
        int                 mPort;
        std::string         mHost;
        struct sockaddr_in  mAddress;
    };
}

#endif //WEBSERV_HTTPSERVER_H

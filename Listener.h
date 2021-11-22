//
// Created by Lajuana Bespin on 10/29/21.
//

#ifndef WEBSERV_LISTENER_H
#define WEBSERV_LISTENER_H
#include <iostream>
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>


namespace webserv {
    class Listener {
    public:
        Listener(const std::string& host, int port);
        ~Listener();

        int     acceptConnection() const;
        int     getListenSd() const;

    private:
        Listener(const Listener& socket) {};
        Listener& operator=(const Listener& socket) {};

    private:
        int                 m_listenSd;
        struct sockaddr_in  m_address;
    };
}

#endif //WEBSERV_LISTENER_H

#include "Socket.h"
#include "EventPool.h"


int main()
{
    webserv::Socket socket("127.0.0.1", 1234);
    //webserv::Socket socket2("127.0.0.1", 1232);
    webserv::EventPool  eventPool;
    eventPool.addListenSocket(socket);
    //eventPool.addListenSocket(socket1);
    //eventPool.addListenSocket(socket2);
    eventPool.eventLoop();

    return 0;
}

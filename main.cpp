#include "Socket.h"
#include "EventPool.h"


int main()
{
    webserv::Socket socket("127.0.0.1", 1234);
    webserv::EventPool  eventPool;
    eventPool.addListenSocket(socket);
    eventPool.runEventLoop();

    return 0;
}

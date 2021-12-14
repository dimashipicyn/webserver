#include <sys/event.h>
#include "TcpSocket.h"
#include "EventPool.h"
#include "http.h"

class ddd : public IHandle {
public:
    virtual ~ddd() {};
    void handler(int conn, const Request& req) {
        std::cout << "handler calling" << std::endl;
        std::string s("hello world\n");
        ::write(conn, s.c_str(), s.size());
    }
};


int main()
{
    EventPool evPool;
    HTTP serve(&evPool, "127.0.0.1");
    ddd h;

    serve.handle("/docs", &h);
    serve.handle("/", &h);
    serve.start();
    return 0;
}

#include <sstream>
#include "TcpSocket.h"
#include "EventPool.h"
#include "http.h"
#include "Request.h"

class ddd : public IHandle {
public:
    virtual ~ddd() {};
    void handler(int conn, const Request& req) {

        if (req.getMethod() == "GET") {
            std::stringstream ss;

            ss << "HTTP/1.1 200 OK\n"
               << "Content-Length: 17\n"
               << "Content-Type: text/html\n\n"
               << "Hello World!\n";
            ::write(conn, ss.str().c_str(), ss.str().size());
        }
    }
};


int main()
{
    EventPool evPool;
    HTTP serve(&evPool, "127.0.0.1");
    ddd h;

    serve.handle("/", &h);
    serve.start();
    return 0;
}

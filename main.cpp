#include <sstream>
#include <fstream>
#include <iterator>
#include "TcpSocket.h"
#include "EventPool.h"
#include "http.h"
#include "Request.h"
#include "Response.h"

class ddd : public IHandle {
public:
    virtual ~ddd() {};
    void handler(Request& req, Response& resp) {
        if (req.getMethod() == "GET") {
            std::stringstream ss;
            /*
            std::ifstream f("/Users/dmitry/programming/webserver/index.html");
            if (!f) {
                std::cout << "file exist\n";
                return;
            }
            std::istreambuf_iterator<char> it(f), end;
            std::string s(it, end);
            */
            std::string s("Hello Webserver!\n");
            ss << "HTTP/1.1 200 OK\n"
               << "Content-Length: " << s.size() << "\n"
               << "Content-Type: text/html\n\n";
            resp.setContent(ss.str() + s);
        }
    }
};

int main()
{
    EventPool evPool;
    HTTP serve(&evPool, "127.0.0.1", 1234);
    ddd h;
    serve.handle("/", &h);
    serve.start();
    return 0;
}

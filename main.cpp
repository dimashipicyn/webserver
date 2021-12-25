#include <sys/event.h>
#include "TcpSocket.h"
#include "EventPool.h"
#include "http.h"
#include "Response.h"

class ddd : public IHandle {
public:
    void handler(int conn, const Request& req) {
        Response res;
		res.write(conn);
    }
};


int main()
{
    HTTP serve("127.0.0.1");
    ddd h;

    serve.handle("/docs", &h);
    serve.handle("/", &h);
    serve.start();
    return 0;
}

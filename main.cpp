#include <sstream>
#include <fstream>
#include "http.h"
#include "Response.h"
#include "Request.h"

int main()
{
    HTTP serve("127.0.0.1", 1234);
    serve.start();
    return 0;
}

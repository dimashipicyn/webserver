#ifndef KQUEUE_H
#define KQUEUE_H

#include <vector>

class Kqueue {
public:
    Kqueue(); //throw exception
    ~Kqueue();

    enum {
        M_READ      = 1 << 0,   //
        M_WRITE     = 1 << 1,   // general flags, set and returned
        M_TIMER     = 1 << 2,   //

        M_ENABLE    = 1 << 3,   //
        M_DISABLE   = 1 << 4,   //
        M_ADD       = 1 << 5,   // action flags, only set
        M_ONESHOT   = 1 << 6,   // allow mixed
        M_CLEAR     = 1 << 7,   //
        M_DELETE    = 1 << 8,   //

        M_EOF       = 1 << 9,   // returned values
        M_ERROR     = 1 << 10   //
    };

    struct ev {
        std::int32_t    fd;
        std::uint16_t   flags;
        void            *ctx;
        std::int32_t    data;
    };

    // modified vector, return n modified < nEvents.size()
    int     getEvents(std::vector<struct ev>& nEvents);                                 // throw exception
    void    setEvent(std::vector<struct ev>& changeEvents);    // throw exception

private:
    int                         kq;
    std::vector<struct kevent>  nEvents_;
    std::vector<struct kevent>  chEvents_;
};

#endif // KQUEUE_H

#ifndef KQUEUE_H
#define KQUEUE_H

#include <vector>

class Kqueue {
public:
    Kqueue();
    ~Kqueue();

    enum {
        M_READ = 1,
        M_WRITE = 2,
        M_TIMER = 4,
        M_EOF = 8,
        M_ERROR = 16,
        M_ENABLE = 32,
        M_DISABLE = 64,
        M_ADD = 128,
        M_ONESHOT = 256,
        M_CLEAR = 512
    };

    struct ev {
        std::int32_t    fd;
        std::uint16_t   flags;
        void            *ctx;
    };

    int     getEvents(std::vector<struct ev>& nEvents);
    void    setEvent(int fd, std::uint16_t flags, void *ctx, std::int64_t time = 0);

private:
    int                         kq;
    std::vector<struct kevent>  nEvents_;
    std::vector<struct kevent>  chEvents_;
};

#endif // KQUEUE_H

#include <sys/event.h>
#include "kqueue.h"

Kqueue::Kqueue()
    : nEvents_(),
      chEvents_()
{
    kq = kqueue();
    if (kq < 0) {
        throw std::runtime_error("kqueue init failed");
    }
}

Kqueue::~Kqueue() {

}

int Kqueue::getEvents(std::vector<struct ev>& events) {
    struct timespec tmout = {5, 0};

    if (events.size() != nEvents_.size()) {
        nEvents_.resize(events.size());
    }
    int n = kevent(kq, chEvents_.data(), chEvents_.size(), nullptr, 0, &tmout);
    chEvents_.clear();
    n = kevent(kq, nullptr, 0, nEvents_.data(), nEvents_.size(), &tmout);
    if (n == -1) {
        std::runtime_error("kevent() error");
    }
    for (int i = 0; i < n; ++i) {
        struct ev event = {};
        if (nEvents_[i].flags & EV_EOF) {
            event.flags |= M_EOF;
        }
        if (nEvents_[i].flags & EV_ERROR) {
            event.flags |= M_ERROR;
        }
        if (event.flags == 0) // dont have error, eof
        {
            if (nEvents_[i].filter == EVFILT_READ) {
                event.flags |= M_READ;
            }
            if (nEvents_[i].filter == EVFILT_WRITE) {
                event.flags |= M_WRITE;
            }
            if (nEvents_[i].filter == EVFILT_TIMER) {
                event.flags |= M_TIMER;
            }
        }
        event.fd = static_cast<std::int32_t>(nEvents_[i].ident);
        event.ctx = nEvents_[i].udata;
        events[i] = event;
    }
    return n;
}

void Kqueue::setEvent(int32_t fd, uint16_t flags, void *ctx, int32_t time)
{
    struct kevent event = {};
    if (flags & M_READ) {
        event.filter |= EVFILT_READ;
    }
    if (flags & M_WRITE) {
        event.filter |= EVFILT_WRITE;
    }
    if (flags & M_TIMER) {
        event.filter |= EVFILT_TIMER;
    }
    if (flags & M_ENABLE) {
        event.flags |= EV_ENABLE;
    }
    if (flags & M_DISABLE) {
        event.flags |= EV_DISABLE;
    }
    if (flags & M_ADD) {
        event.flags |= EV_ADD;
    }
    if (flags & M_ONESHOT) {
        event.flags |= EV_ONESHOT;
    }
    if (flags & M_CLEAR) {
        event.flags |= EV_CLEAR;
    }
    if (flags & M_DELETE) {
        event.flags |= EV_DELETE;
    }
    event.ident = fd;
    event.udata = ctx;
    event.data = time;
    chEvents_.push_back(event);
}

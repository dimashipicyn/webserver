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

void Kqueue::getEvents(std::vector<struct ev>& events) {
    if (events.size() != nEvents_.size()) {
        nEvents_.resize(events.size());
    }
    int n = kevent(kq, chEvents_.data(), chEvents_.size(), nEvents_.data(), nEvents_.size(), nullptr);
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
        if (nEvents_[i].filter == EVFILT_READ) {
            event.flags |= M_READ;
        }
        if (nEvents_[i].filter == EVFILT_WRITE) {
            event.flags |= M_WRITE;
        }
        if (nEvents_[i].filter == EVFILT_TIMER) {
            event.flags |= M_TIMER;
        }
        event.fd = static_cast<std::int32_t>(nEvents_[i].ident);
        event.ctx = nEvents_[i].udata;
        events[i] = event;
    }
    chEvents_.clear();
}

void Kqueue::setEvent(int fd, std::uint16_t flags, void *ctx) {
    struct kevent event = {};

    if (flags & M_READ) {
        event.filter = EVFILT_READ;
    }
    else if (flags & M_WRITE) {
        event.filter = EVFILT_WRITE;
    }
    else if (flags & M_TIMER) {
        event.filter = EVFILT_TIMER;
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
    event.ident = fd;
    event.udata = ctx;
    chEvents_.push_back(event);
}

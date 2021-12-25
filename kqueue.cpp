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
    int n = kevent(kq, chEvents_.data(), chEvents_.size(), nEvents_.data(), nEvents_.size(), &tmout);
    chEvents_.clear();

    if (n == -1) {
        std::runtime_error("kevent() error");
    }
    for (int i = 0; i < n; ++i) {
        struct ev event = {0, 0, 0};
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

void Kqueue::setEvent(std::vector<struct ev>& changeEvents) {
    for (std::vector<struct ev>::size_type i = 0; i < changeEvents.size(); i++) {
        struct kevent event = {};
        std::uint16_t flags = changeEvents[i].flags;
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
        event.ident = changeEvents[i].fd;
        event.udata = changeEvents[i].ctx;
        event.data = changeEvents[i].data;
        chEvents_.push_back(event);
    }
}

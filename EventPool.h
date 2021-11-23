//
// Created by Lajuana Bespin on 10/29/21.
//

#ifndef WEBSERV_EVENTPOOL_H
#define WEBSERV_EVENTPOOL_H

namespace webserv {
    class EventPool {
    public:
        struct Event;
        typedef void (*acceptCB)(EventPool *evt, int sd, struct sockaddr *addr);
        typedef void (*eventCB)(EventPool *evt, struct Event *event, std::uint16_t flags, std::uintptr_t ctx);
        typedef void (*readCB)(EventPool *evt, struct Event *event, std::uintptr_t ctx);
        typedef void (*writeCB)(EventPool *evt, struct Event *event, std::uintptr_t ctx);

        enum type {
            READ  = -1,
            WRITE = -2,
            TIMER = -7
        };

        class Event {
        public:
            Event(int sock, struct sockaddr *addr);
            ~Event();

            void            setCb(readCB rcb, writeCB wcb, eventCB ecb, std::uintptr_t ctx);
            int             getSock();
            struct sockaddr *getAddr();
            std::uintptr_t  getCtx();

        public:
            int             m_sock;
            struct sockaddr *m_addr;
            std::uintptr_t  m_ctx;
            readCB          m_readCb;
            writeCB         m_writeCb;
            eventCB         m_eventCb;
        };

    public:
        EventPool();
        ~EventPool();

        void eventLoop();
        void addListener(int sock, struct sockaddr *addr, acceptCB acceptCb);
        void eventAdd(struct Event *event);
        void eventEnable(struct Event *event, std::int16_t flags);
        void eventDisable(struct Event *event, std::int16_t flags);

    private:
        int                                                     mKqueue;
        std::map<int, std::pair<struct sockaddr *, acceptCB> >  mListenSockets;
    };
}

#endif //WEBSERV_EVENTPOOL_H
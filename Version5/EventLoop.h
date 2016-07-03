
#ifndef THORSANVIL_SOCKET_EVENT_LOOP_H
#define THORSANVIL_SOCKET_EVENT_LOOP_H

#include "Socket.h"
#include "CommonNonBlocking.h"
#include <boost/coroutine/all.hpp>
#include <event2/event.h>
#include <memory>

namespace ThorsAnvil
{
    namespace Socket
    {

extern "C" void callbackEventClient(int fd, short event, void* cbData);
extern "C" void callbackEventServer(int fd, short event, void* cbData);

using EventBase = struct event_base;
using Event     = struct event;

auto eventBaseDeleter = [](EventBase* base) {event_base_free(base);};
auto eventDeleter     = [](Event* ev)       {event_del(ev);event_free(ev);};

class EventLoop
{
    private:
        using EventBasePtr    = std::unique_ptr<event_base, decltype(eventBaseDeleter)&>;
        EventBasePtr    eventBase;
        
    public:
        EventLoop();
        void runLoop();
        operator EventBase*() const {return eventBase.get();}
};


class EventClient
{
    public:
        enum ConnectionPhase {Created, Read, Write, Done};
    private:
        using PullType = boost::coroutines::asymmetric_coroutine<ConnectionPhase>::pull_type;
        using PushType = boost::coroutines::asymmetric_coroutine<ConnectionPhase>::push_type;
        using EventPtr = std::unique_ptr<Event, decltype(eventDeleter)>;

        struct NonBlockingPolicy: public PolicyNonBlocking
        {
            PushType*   sinkPtr = nullptr;

            virtual void setNonBlocking(int socketId) override;
            virtual void readWouldBlock() override;
            virtual void writeWouldBlock() override;
        };
        NonBlockingPolicy   nonBlockingPolicy;
        EventLoop&          loop;
        ActionNonBlocking&  action;
        DataSocket          connection;
        ConnectionPhase     phase;

        EventPtr            event;
        PullType            coRoutineHandler;

        void processesHttpRequest(PushType& sink);
        void setUpEventLoop();

    public:
        EventClient(EventLoop& loop, ServerSocket& server, ActionNonBlocking& action);
        ConnectionPhase     getPhase() const {return phase;}

    private:
        friend void callbackEventClient(int fd, short event, void* cbData);
        bool handleEvent();
};

class EventServer
{
    private:
        using EventPtr          = std::unique_ptr<event, decltype(eventDeleter)&>;

        EventLoop&          loop;
        ActionNonBlocking&  action;
        ServerSocket        server;
        EventPtr            event;

    public:
        EventServer(int port, EventLoop& loop, ActionNonBlocking& action);

    private:
        friend void callbackEventServer(int fd, short event, void* cbData);
        void handleEvent();
};

    }
}

#endif


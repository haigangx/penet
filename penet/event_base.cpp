#include "event_base.h"

#include "poller.h"

#include <unistd.h>


EventBase::EventBase()
{
    init();
}

EventBase::~EventBase()
{

}

void EventBase::init()
{

}

void EventBase::exit()
{

}

void EventBase::loop()
{}


EventImp::EventImp(EventBase* base, int taskCap)
    : base_(base)
    , poller_(createPoller())
    , exit_(false)
    //, nextTimeout_(1 << 30)
    //, timerSeq_(0)
    //, idleEnalbed(false)
{}

EventImp::~EventImp()
{}

void EventImp::init()
{

}
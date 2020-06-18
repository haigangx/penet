#pragma once


class EventBase
{
public:
    EventBase();
    ~EventBase();

    void init();
    void exit();
    void loop();

private:
    int wakeup_fds[2];
};

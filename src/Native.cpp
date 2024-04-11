#include <thread>

#include "Native.hpp"

void new_thread(Runtime& runtime, Thread& thread)
{
    // spawn a new thread
    std::jthread thread{};
    // create a new thread instance
    // enque this thread instance
    // and finally run it?
    
}

void new_chan(Runtime& runtime, Thread& thread)
{
    // create a new channel and put it
    // but where to put it?
    
}

void chan_send(Runtime& runtime, Thread& thread)
{
    // send to a channel
    // we expect the address of the channel
    // we can get the address of the channel from the operand stack I guess?
}

void chan_recv(Runtime& runtime, Thread& thread)
{
    // recv from a channel
    // we expect the address of the channel
    // we can get the address of the channel from the operand stack
}

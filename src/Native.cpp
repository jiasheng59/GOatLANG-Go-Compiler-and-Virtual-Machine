#include <thread>

#include "Native.hpp"

void new_thread(Runtime& runtime, Thread& thread)
{
    // prepare the argument
    // argument for this funtion is a single closure
    // so this is kinda like "invoke dynamic"
    auto& call_stack = thread.get_call_stack();
    auto& operand_stack = thread.get_operand_stack();
    // load the content into the stack
    // I think we should have function specifically for calling function
    // cause calling function is reused quite often ngl
    // spawn a new thread
    std::jthread platform_thread;
    Thread thread{runtime};
    // create a new thread instance
    // enque this thread instance
    // and finally run it?
    // but then, how can the runtime wait for this thread to complete?
    // we can enqueue this thread into the runtime, and then
    // keep adding things into the runtime
    // main thread will sleep, and will only wake up (std::conditional) when the queue
    // is empty
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

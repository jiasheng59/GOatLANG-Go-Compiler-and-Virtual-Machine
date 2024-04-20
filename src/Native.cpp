#include <iostream>
#include <thread>

#include "BlockingQueue.hpp"
#include "ChannelManager.hpp"
#include "Native.hpp"
#include "Runtime.hpp"
#include "StringPool.hpp"
#include "Thread.hpp"

void new_thread(Runtime& runtime, Thread& thread)
{
    auto& cur_call_stack = thread.get_call_stack();
    auto& cur_operand_stack = thread.get_operand_stack();

    Thread new_thread{runtime};

    auto& new_call_stack = new_thread.get_call_stack();
    auto& new_operand_stack = new_thread.get_operand_stack();
    auto& new_instruction_stream = new_thread.get_instruction_stream();
    auto& heap = runtime.get_heap();

    u64 closure_address = cur_operand_stack.pop<u64>();
    const auto& closure_header = heap.load<ClosureHeader>(closure_address);

    const auto& function = runtime.get_function_table()[closure_header.index];
    new_call_stack.push_frame(function, 0);
    new_instruction_stream.jump_to(function);

    for (u16 i = 0; i < function.capc; ++i) {
        u64 cap_address = heap.load<u64>(closure_address + sizeof(ClosureHeader) + sizeof(u64) * i);
        new_call_stack.store_local(i, cap_address);
    }

    std::thread platform_thread{[new_thread = std::move(new_thread)]() mutable {
        new_thread.start();
    }};
    platform_thread.detach();
}

void new_chan(Runtime& runtime, Thread& thread)
{
    auto& operand_stack = thread.get_operand_stack();
    auto& heap = runtime.get_heap();
    auto& channel_manager = runtime.get_channel_manager();

    u64 chan_size = operand_stack.pop<u64>();
    const auto& chan_type = *runtime.get_type_table()[Runtime::channel_type_index];
    u64 chan_address = heap.allocate(chan_type, 1);
    u64 chan_index = channel_manager.new_channel(chan_size);

    heap.store(chan_address, chan_index);
    operand_stack.push(chan_address);
}

void chan_send(Runtime& runtime, Thread& thread)
{
    // send to a channel
    // we expect the address of the channel
    // we can get the address of the channel from the operand stack I guess?
    auto& operand_stack = thread.get_operand_stack();
    auto& heap = runtime.get_heap();
    auto& channel_manager = runtime.get_channel_manager();

    u64 item_address = operand_stack.pop<u64>();
    u64 chan_address = operand_stack.pop<u64>();

    const auto& chan = heap.load<NativeChannel>(chan_address);
    auto& blocking_queue = channel_manager.get(chan.index);
    blocking_queue.push(item_address);
}

void chan_recv(Runtime& runtime, Thread& thread)
{
    // recv from a channel
    auto& operand_stack = thread.get_operand_stack();
    auto& heap = runtime.get_heap();
    auto& channel_manager = runtime.get_channel_manager();

    u64 chan_address = operand_stack.pop<u64>();
    const auto& chan = heap.load<NativeChannel>(chan_address);
    auto& blocking_queue = channel_manager.get(chan.index);

    u64 item_address;
    blocking_queue.pop(item_address);
    operand_stack.push(item_address);
}

void sprint(Runtime& runtime, Thread& thread)
{
    u64 string_address = thread.get_operand_stack().pop<u64>();
    const auto& string = runtime.get_heap().load<NativeString>(string_address);
    const auto& native_string = runtime.get_string_pool().get(string.index);
    std::cout << native_string;
}

void iprint(Runtime& runtime, Thread& thread)
{
    i64 i = thread.get_operand_stack().pop<i64>();
    std::cout << i;
}

void fprint(Runtime& runtime, Thread& thread)
{
    f64 f = thread.get_operand_stack().pop<f64>();
    std::cout << f;
}

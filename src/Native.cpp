#include <thread>

#include "Native.hpp"
#include "ChannelManager.hpp"
#include "BlockingQueue.hpp"

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
    ClosureHeader closure_header = heap.load<ClosureHeader>(closure_address);

    const auto& function = runtime.get_function_table()[closure_header.function_index];
    u64 program_counter = -1;

    new_call_stack.push_frame(function, program_counter);
    new_instruction_stream.jump_to(function);

    for (u16 i = 1; i <= function.argc; ++i) {
        Word word = cur_operand_stack.pop<Word>();
        new_call_stack.store_local(function.capc + function.argc - i, word);
    }
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
    // create a new channel and put it
    // but where to put it?
    // we will create a channel in the C++ side
    // and then store the address in the C++ side
    auto& operand_stack = thread.get_operand_stack();
    auto& heap = runtime.get_heap();
    auto& channel_manager = runtime.get_channel_manager();

    u64 chan_size = operand_stack.pop<u64>();
    u64 chan_address = heap.allocate(Runtime::channel_type_index, 1);
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

    u64 chan_address = operand_stack.pop<u64>();
    u64 chan_index = heap.load<u64>(chan_address);
    auto& blocking_queue = channel_manager.get(chan_index);

    u64 item_address = operand_stack.pop<u64>();
    blocking_queue.push(item_address);
}

void chan_recv(Runtime& runtime, Thread& thread)
{
    // recv from a channel
    auto& operand_stack = thread.get_operand_stack();
    auto& heap = runtime.get_heap();
    auto& channel_manager = runtime.get_channel_manager();

    u64 chan_address = operand_stack.pop<u64>();
    u64 chan_index = heap.load<u64>(chan_address);
    auto& blocking_queue = channel_manager.get(chan_index);

    u64 item_address;
    blocking_queue.pop(item_address);
    operand_stack.push(item_address);
}

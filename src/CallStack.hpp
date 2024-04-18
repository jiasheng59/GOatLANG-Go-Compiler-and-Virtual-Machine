#ifndef CALL_STACK_HPP
#define CALL_STACK_HPP

#include <memory>

#include "Common.hpp"
#include "Code.hpp"

struct FrameData
{
    u64 function_index;
    u64 frame_pointer;
    u64 program_counter;
};

class CallStack
{
public:
    CallStack() = default;
    CallStack(const CallStack&) = delete;
    CallStack(CallStack&&) = default;

    CallStack& operator=(const CallStack&) = delete;
    CallStack& operator=(CallStack&&) = default;

    CallStack(u64 stack_size) :
        managed_memory{std::make_unique<std::byte[]>(stack_size)},
        memory{managed_memory.get()},
        top{0},
        size{stack_size},
        frame_pointer{-1}
    {
    }

    bool empty() const { return top == 0; }
    u64 get_size() const { return size; }
    u64 get_frame_pointer() const { return frame_pointer; }

    template<typename T>
    T read_local(u64 frame_address, u64 index)
    {
        static_assert(sizeof(T) == sizeof(Word), "T must have the same size as Word");
        return read<T>(memory, frame_address + sizeof(FrameData) + sizeof(Word) * index);
    }

    template<typename T>
    void write_local(u64 frame_address, u64 index, T value)
    {
        static_assert(sizeof(T) == sizeof(Word), "T must have the same size as Word");
        write(memory, frame_address + sizeof(FrameData) + sizeof(Word) * index, value);
    }

    template<typename T>
    T load_local(u64 index)
    {
        return read_local<T>(frame_pointer, index);
    }

    template<typename T>
    void store_local(u64 index, T value)
    {
        write_local(frame_pointer, index, value);
    }

    u64 pop_frame()
    {
        FrameData frame_data = read<FrameData>(memory, frame_pointer);
        top = frame_pointer;
        frame_pointer = frame_data.frame_pointer;
        return frame_data.program_counter;
    }

    void push_frame(const Function& function, u64 program_counter)
    {
        u64 function_index = function.index;
        FrameData frame_data = {
            function_index,
            frame_pointer,
            program_counter,
        };
        write(memory, top, frame_data);
        u64 frame_size = sizeof(FrameData) + sizeof(Word) * function.varc;
        frame_pointer = top;
        top += frame_size;
    }

    const FrameData& read_frame_data(u64 frame_address)
    {
        return read<FrameData>(memory, frame_address);
    }

    const FrameData& peek_frame_data()
    {
        return read_frame_data(frame_pointer);
    }

private:
    std::unique_ptr<std::byte[]> managed_memory;
    std::byte* memory;
    u64 top;
    u64 size;
    u64 frame_pointer;
};

#endif /* CALL_STACK_HPP */

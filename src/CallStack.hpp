#ifndef CALL_STACK_HPP
#define CALL_STACK_HPP

#include <vector>

#include "Code.hpp"
#include "BitSet.hpp"
#include "Common.hpp"

struct FrameData
{
    u64 function_index;
    u64 prev_frame_pointer;
    u64 next_program_counter;
};

class CallStack
{
    u64 local_address(u64 index)
    {
        return frame_pointer + frame_data_size + word_size * index;
    }

public:
    static constexpr u64 frame_data_size = sizeof(FrameData);
    static constexpr u64 word_size = sizeof(Word);
    
    bool empty() const
    {
        return top == 0;
    }

    template<typename T>
    T load_local(u64 index)
    {
        static_assert(sizeof(T) == sizeof(Word), "T must have the same size as Word");
        return read<T>(memory, local_address(index));
    }

    template<typename T>
    void store_local(u64 index, T value)
    {
        static_assert(sizeof(T) == sizeof(Word), "T must have the same size as Word");
        write(memory, local_address(value), value);
    }

    u64 pop_frame()
    {
        FrameData frame_data = read<FrameData>(memory, frame_pointer);
        top = frame_pointer;
        frame_pointer = frame_data.prev_frame_pointer;
        return frame_data.next_program_counter;
    }

    void push_frame(u64 function_index, u64 next_program_counter)
    {
        FrameData frame_data = {
            function_index,
            frame_pointer,
            next_program_counter
        };
        write(memory, top, frame_data);
        const Function& function = function_table[function_index];
        u64 frame_size = frame_data_size + word_size * function.varc;
        frame_pointer = top;
        top += frame_size;
    }

    FrameData read_frame_data(u64 frame_address)
    {
        return read<FrameData>(memory, frame_address);
    }

    u64 get_frame_pointer()
    {
        return frame_pointer;
    }

    template<typename T>
    T read_local(u64 frame_address, u64 index)
    {
        static_assert(sizeof(T) == sizeof(Word), "T must have the same size as Word");
        return read<T>(memory, frame_address + frame_data_size + word_size * index);
    }
    
    template<typename T>
    void write_local(u64 frame_address, u64 index, T value)
    {
        static_assert(sizeof(T) == sizeof(Word), "T must have the same size as Word");
        write(memory, frame_address + frame_data_size + word_size * index, value);
    }

    std::vector<Function>& function_table;
    std::byte* memory;
    u64 top;
    u64 size;
    u64 frame_pointer;
};

#endif

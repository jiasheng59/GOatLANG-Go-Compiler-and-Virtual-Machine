#ifndef HEAP_HPP
#define HEAP_HPP

#include <vector>

#include "Code.hpp"
#include "Common.hpp"
#include "BitSet.hpp"

class Runtime;

struct BlockHeader
{
    u64 control_bits;
    u64 type_index;
    u64 count;
};

class Heap
{
    bool enough_space(u64 block_size)
    {
        return top + block_size <= size;
    }

    u64 new_block(const Type& type, u64 count);

public:
    Heap() = default;
    Heap(const Heap&) = delete;
    Heap(Heap&&) = default;
    Heap& operator=(const Heap&) = delete;
    Heap& operator=(Heap&&) = default;

    Heap(Runtime& runtime, u64 heap_size) :
        runtime{&runtime},
        mutex{},
        managed_memory{std::make_unique<std::byte[]>(heap_size * 2)},
        this_half{managed_memory.get()},
        that_half{this_half + heap_size},
        size{heap_size},
        top{0}
    {
    }

    /*
    static constexpr u64 default_control_bits = 0;
    static constexpr u64 default_foward_pointer = 0;
    static constexpr u64 mark_bit = 1;
    */

    template<typename T>
    const T& load(u64 address)
    {
        return read<T>(this_half, address);
    }

    template<typename T>
    void store(u64 address, const T& value)
    {
        write(this_half, address, value);
    }

    BlockHeader& access_block_header(u64 address)
    {
        return read<BlockHeader>(this_half, address - sizeof(BlockHeader));
    }

    u64 allocate(u64 type_index, u64 count);
    u64 allocate(const Type& type, u64 count);

    /*
    void run_gc()
    {
        // TO RUN THE GC, WHAT DO WE NEED?
        // WE NEED TO ACCESS THE CALL STACK
        // OF ALL PLATFORM THREAD
        // NOTE THAT, GOATROUTINE ARE NOT GC ROOT
        std::swap(this_half, that_half);
        top = 0;
        // determine root
        for (CallStack& call_stack : call_stacks) {
            inspect_call_stack(call_stack);
        }
    }

    void inspect_call_stack(CallStack& call_stack)
    {
        u64 frame_pointer = call_stack.get_frame_pointer();
        while (true) {
            FrameData frame_data = call_stack.read_frame_data(frame_pointer);
            inspect_frame(call_stack, frame_pointer, frame_data);
            u64 prev_frame_pointer = frame_data.prev_frame_pointer;
            if (frame_pointer == 0 && prev_frame_pointer == 0) {
                break;
            }
            frame_pointer = prev_frame_pointer;
        }
    }

    void inspect_frame(CallStack& call_stack, u64 frame_pointer, FrameData frame_data)
    {
        const Function& function = function_table[frame_data.function_index];
        for (u64 index = 0; index < function.varc; ++index) {
            bool is_ref = function.pointer_map.get(index);
            if (!is_ref) {
                continue;
            }
            // if is ref, do copy gc
            u64 address = call_stack.read_local<u64>(frame_pointer, index);
            u64 new_address = copy(address);
            call_stack.write_local(frame_pointer, index, new_address);
        }
    }

    u64 copy(u64 address)
    {
        u64 block_address = address - block_header_size;
        BlockHeader block_header = read<BlockHeader>(that_half, block_address);

        if (block_header.control_bits & mark_bit) {
            return block_header.forward_pointer;
        }

        u64 new_address = internal_allocate(block_header.type_index, block_header.count);
        block_header.control_bits |= mark_bit;
        block_header.forward_pointer = new_address;
        write(that_half, block_address, block_header);

        const Type& type = type_table[block_header.type_index];
        memcpy(this_half + new_address, that_half + address, type.size * block_header.count);

        if (type.memc == 0) {
            return new_address;
        }

        for (u64 elem_index = 0; elem_index < block_header.count; ++elem_index) {
            for (u64 slot_index = 0; slot_index < type.memc; ++slot_index) {
                bool is_ref = type.pointer_map.get(slot_index);
                if (!is_ref) {
                    continue;
                }
                u64 mem_offset = type.size * elem_index + word_size * slot_index;
                u64 mem_value = read<u64>(that_half, address + mem_offset);
                u64 new_mem_value = copy(mem_value);
                write(this_half, new_address + mem_offset, new_mem_value);
            }
        }

        return new_address;
    }
    */

    Runtime* runtime;
    std::mutex mutex;
    std::unique_ptr<std::byte[]> managed_memory;
    std::byte* this_half;
    std::byte* that_half;
    u64 size;
    u64 top;
};

#endif

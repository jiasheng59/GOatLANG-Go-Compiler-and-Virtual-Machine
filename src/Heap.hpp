#ifndef HEAP_HPP
#define HEAP_HPP

#include <memory>
#include <mutex>

#include "Code.hpp"

struct BlockHeader
{
    u64 control_bits;
    u64 type_index;
    u64 count;
};

class Heap
{
    std::mutex mutex;
    std::unique_ptr<std::byte[]> managed_memory;
    std::byte* this_half;
    std::byte* that_half;
    u64 size;
    u64 top;

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

    Heap(u64 heap_size) : mutex{},
                          managed_memory{std::make_unique<std::byte[]>(heap_size)},
                          this_half{managed_memory.get()},
                          that_half{this_half + heap_size},
                          size{heap_size},
                          top{0}
    {
    }

    template <typename T>
    const T& load(u64 address)
    {
        return read<T>(this_half, address);
    }

    template <typename T>
    void store(u64 address, const T& value)
    {
        write(this_half, address, value);
    }

    BlockHeader& access_block_header(u64 address)
    {
        return read<BlockHeader>(this_half, address - sizeof(BlockHeader));
    }

    u64 allocate(const Type& type, u64 count);
};

#endif

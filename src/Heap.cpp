#include <stdexcept>

#include "Heap.hpp"

u64 Heap::new_block(const Type& type, u64 count)
{
    u64 block_size = sizeof(BlockHeader) + type.size * count;
    if (!enough_space(block_size)) {
        throw std::runtime_error{"out of memory!"};
    }
    u64 address = top + sizeof(BlockHeader);
    BlockHeader block_header = {
        .control_bits = 0,
        .type_index = type.index,
        .count = count,
    };
    {
        std::lock_guard lock{mutex};
        write(this_half, top, block_header);
        top += block_size;
    }
    return address;
}

u64 Heap::allocate(const Type& type, u64 count)
{
    return new_block(type, count);
}

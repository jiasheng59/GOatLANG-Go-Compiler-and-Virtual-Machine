#include "Heap.hpp"
#include "Runtime.hpp"

u64 Heap::new_block(const Type& type, u64 count)
{
    u64 address = top + sizeof(BlockHeader);
    u64 block_size = sizeof(BlockHeader) + type.size * count;
    // if (!enough_space(block_size)) {
    //     // error
    // }
    BlockHeader block_header = {
        default_control_bits,
        default_foward_pointer,
        type.index,
        count
    };
    write(this_half, top, block_header);
    top += block_size;
    return address;
}

u64 Heap::allocate(u64 type_index, u64 count)
{
    auto& type_table = runtime->get_type_table();
    auto& type = type_table[type_index];
    return allocate(type, count);
}

u64 Heap::allocate(const Type& type, u64 count)
{
    return new_block(type, count);
}


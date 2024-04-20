#ifndef CHANNEL_MANAGER_HPP
#define CHANNEL_MANAGER_HPP

#include <deque>
#include <mutex>

#include "BlockingQueue.hpp"

class ChannelManager
{
    std::mutex mutex;
    std::deque<BlockingQueue> channels;

public:
    u64 new_channel(u64 channel_size)
    {
        std::lock_guard lock{mutex};
        u64 channel_index = channels.size();
        channels.emplace_back(channel_size);
        // channels.push_back(BlockingQueue{channel_size});
        return channel_index;
    }

    BlockingQueue& get(u64 channel_index)
    {
        std::lock_guard lock{mutex};
        return channels[channel_index];
    }
};

#endif /* CHANNEL_MANAGER_HPP */

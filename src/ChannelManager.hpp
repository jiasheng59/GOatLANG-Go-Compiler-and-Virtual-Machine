#include <mutex>
#include <vector>

#include "Common.hpp"
#include "BlockingQueue.hpp"

class ChannelManager
{
    std::mutex mutex;
    std::vector<BlockingQueue> channels;
public:
    u64 new_channel(u64 channel_size)
    {
        std::lock_guard lock{mutex};
        u64 channel_index = channels.size();
        channels.emplace_back(channel_size);
        return channel_index;
    }

    BlockingQueue& get(u64 channel_index)
    {
        std::lock_guard lock{mutex};
        return channels[channel_index];
    }
};

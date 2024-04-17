#include <mutex>
#include <vector>

#include "Common.hpp"
#include "BlockingQueue.hpp"

class ChannelManager
{
    std::mutex mutex;
    std::vector<BlockingQueue> channels;
public:
    u64 new_channel(u64 channel_size);
    BlockingQueue& get(u64 channel_index);
};

#include <deque>
#include <mutex>
#include <condition_variable>
#include <cstdint>

#include "Common.hpp"

class BlockingQueue {
    std::deque<u64> content;
    std::size_t capacity;

    std::mutex mutex;
    std::condition_variable not_empty;
    std::condition_variable not_full;
public:
    BlockingQueue() = delete;
    BlockingQueue(const BlockingQueue&) = delete;
    BlockingQueue(BlockingQueue&&) = default;
    BlockingQueue& operator=(const BlockingQueue&) = delete;
    BlockingQueue& operator=(BlockingQueue&&) = default;

    BlockingQueue(std::size_t capacity) : capacity(capacity) {}

    void push(u64 item)
    {
        {
            std::unique_lock<std::mutex> lock(mutex);
            not_full.wait(lock, [this]() { return content.size() < capacity; });
            content.push_back(item);
        }
        not_empty.notify_one();
    }

    bool try_push(u64 item)
    {
        {
            std::unique_lock<std::mutex> lock(mutex);
            if (content.size() == capacity)
                return false;
            content.push_back(item);
        }
        not_empty.notify_one();
        return true;
    }

    void pop(u64 &item)
    {
        {
            std::unique_lock<std::mutex> lock(mutex);
            not_empty.wait(lock, [this]() { return !content.empty(); });
            item = content.front();
            content.pop_front();
        }
        not_full.notify_one();
    }

    bool try_pop(u64 &item)
    {
        {
            std::unique_lock<std::mutex> lock(mutex);
            if (content.empty())
                return false;
            item = content.front();
            content.pop_front();
        }
        not_full.notify_one();
        return true;
    }
};

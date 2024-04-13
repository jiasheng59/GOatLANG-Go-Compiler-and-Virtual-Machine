#include <mutex>

#include "Runtime.hpp"

void Runtime::start()
{
    Thread main_thread{*this};
    main_thread.start(init_function_index);
    std::unique_lock lock{thread_pool_mutex};
    termination_condition.wait(lock, [this]() { return thread_pool.empty(); });
}

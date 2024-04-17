#include <mutex>

#include "Runtime.hpp"

void Runtime::start()
{
    Thread main_thread{*this};
    auto &init_function = function_table[configuration.init_function_index];
    main_thread.get_instruction_stream().jump_to(init_function);
    main_thread.get_call_stack().push_frame(init_function, -1);
    main_thread.start();
    std::unique_lock lock{thread_pool_mutex};
    termination_condition.wait(lock, [this]() { return thread_pool.empty(); });
}

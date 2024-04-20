#include <iostream>

#include "Runtime.hpp"

Runtime::Runtime(
    const Configuration& configuration,
    std::vector<Function>&& function_table,
    std::vector<NativeFunction>&& native_function_table,
    std::vector<std::unique_ptr<Type>>&& type_table,
    StringPool&& string_pool)
    : configuration{configuration},
      function_table{std::move(function_table)},
      native_function_table{std::move(native_function_table)},
      type_table{std::move(type_table)},
      heap{configuration.heap_size},
      string_pool(std::move(string_pool))
{
}

void Runtime::start()
{
    Thread main_thread{*this};
    auto& init_function = function_table[configuration.init_function_index];
    main_thread.get_instruction_stream().jump_to(init_function);
    main_thread.get_call_stack().push_frame(init_function, 0);
    main_thread.start();
    std::unique_lock lock{thread_pool_mutex};
    termination_condition.wait(lock, [this]() { return thread_pool.empty(); });
}

#ifndef RUNTIME_HPP
#define RUNTIME_HPP

#include <condition_variable>
#include <mutex>
#include <unordered_set>
#include <vector>

#include "ChannelManager.hpp"
#include "Code.hpp"
#include "Common.hpp"
#include "Heap.hpp"
#include "StringPool.hpp"
#include "Thread.hpp"

struct Configuration
{
    u64 heap_size;
    u64 call_stack_size;
    u64 operand_stack_size;
    u64 init_function_index;
};

class Runtime
{
public:
    static constexpr u64 channel_type_index = 5;

    Runtime() = default;
    Runtime(const Runtime&) = delete;
    Runtime(Runtime&&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    Runtime& operator=(Runtime&&) = delete;

    Runtime(
    const Configuration& configuration,
    std::vector<Function>&& function_table,
    std::vector<NativeFunction>&& native_function_table,
    std::vector<std::unique_ptr<Type>>&& type_table,
    StringPool&& string_pool);

    std::vector<Function>& get_function_table()
    {
        return function_table;
    }

    std::vector<NativeFunction>& get_native_function_table()
    {
        return native_function_table;
    }

    std::vector<std::unique_ptr<Type>>& get_type_table()
    {
        return type_table;
    }

    Heap& get_heap()
    {
        return heap;
    }

    std::unordered_set<Thread*>& get_thread_pool()
    {
        return thread_pool;
    }

    std::mutex& get_thread_pool_mutex()
    {
        return thread_pool_mutex;
    }

    std::condition_variable& get_termination_condition()
    {
        return termination_condition;
    }

    const Configuration& get_configuration() const
    {
        return configuration;
    }

    ChannelManager& get_channel_manager()
    {
        return channel_manager;
    }

    StringPool& get_string_pool()
    {
        return string_pool;
    }

    void start();

    Configuration configuration;
    std::vector<Function> function_table;
    std::vector<NativeFunction> native_function_table;
    std::vector<std::unique_ptr<Type>> type_table;

    Heap heap;
    ChannelManager channel_manager;
    StringPool string_pool;

    static Configuration default_configuration()
    {
        return Configuration{
            .heap_size = 64 * 1024 * 1024,  // 64 MB
            .call_stack_size = 8 * 1024,    // 8 KB
            .operand_stack_size = 1 * 1024, // 1 KB, 128 values
            .init_function_index = 0,
        };
    }

    std::unordered_set<Thread*> thread_pool;
    std::mutex thread_pool_mutex;
    // main thread can only terminate
    // after all spawned thread terminated
    // this behavior is different compared to Go, but easier to implement
    std::condition_variable termination_condition;
};

/*
class RuntimeBuilder
{
public:
    template<typename T>
    void function_table(T&& new_function_table)
    {
        function_table = std::forward<T>(new_function_table);
    }

    template<typename T>
    void native_function_table(T&& new_native_function_table)
    {
        native_function_table = std::forward<T>(new_native_function_table);
    }

    template<typename T>
    void type_table(T&& new_type_table)
    {
        type_table = std::forward<T>(new_type_table);
    }

    Runtime build()
    {
        return Runtime{};
    }
private:
    Configuration configuration;

    std::vector<Function> function_table;
    std::vector<NativeFunction> native_function_table;
    std::vector<Type> type_table;
};
*/

#endif /* RUNTIME_HPP */

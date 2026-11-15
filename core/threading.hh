#ifndef BBD57BFE_9EA5_4FC3_913E_3BCF317F08AC
#define BBD57BFE_9EA5_4FC3_913E_3BCF317F08AC

enum class task_status {
    ENQUEUED,
    PROCESSING,
    COMPLETED,
    CANCELLED,
};

class Task {
public:
    virtual ~Task(void) = default;
    virtual void process(void) = 0;
    virtual void finalize(void) = 0;

    std::atomic<task_status> status;
    std::future<void> future;
};

namespace vx
{
template<typename T>
concept derived_task = std::derived_from<T, Task>;
} // namespace vx

namespace threading::detail
{
void submit_new(std::unique_ptr<Task>&& task);
} // namespace threading::detail

namespace threading
{
void init(void);
void shutdown(void);
void update(void);
void wait(void);
} // namespace threading

namespace threading
{
template<vx::derived_task T, typename... AT>
requires std::constructible_from<T, AT...>
void submit(AT&&... args);
} // namespace threading

template<vx::derived_task T, typename... AT>
requires std::constructible_from<T, AT...>
void threading::submit(AT&&... args)
{
    detail::submit_new(std::make_unique<T>(std::forward<AT>(args)...));
}

#endif /* BBD57BFE_9EA5_4FC3_913E_3BCF317F08AC */

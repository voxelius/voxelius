#include "core/pch.hh"

#include "core/threading.hh"

#include "core/cmdline.hh"

constexpr static std::string_view DEFAULT_POOL_SIZE_ARG { "4" };
constexpr static std::chrono::milliseconds FINALIZE_BUDGET { 5 };

static std::unique_ptr<BS::light_thread_pool> s_threads;
static std::deque<std::unique_ptr<Task>> s_deque;
static std::deque<std::unique_ptr<Task>> s_finalize_queue;

// A ready future proves the worker left task_process; freeing on the status alone dangles
static bool task_joined(const std::unique_ptr<Task>& task)
{
    if(!task->future.valid()) {
        return true;
    }

    return std::future_status::ready == task->future.wait_for(std::chrono::seconds(0));
}

static void task_process(Task* task)
{
    ZoneScoped;

    task->status.store(task_status::PROCESSING, std::memory_order_relaxed);
    task->process();

    if(task_status::PROCESSING == task->status.load(std::memory_order_acquire)) {
        task->status.store(task_status::COMPLETED, std::memory_order_release);
    }
}

void threading::detail::submit_new(std::unique_ptr<Task>&& task)
{
    task->status.store(task_status::ENQUEUED, std::memory_order_relaxed);
    task->future = s_threads->submit_task(std::bind(&task_process, task.get()));
    s_deque.push_back(std::move(task));
}

void threading::init(void)
{
    auto threads_option = cmdline::value("threads");
    auto argument = threads_option.value_or(DEFAULT_POOL_SIZE_ARG);
    auto num_concurrent_threads = std::thread::hardware_concurrency();
    unsigned thread_pool_size;

    if(num_concurrent_threads && 0 == argument.compare("max")) {
        thread_pool_size = num_concurrent_threads;
    }
    else if(num_concurrent_threads) {
        auto result = std::from_chars(argument.data(), argument.data() + argument.size(), thread_pool_size);

        if(result.ec == std::errc()) {
            thread_pool_size = std::clamp<unsigned>(thread_pool_size, 1U, num_concurrent_threads);
        }
        else {
            thread_pool_size = 4U;
        }
    }
    else {
        auto result = std::from_chars(argument.data(), argument.data() + argument.size(), thread_pool_size);

        if(result.ec == std::errc()) {
            thread_pool_size = std::max<unsigned>(thread_pool_size, 1U);
        }
        else {
            thread_pool_size = 4U;
        }
    }

    msg::log_info("using {} threads for pooling tasks", thread_pool_size);

    s_threads = std::make_unique<BS::light_thread_pool>(thread_pool_size, [](std::size_t idx) {
        auto name = std::format("Worker {}", idx);

        tracy::SetThreadName(name.c_str());
    });

    s_deque.clear();
}

void threading::shutdown(void)
{
    if(s_threads == nullptr) {
        // Entry points stop the pool before the hard purge, so teardown calls this a second time
        return;
    }

    for(auto& it : s_deque) {
        auto status = it->status.load(std::memory_order_relaxed);

        if(status == task_status::ENQUEUED || status == task_status::PROCESSING) {
            it->status.store(task_status::CANCELLED, std::memory_order_relaxed);
        }
    }

    s_threads->purge();
    s_threads->wait();

    s_deque.clear();
    s_finalize_queue.clear();
    s_threads.reset();
}

void threading::update(void)
{
    ZoneScoped;

    auto it = s_deque.begin();

    while(it < s_deque.end()) {
        auto& task = *it;
        auto status = task->status.load(std::memory_order_acquire);

        if(status == task_status::CANCELLED) {
            // A task that cancels itself from process() is still running here
            if(!task_joined(task)) {
                it = std::next(it);
                continue;
            }

            it = s_deque.erase(it);
            continue;
        }

        if(status == task_status::COMPLETED) {
            if(!task_joined(task)) {
                it = std::next(it);
                continue;
            }

            s_finalize_queue.push_back(std::move(task));
            it = s_deque.erase(it);
            continue;
        }

        it = std::next(it);
    }

    auto deadline = std::chrono::steady_clock::now() + FINALIZE_BUDGET;

    while(s_finalize_queue.size()) {
        auto task = std::move(s_finalize_queue.front());

        s_finalize_queue.pop_front();

        task->finalize();

        if(std::chrono::steady_clock::now() >= deadline) {
            break;
        }
    }
}

void threading::wait(void)
{
    for(auto& task : s_deque) {
        if(task->future.valid()) {
            task->future.wait();
        }
    }
}

void threading::cancel_all(void)
{
    if(s_threads == nullptr) {
        return;
    }

    // The pool discards queued work, but the tasks it already picked up still
    // read the old world; join those before the caller frees it
    s_threads->purge();

    wait();

    s_deque.clear();
    s_finalize_queue.clear();
}

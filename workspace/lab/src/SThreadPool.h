#pragma once

#include <assert.h>
#include <deque>
#include <functional>
#include <iostream>
#include <vector>


#include "winapi.h"

// 线程池
namespace SThreadPool {

#define TASK_BUFFER_SIZE 128

static DWORD get_core_number() {
    SYSTEM_INFO sysInfo;
    // 获取系统信息，
    // SYSTEM_INFO结构体中可获取当前系统的逻辑处理器的个数
    GetSystemInfo(&sysInfo);
    return sysInfo.dwNumberOfProcessors;
}

using Task = std::function<void()>;

class ThreadPool {
public:
    ThreadPool(size_t thread_num = 0) {
        thread_num = thread_num == 0 ? get_core_number() : thread_num;
        std::cout << "ThreadPool: " << thread_num << std::endl;
        task_queue_access_mutex = CreateMutex(NULL, false, NULL);
        assert(task_queue_access_mutex != INVALID_HANDLE_VALUE);
        semaphore_task_count = CreateSemaphore(NULL, 0, TASK_BUFFER_SIZE, NULL);
        assert(semaphore_task_count != INVALID_HANDLE_VALUE);
        semaphore_task_available = CreateSemaphore(NULL, TASK_BUFFER_SIZE, TASK_BUFFER_SIZE, NULL);
        assert(semaphore_task_available != INVALID_HANDLE_VALUE);
        task_complete_event = CreateEvent(NULL, true, true, NULL);
        assert(task_complete_event != INVALID_HANDLE_VALUE);

        // 先创建锁再创建线程不能反过来。。。。
        threads.reserve(thread_num);
        for (size_t i = 0; i < thread_num; ++i) {
            HANDLE thread = CreateThread(NULL, 0, thread_cycle, this, 0, NULL);
            assert(thread != INVALID_HANDLE_VALUE);
            threads.push_back(thread);
        }
    }

    // 添加待执行任务并立即开始执行
    void add_task(Task &&task) {
        ResetEvent(task_complete_event);
        // std::cout << "WaitForSingleObject: " << ret << std::endl;
        WaitForSingleObject(semaphore_task_available, INFINITE);
        WaitForSingleObject(task_queue_access_mutex, INFINITE);
        tasks.push_back(std::move(task));
        ReleaseMutex(task_queue_access_mutex);
        ReleaseSemaphore(semaphore_task_count, 1, NULL);
    }

    // 等待所有任务执行完毕
    void wait_for_all_done() {
        WaitForSingleObject(task_complete_event, INFINITE);
        assert(tasks.size() == 0);
    }

    ~ThreadPool() {
        wait_for_all_done();
        for (auto thread : threads) {
            TerminateThread(thread, 0);
            CloseHandle(thread);
        }
        CloseHandle(task_queue_access_mutex);
        CloseHandle(task_complete_event);
        CloseHandle(semaphore_task_available);
        CloseHandle(semaphore_task_count);
    }

private:
    std::vector<HANDLE> threads;
    std::deque<Task> tasks;

    HANDLE task_queue_access_mutex;  // 访问task_queue的锁
    HANDLE task_complete_event;      // 任务执行完毕的事件
    HANDLE semaphore_task_available; // task_queue是否有任务可执行的信号
    HANDLE semaphore_task_count;     // task_queue中任务数量的信号

    static DWORD WINAPI thread_cycle(void *arg) {
        ThreadPool &pool = *(ThreadPool *)arg;
        while (true) {
            WaitForSingleObject(pool.semaphore_task_count, INFINITE);
            WaitForSingleObject(pool.task_queue_access_mutex, INFINITE);

            assert(pool.tasks.size() > 0);
            Task t = pool.tasks.front();
            pool.tasks.pop_front();
            bool is_empty = pool.tasks.empty();
            ReleaseMutex(pool.task_queue_access_mutex);

            t(); // 执行任务

            ReleaseSemaphore(pool.semaphore_task_available, 1, NULL);
            if (is_empty && pool.tasks.empty()) {
                SetEvent(pool.task_complete_event);
            }
        }
        return 0;
    }
};

} // namespace SThreadPool
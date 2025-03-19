#pragma once

#include "torpedo/foundation/DeviceAllocator.h"

#include <plog/Log.h>

#include <mutex>
#include <queue>
#include <thread>

namespace tpd {
    template<typename T> requires std::is_base_of_v<Destroyable, T> && std::is_final_v<T>
    class DeletionWorker final {
    public:
        struct Task {
            vk::Device device;
            vk::Semaphore semaphore;
            vk::Fence fence;
            std::unique_ptr<T, Deleter<T>> resource;
            std::vector<std::pair<vk::CommandPool, vk::CommandBuffer>> buffers;
        };

        DeletionWorker(std::pmr::memory_resource* resourcePool, std::mutex& commandPoolMutex, std::string identifier);

        DeletionWorker(const DeletionWorker&) = delete;
        DeletionWorker& operator=(const DeletionWorker&) = delete;

        void submit(
            vk::Device device, vk::Semaphore semaphore, vk::Fence fence, std::unique_ptr<T, Deleter<T>> resource,
            std::vector<std::pair<vk::CommandPool, vk::CommandBuffer>>&& commandBuffers = {});

        void waitEmpty();

        void shutdown() noexcept;
        ~DeletionWorker() noexcept;

    private:
        void deletionWork();
        std::thread _workerThread;

        std::pmr::deque<Task> _tasks;
        bool _stopWorker{ false };

        std::mutex _queueMutex{};
        std::condition_variable _queueCondition{};

        // According to the spec, the command pool and command buffers involved in vkFreeCommandBuffers
        // must be externally synchronized. While the command buffers are no longer the interest of the
        // main thread, the command pool may still be used by the main thread, hence an extra lock for it
        std::mutex& _commandPoolMutex;

        std::string _identifier;
    };

    template<typename T> requires std::is_base_of_v<Destroyable, T> && std::is_final_v<T>
    DeletionWorker<T>::DeletionWorker(std::pmr::memory_resource* resourcePool, std::mutex& commandPoolMutex, std::string identifier)
        : _workerThread{ std::thread(&DeletionWorker::deletionWork, this) }
        , _tasks{ resourcePool }, _commandPoolMutex{ commandPoolMutex }, _identifier{ std::move(identifier) } {
        PLOGD << "DeletionWorker - Launched 1 deletion worker: " << _identifier;
    }

    template<typename T> requires std::is_base_of_v<Destroyable, T> && std::is_final_v<T>
    void DeletionWorker<T>::submit(
        const vk::Device device,
        const vk::Semaphore semaphore,
        const vk::Fence fence,
        std::unique_ptr<T, Deleter<T>> resource,
        std::vector<std::pair<vk::CommandPool, vk::CommandBuffer>>&& commandBuffers)
    {
        {
            std::lock_guard lock(_queueMutex);
            PLOGD << "DeletionWorker<" << _identifier << "> - Inserting a resource: " << resource.get();
            _tasks.emplace_back(device, semaphore, fence, std::move(resource), std::move(commandBuffers));
        }
        _queueCondition.notify_one();
    }

    template<typename T> requires std::is_base_of_v<Destroyable, T> && std::is_final_v<T>
    void DeletionWorker<T>::waitEmpty() {
        if (_workerThread.joinable()) {
            std::unique_lock lock(_queueMutex);
            _queueCondition.wait(lock, [this] { return _tasks.empty(); });
        }
    }

    template<typename T> requires std::is_base_of_v<Destroyable, T> && std::is_final_v<T>
    void DeletionWorker<T>::shutdown() noexcept {
        if (_workerThread.joinable()) {
            {
                std::lock_guard lock(_queueMutex);
                _stopWorker = true;
            }
            _queueCondition.notify_one();
            _workerThread.join();
            PLOGD << "DeletionWorker - Shut down 1 deletion worker: " << _identifier;
        }
    }

    template<typename T> requires std::is_base_of_v<Destroyable, T> && std::is_final_v<T>
    DeletionWorker<T>::~DeletionWorker() noexcept {
        shutdown();
    }

    template<typename T> requires std::is_base_of_v<Destroyable, T> && std::is_final_v<T>
    void DeletionWorker<T>::deletionWork() {
        while (true) {
            // Sleep until there a deletion task to work on, or we're shutting down
            std::unique_lock lock(_queueMutex);
            _queueCondition.wait(lock, [this] { return !_tasks.empty() || _stopWorker; });

            if (_stopWorker && _tasks.empty()) break;

            const auto [device, semaphore, fence, resource, buffers] = std::move(_tasks.front());
            _tasks.pop_front();
            lock.unlock();  // shared-data has been read, we can unlock to let other threads add more tasks

            // Wait until the GPU is done with the resource then destroy it
            using limits = std::numeric_limits<uint64_t>;
            // The fence has been submitted by the main thread prior to this point, so no sync needed
            [[maybe_unused]] const auto result = device.waitForFences(fence, vk::True, limits::max());
            {
                // VkCommandPool in vkFreeCommandBuffers must be synchronized externally
                // The same is applied to buffers, but the main thread no longer uses them
                std::lock_guard poolLock(_commandPoolMutex);
                for (const auto [pool, buffer] : buffers) device.freeCommandBuffers(pool, buffer);
            }
            // A Destroyable uses VMA allocator internally, which is thread-safe
            resource->destroy();
            // The destruction of semaphore and fence are thread-safe
            if (semaphore)
                device.destroySemaphore(semaphore);
            if (fence) [[likely]]
                device.destroyFence(fence);

            PLOGD << "DeletionWorker<" << _identifier << "> - Destroyed a resource: " << resource.get();

            // Notify the main thread who could potentially be waiting until all tasks are free
            // If that's not the case, this signal can be lost without causing any issue
            _queueCondition.notify_one();
        }
    }
}

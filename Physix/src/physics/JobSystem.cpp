// See JobSystem.h for what this class is (a persistent worker-thread pool
// with a single generation-counter-barrier entry point, RunParallelFor);
// this file is the implementation.
#include "JobSystem.h"

JobSystem::JobSystem(int numWorkers) : numWorkers(numWorkers) {
    workers.reserve(numWorkers);
    for (int i = 0; i < numWorkers; i++) {
        workers.emplace_back(&JobSystem::WorkerLoop, this, i);
    }
}

JobSystem::~JobSystem() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        shuttingDown = true;
        generation++;
    }
    startCv.notify_all();
    for (std::thread& worker : workers) {
        worker.join();
    }
}

void JobSystem::RunParallelFor(const std::function<void(int)>& task) {
    std::unique_lock<std::mutex> lock(mutex);
    currentTask = &task;
    pendingWorkers = numWorkers;
    generation++;
    lock.unlock();
    startCv.notify_all();

    lock.lock();
    doneCv.wait(lock, [this] { return pendingWorkers == 0; });
    currentTask = nullptr;
}

void JobSystem::WorkerLoop(int workerId) {
    uint64_t seenGeneration = 0;
    while (true) {
        std::unique_lock<std::mutex> lock(mutex);
        startCv.wait(lock, [this, seenGeneration] { return generation != seenGeneration; });
        seenGeneration = generation;
        if (shuttingDown) {
            return;
        }
        const std::function<void(int)>* task = currentTask;
        lock.unlock();

        (*task)(workerId);

        lock.lock();
        bool isLastWorker = (--pendingWorkers == 0);
        lock.unlock();
        if (isLastWorker) {
            doneCv.notify_one();
        }
    }
}

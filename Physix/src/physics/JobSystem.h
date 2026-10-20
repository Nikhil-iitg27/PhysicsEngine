#pragma once
#include <thread>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <cstdint>

// A fixed-size pool of persistent worker threads used to run the same
// per-frame parallel-for pattern PhysicsWorld needs -- split a fixed number
// of independent work items across worker threads, block until all finish --
// without paying OS thread-creation/destruction cost every frame (previously
// PhysicsWorld::Step constructed and joined a fresh std::vector<std::thread>
// every sub-step).
//
// Deliberately not a general task queue or thread pool (no arbitrary task
// submission, no futures, no work-stealing) -- RunParallelFor is the only
// entry point, matching PhysicsWorld's actual usage: call once per sub-step
// with a lambda, wait for the whole batch, then continue. Pool size is fixed
// at construction; there is no resize, since a fixed number of workers is
// exactly what makes the barrier synchronization below correct and simple --
// see PhysicsWorld's removal of its old setNumThreads for the corresponding
// decision on the caller side.
//
// Not reentrant: do not call RunParallelFor concurrently from multiple
// threads, and do not call it from inside a task passed to RunParallelFor.
class JobSystem {
public:
    explicit JobSystem(int numWorkers);
    ~JobSystem();

    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    // Runs task(workerId) once for each workerId in [0, numWorkers), across
    // the pool's persistent worker threads, and blocks the calling thread
    // until every worker has finished this round.
    void RunParallelFor(const std::function<void(int)>& task);

    int getNumWorkers() const { return numWorkers; }

private:
    int numWorkers;
    std::vector<std::thread> workers;

    std::mutex mutex;
    std::condition_variable startCv;
    std::condition_variable doneCv;

    const std::function<void(int)>* currentTask = nullptr;
    // Incremented once per RunParallelFor call (and once more on shutdown) so
    // each worker's wait-predicate ("has a new round started?") is a simple
    // generation-counter comparison instead of a bare bool, which would be
    // ambiguous about whether a given worker already handled the current round.
    uint64_t generation = 0;
    int pendingWorkers = 0;
    bool shuttingDown = false;

    void WorkerLoop(int workerId);
};

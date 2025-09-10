
#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>
#include <functional>
#include <chrono>
#include <random>
using namespace std::literals;



/*
 * Thread Pool with Work Stealing
 *
 * */

/*
 * Long-running Task
 *
 * - One task takes a long time to perform
 *      - It cannot execute the following tasks in the queue until this task completes
 *      - The following tasks will be delayed
 *
 * - Other threads have no work to do
 *      - Inefficient use of resources
 *      */

/*
 * Work Stealing
 *
 * - A refinement of work sharing
 * - If a thread's queue is empty, the thread "steals" a task
 *      - The thread takes a task from another thread's queue
 * - This ensures that threads are never idle
 *      - Provided there is enough work for all the threads
 * - Can perform better than work sharing
 *      - if some tasks take much longer than other
 *      */

/* Work Stealing Strategy
 *
 * - If a thread's queue is empty
 *      - Do not wait for a task to arrive on the queue
 *      - Choose another thread's queue at random
 *      - If there is a task on that queue, pop it and execute it
 *      - Otherwise, choose a different thread's queue at random
 *      - Continue until it finds a task to perform
 *
 * - If all the queues are empty
 *      - Pause for a while
 *      - Then repeat the process
 *
 * */

/*
 * Work Stealing Implementation
 * - Add non-blocking functions to the queue
 *      - Return immediately if they cannot obtain a lock
 *      - try_pop() returns immediately if the queue is empty
 *      - try_push() returns immediately if the queue is full
 *
 * - The thread pool uses these non-blocking functions
 *      - worker() tries to find a queue where try_pop() succeeds
 *      - submit() tries to find a queue where try_push() succeeds
 *      */

/*
 * Concurrent Queue Member Functions
 *
 * - try_pop()
 *      - Lock the mutex with a time-out
 *      - Call pop() if able to lock
 *
 *              bool try_pop(T &value) {
 *                  // Lock the mutex with a time-out
 *                  std::unique_lock<std::timed_mutex> lck_guard(mut, std::defer_lock);
 *
 *
 *                  // Cannot lock - return immediately
 *                  if(!lck_guard.try_lock_for(1ms) || que.empty()) {
 *                      return false;
 *                  }
 *
 *                  // Locked - remove front element from the queue
 *                  value = que.front();
 *                  que.pop();
 *                  return true;
 *              }
 *              */

/*
 * Concurrent Queue Member Functions
 *
 * - try_push()
 *      - Lock the mutex with a time-out
 *      - Call push() if able to lock
 *
 *          bool try_push(T value) {
 *              // Lock the mutex with a time-out
 *              std::unique_lock<std::timed_mutex>lck_guard(mut, std::defer_lock);
 *
 *              // Cannot lock - return immediately
 *              if(! lck_guard.try_lock_for(1ms) || que.size() > max) {
 *                  return false;
 *              }
 *
 *              // Locked - add the element to the queue
 *              que.push(value);
 *              return true;
 *          }
 *          */


/*
 * - Random number engine as member
 * - Private helper function which returns a random number
 *      - This number will be an index into the array of queues
 *
 *          class thread_pool {
 *              // Random number engine
 *              std::mt19937 mt;
 *
 *              //....
 *
 *              // Returns a random number between 0 and thread_count - 1
 *              int get_random();
 *          };
 *          */

/*
 * Member Functions
 *
 * - get_random()
 * - Needs to lock a mutex
 *      - Many random number generators use shared state
 *
 *
 *          // Returns a random number between 0 and thread_count-1
 *
 *          int thread_pool::get_random()
 *          {
 *               std::lock_guard<std::mutex>lck_guard(rand_mut);
 *               std::uniform_int_distribution<int> dist(0, thread_count-1);
 *               return dist(mt);
 *           }
 *           */

/*
 *
 * - MemBER Functions
 *
 * - submit()
 *      - Chooses a queue at random
 *      - Calls try_push() on that queue
 *      - Repeat with different queues until the call succeeds
 *
 *          // Choose a thread's queue and add a task to it
 *          void thread_pool::submit(Func func) {
 *              int i;
 *
 *              // Pick a queue at random
 *              do {
 *                  i = get_random();
 *              }
 *              // Until we find one that is not full
 *              while(!work_queues[i].try_push(func));
 *          }
 *          */

/*
 *  Member Functions
 *
 *  - worker()
 *          - Try our own queue first
 *          - if try_pop() fails, pick another queue at random
 *
 *          // Number of queues we have checked so far
 *          int visited = 0;
 *
 *          // Take a task function off our queue
 *          int i = idx;
 *
 *          while(!work_queues[i].try_pop(task)) {
 *              // Nothing oon this queue. Pick another at random
 *              i = get_random();
 *
 *              //...
 *          }
 *          */

/*
 * Member Functions
 *
 * - worker()
 *      - if all queues are empty, stop looking for a short time
 *
 *          while(! work_queues[i].try_pop(task)) {
 *              // ...
 *
 *              // Hot loop avoidance
 *              // if we have checked "enough" queues, pause for a while
 *              // then start again with our own queue
 *
 *              if(++visited == thread_count) {
 *                  std::this_thread::sleep_for(10ms);
 *                  visited = 0;
 *                  i = idx;
 *              }
 *          }
 *          */

template<class T>
class Concurrent_Queue{
private:
    std::timed_mutex mut;
    int max {50};
    std::queue<T> queue;

public:
    Concurrent_Queue(const Concurrent_Queue &source) = delete;
    Concurrent_Queue &operator=(const Concurrent_Queue &source) = delete;
    Concurrent_Queue(Concurrent_Queue &&source) = delete;
    Concurrent_Queue &operator=(Concurrent_Queue &&source) = delete;


    Concurrent_Queue() = default;


    bool try_push(T value) {
        // Lock the mutex with a timeout
        std::unique_lock<std::timed_mutex> lck_guard(mut, std::defer_lock);

        // Cannot lock - return immediately
        if (!lck_guard.try_lock_for(1ms) || queue.size() > max) {
            return false;
        }

        // Locked - add the element to the queue
        queue.push(value);

        return true;

    }

    bool try_pop(T &value) {
        // Lock the mutex with a time-out
        std::unique_lock<std::timed_mutex> lck_guard(mut, std::defer_lock);

        // Cannot lock - return immediately
        if (!lck_guard.try_lock_for(1ms) || queue.empty()) {
            return false;
        }

        // Locked - remove front element from the queue
        value = std::move(queue.front());
        queue.pop();

        return true;
    }
};

// Type alias to simplify the code
// All the task functions will have this type
using Func = std::function<void()>;

using Queue = Concurrent_Queue<Func>;

class Thread_Pool {
    // Random number engine
    std::mt19937 mt;

    // Each thread has its own queue of task functions
    std::unique_ptr<Queue []> work_queues;

    // Vector of thread objects which make up the pool
    std::vector<std::thread> threads;

    // Returns a random number between 0 and thread_count
    int get_random()
    {
        std::lock_guard<std::mutex> lck_guard(rand_mut);
        std::uniform_int_distribution<int> dist(0, thread_count -1);
        return dist(mt);
    }


    // The number of threads in the pool
    int thread_count;

    // Index into the vector of queues
    int pos{0};

public:

    // Protect shared state in random number engine
    std::mutex rand_mut;

    // Constructor
    Thread_Pool()
    {
        thread_count = std::thread::hardware_concurrency() - 1;
        std::cout << "Creating a thread pool with " << thread_count << " threads" << std::endl;

        // Create a dynamic array of queues
        work_queues = std::make_unique<Queue []>(thread_count);
        // Start the threads
        for (int i = 0; i < thread_count; ++i)
            threads.push_back(std::thread{&Thread_Pool::worker, this, i});
    }

    // Destructor
    ~Thread_Pool() {
        // Wait for the threads to finish
        for (auto &thr: threads) {
            thr.join();
        }
    }

    // Entry point function for the threads
    void worker(int idx) {
        while(true) {
            // Number of queues we have checked so far
            int visited = 0;
            Func task;

            // Take a task function off our queue
            int i = idx;
            while (!work_queues[i].try_pop(task)) {
                // Nothing on this queue
                // Pick another queue at random
                i = get_random();

                // Hot loop avoidance
                // If we have checked "enough" queues, pause for a while
                // then start again with our own queue
                if (++visited == thread_count) {
                    std::this_thread::sleep_for(10ms);
                    visited = 0;
                    i = idx;
                }
            }

            // Invoke the task function
            task();
        }
    }

    // Choose a thread's queue and add a task to it
    void submit(Func func) {

        int i;

        // Pick a queue at random
        do {
            i = get_random();
        }
        // Until we find one that is not full
        while (!work_queues[i].try_push(func));
    }
};



using namespace std::literals;

// A task function
void task() {
    std::cout << "Thread id: " << std::this_thread::get_id() << " starting a task " << std::endl;
    std::this_thread::sleep_for(100ms);
    std::cout << "Thread id: " << std::this_thread::get_id() << " Finishing a task " << std::endl;
}

void task2() {
    std::cout << "Thread id: " << std::this_thread::get_id() << " starting a task " << std::endl;
    std::this_thread::sleep_for(5s);
    std::cout << "Thread id: " << std::this_thread::get_id() << " finishing a task" << std::endl;
}

int main() {


    // Create the Thread Pool
    Thread_Pool pool;

    // Send some tasks to the thread pool
    for (int i = 0; i < 200; ++i)
        pool.submit(task);

    pool.submit([&pool](){
        std::this_thread::sleep_for(1s);
        std::cout << "All tasks completed" << std::endl;
    });
    return 0;
}

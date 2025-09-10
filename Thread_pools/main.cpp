#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>
#include <functional>

/*
 * Thread Creation Overhead
 *
 * - Creating a thread requires a lot of work
 *      - Create an execution stack for the thread
 *      - Call a system API
 *      - The operating system creates internal data to manage thread
 *      - The scheduler executes the thread
 *      - A context switch occurs to run the thread
 *
 * - Creating a new thread can take 10,000 times as long as calling a function directly
 * - Is there any way we can "recycle" threads?
 *      - Reduce or avoid this overhead
 *      */

/*
 * Thread Pool Motivation
 * - We want to make full use of all our processor cores
 * - Every core should be running one of our threads
 *      - Except perhaps for main()
 *      - And the operating system
 * - Difficult to achieve with std::async()
 *      - Need to keep track of the number of threads started
 *      */

/*
 * Typing Pool
 *
 * - Before computers, all business correspondents had to be typed
 *      - Senior managers had a dedicated secretary
 *      - Other employees used a typing pool
 *      */

/*
 * Typing Pool
 * - Each typist is working on a letter
 * - A new work item arrives
 *      - It is added to a pile of pending work
 *      - A typist becomes free
 *      - The typist takes the next item and starts working on it
 *      */

/*
 * Thread Pool Structure
 * - Container of C++ thread objects
 *      - Has a fixed size
 *      - Usually matched to the number of cores on the machine
 *      - Found by calling std::thread::hardware_concurrency()
 *      - Subtract 2 from the result (main thread + OS)
 *
 *
 * - A queue of tasks
 *      - A thread takes a task off the queue
 *      - It performs the task
 *      - Then it takes the next task from the queue
 * - Tasks represented as callable objects
 *
 * */

/*
 * Advantages of Thread Pool
 *
 * - No scaling concerns
 *      - The thread pool will automatically use all the available cores
 *
 * - Makes efficient use of resources
 *      - Threads are always busy
 *      - Provided there is work for them to do
 *
 * - Works best with short, simple tasks where
 *      - The time taken to create a thread causes a significant delay
 *      - The task does not block
 *      */

/*
 * Disadvantages of thread Pool
 * - Requires a concurrent queue or similar
 *      - Not directly supported in C++
 * - Overhead
 *      - Must add and remove task functions in a thread-safe-way
 *      */

/*
 * Basic Implementation of Thread Pool
 *
 * - The thread_pool class will contain
 *      - A vector of std::thread objects
 *      - A concurrent queue to store incoming tasks as callable objects
 *
 * - Each thread will execute as an infinite loop
 *      - Call pop() on the queue
 *      - Invoke the task returned by pop()
 *      - Call pop() on the queue again
 *      */

/*
 * Class definition
 *
 * // Type alias to simplify the code
 * // All the task functions will have this type
 *
 *          using Func = std::function<void()>;
 *
 *          class thread_pool{
 *              concurrent_queue<Func>work_queue; // Queue of tasks
 *              std::vector<std::thread> threads; // Vector of thread objects
 *              void worker(); // Their entry point function
 *
 *          public:
 *              // Add a task to the queue
 *              void submit(Func func);
 *          };
 *          */


/*
 * Member functions
 *
 *      // Entry point function for the threads
 *      void thread_pool::worker() {
 *          while(true) {
 *              Func task;
 *              work_queue.pop(task);
 *              task();
 *          }
 *      }
 *
 *      // Add a task to the queue
 *      void thread_pool::submit(Func func) {
 *          work_queue.push(func);
 *      }
 *      */

/*
 * Thread_pool's Constructor
 *
 * - Populates a vector of std::thread objects
 * - These have worker() as their entry point function
 *
 *          // Start threads
 *          thread_pool::thread_pool() {
 *              for(int i = 0; i < thread_count; ++i)
 *                  threads.push_back(std::thread(&thread_pool::worker, this));
 *          }
 *          */

/*
 * thread_pool's Destructor
 *
 * - Calls join() on all the threads
 *
 *          // Wait for the threads to finish
 *          thread_pool::~thread_pool() {
 *              for(auto &thr:threads)
 *                  thr.join();
 *          }
 *          */


/*
 * thread_pool Usage
 *
 * - Create an Object of the class
 * - Call submit() with a suitable task function as argument
 *
 *          void task() {
 *              ...
 *          }
 *          thread_pool pool;
 *          pool.submit(task);
 *          */

/*
 * Thread Pool with Multiple Queues
 *
 * - The queue can become a bottle-neck
 *      - When a thread takes a task off the queue, it locks the queue
 *      - Other threads are blocked until this operation  is complete
 *      - if there are many small tasks, this can affect performance
 *
 * - An alternative is to use a separate queue for each thread
 *      - A thread never has to wait to get its next task
 *      - Uses more memory
 *      */

/*
 * Work Sharing
 *
 * - Can perform better than single-queue pool
 *      - When there are many small tasks
 * - If a thread's queue is empty, the thread is idle
 * */

/*
 * Work Sharing Implementation
 * - Replace the queue by a fixed-size vector of queues
 *      - One element for each thread
 * - "Round-robin" scheduling
 *      - Put a new task on the next thread's queue
 *      - After the last element of the vector, go to the front element
 *      */

/*
 * Class Definition
 * - We add some new members
 *      // Alias for concurrent queue type
 *      using Queue = concurrent_queue<func>;
 *
 *      class thread_pool {
 *
 *      ...
 *
 *      // Each thread has its own queue of task functions
 *      // The best way tio create a fixed size vector is to use a unique pointer with the array version
 *      std::unique_ptr<Queue []> work_queues;
 *
 *      // Index into the vector of queue
 *      int pos{0};
 *
 *      };
 *      */

/*
 * Member functions
 *
 * - worker() removes a task from the current threads's queue
 *          void thread_pool::worker(int idx) {
 *              while(true) {
 *                  Func task;
 *
 *                  // Take a task function off the queue
 *                  work_queues[idx].pop(task);
 *
 *                  // Invoke it
 *                  task();
 *              }
 *          }
 *          */

/*
 * Member Functions
 *
 * - submit() adds a task to the current thread's queue
 *          void thread_pool::submit(Func func) {
 *              std::lock_guard<std::mutex> lck_guard(pos_mut);
 *
 *              work_queues[pos].push(func);
 *
 *              // Advance to the next thread's queue
 *              pos = (pos + 1)% thread_count;
 *              */

/*
 * Constructor
 *
 * - The constructor populates a vector of queues
 *      - One for each thread
 *          // Alias for concurrent queue type
 *          using Queue = concurrent_queue<Func>;
 *
 *          // Create a dynamic array of queues
 *          work_queues = std::make_unique<queue []>(thread_count);
 *          */

template<class T>
class Concurrent_Queue{
private:
    std::mutex mut;
    std::condition_variable not_full;
    std::condition_variable not_empty;
    int max {50};
    std::queue<T> queue;

public:
    Concurrent_Queue(const Concurrent_Queue &source) = delete;
    Concurrent_Queue &operator=(const Concurrent_Queue &source) = delete;
    Concurrent_Queue(Concurrent_Queue &&source) = delete;
    Concurrent_Queue &operator=(Concurrent_Queue &&source) = delete;


    explicit Concurrent_Queue(int max): max{max} {};

    Concurrent_Queue() : max{50} {};

    void push(T value) {
        std::unique_lock<std::mutex> uniq_lck(mut);
        not_full.wait(uniq_lck, [this]{return queue.size() < max;});
        queue.push(std::move(value));
        // We just produced an item, hence, wake up consumer
        not_empty.notify_one();
    }

    void pop(T &value){
        std::unique_lock<std::mutex> uniq_lck(mut);
        not_empty.wait(uniq_lck, [this]{return !queue.empty();});
        value = std::move(queue.front());
        queue.pop();
        // We just consumed an item(freed space), hence, wake up producer
        not_full.notify_one();
    }
};

// Type alias to simplify the code
// All the task functions will have this type
using Func = std::function<void()>;

using Queue = Concurrent_Queue<Func>;

class Thread_Pool {
    // Each thread has its own queue of task functions
    std::unique_ptr<Queue []> work_queues;

    // Vector of thread objects which make up the pool
    std::vector<std::thread> threads;

    // Entry point function for the threads
    void worker(int idx) {
        while(true) {
            Func task;

            // Take a task function off the queue
            work_queues[idx].pop(task);

            // Invoke it
            task();
        }
    }

    // The number of threads in the pool
    int thread_count;

    // Index into the vector of queues
    int pos{0};

public:

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

    void submit(Func func) {
        work_queues[pos].push(func);

        // Advance to the next thread's queue
        pos = (pos + 1) % thread_count;
    }
};



using namespace std::literals;

// A task function
void task() {
    std::cout << "Thread id: " << std::this_thread::get_id() << " starting a task " << std::endl;
    std::this_thread::sleep_for(100ms);
    std::cout << "Thread id: " << std::this_thread::get_id() << " Finishing a task " << std::endl;
}
int main() {

    // Create the Thread Pool
    Thread_Pool pool;

    // Send some tasks to the thread pool
    for (int i = 0; i < 20; ++i)
        pool.submit(task);

    pool.submit([&pool](){
        std::this_thread::sleep_for(1s);
        std::cout << "All tasks completed" << std::endl;
    });


    return 0;
}

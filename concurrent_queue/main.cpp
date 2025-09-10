#include <iostream>
#include <mutex>
#include <queue>
#include <future>
#include <string>
#include <thread>
#include <condition_variable>
/*
 * Queue
 * - FIFO data structure
 * - Stores elements in the order they were inserted
 *      - Elements are "pushed" onto the back of the queue
 *      - Elements are "popped" off the front
 *      */

/*
 * std::queue Limitations
 *
 * - Implemented as a "memory location"
 * - If pop() is called on an empty container, the behaviour is undefined
 * - Removing an element involves two operations
 *      - front() returns a reference to the element at the front
 *      - pop() removes the element at the front, without returning anything
 *
 * - Intended to provide exception safety
 *      - But removes thread safety
 *      - Race condition between front() and pop()
 *      */

/*
 * Concurrent Queue
 *
 * - We will write a wrapper class
 *      - std::queue object is class member
 *      - std::mutex is class member
 *
 * - Same interact as std::queue
 *      - Will omit "uninteresting" member functions
 *
 * - Each member functions locks the mutex
 *      - Then calls the corresponding member function of std::queue
 *      - Arguments will be forwarded
 *      */

/*
 * Concurrent Queue Special Member Functions
 *
 * - No requirement to copy or move objects of this class
 * - We will delete the copy and move operators
 * */

/*
 * Concurrent Queue Member Functions
 *
 * - Push()
 *      - Lock the mutex
 *      - Call std::queue's push() with its argument
 *      - Unlock the mutex
 *
 * - Pop()
 *      - Lock the mutex
 *      - Call std::queue's front()
 *      - Copy the returned value into its argument
 *      - Call std::queue's pop()
 *      - Unlock the mutex
 *      - Do something sensible if the queue is empty
 *      */

/*
 * concurrent_queue Class
 *
 *      template<class T>
 *
 *      class Concurrent_Queue {
 *          // Data members
 *          std::mutex mut;
 *          std::queue<T> que;
 *
 *      public:
 *          Concurrent_Queue() = default;
 *
 *          // Deleted special member functions
 *          Concurrent_Queue(const Concurrent_Queue &source) = delete;
 *          Concurrent_Queue(Concurrent_Queue &&source) = delete;
 *          Concurrent_Queue &operator=(const Concurrent_Queue &source) = delete;
 *          Concurrent_Queue &operator=(Concurrent_Queue &&source) = delete
 *          */

/*
 * Concurrent_Queue Class
 *
 *          // Member functions
 *          // Push an element onto the queue
 *
 *
 *          void push(T value) {
 *              std::lock_guard<std::mutex> lck_guard(mut);
 *              queue.push(value);
 *          }
 *
 * */

/*
 *
 *          // Pop an element from the queue
 *
 *          void pop(T& value) {
 *              std::unique_lock<std::mutex> lck_guard(mut);
 *
 *              if(que.empty()) {
 *                  throw Concurrent_Queue_exeception("Queue is empty");
 *              }
 *
 *              value = q.front()
 *              q.pop()
 *          }
 *      };
 *      */

/*
 * Concurrent Queue with Condition Variable
 *
 * Concurrent Queue
 * - The queue we have so far is safe
 *      - pop() throws an exception when the queue is empty
 *      - push() throws an exception when the queue is full
 *
 * - A more useful approach
 *      - pop() waits until there is some data in the queue
 *      - push() waits until the queue is no longer full
 *      */

/*
 * Concurrent Queue with Condition Variable
 *
 * - We can do this with a condition variable
 *      - The thread that calls pop() calls wait() on the condition variable
 *      - The thread that calls push() notifies the condition variable
 *
 * - We add a predicate to the wait() call
 *      - If the queue is empty, we continue waiting
 *      - If it is not empty, then it is safe to continue and pop the queue
 *      - Avoids spurious and lost wakeups
 *      */

/*
 * Concurrent_queue with Condition Variable pop function
 *
 *      void pop(T &value) {
 *          std::unique_lock<std::mutex> lck_guard(mut);
 *
 *          // Block when the queue is empty
 *          cv.wait(lck_guard, [this] {return !que.empty();});
 *
 *          // Perform the pop
 *          value = que.front();
 *          que.pop();
 *      }
 *      */

/*
 * Concurrent_Queue with Condition Variable push function
 *
 *          void push(T value) {
 *              std::lock_guard<std::mutex>lck_guard(mut);
 *
 *              // if the queue is full, wait and try again
 *              while(que.size() > max) {
 *                  uniq_lck.unlock();
 *                  std::this_thread::sleep_for(50ms);
 *                  uniq_lck.lock();
 *              }
 *
 *              // Perform the push and notify
 *              que.push(value);
 *              cv.notify_one();
 *          }
 *          */


/*
 * Conclusion
 *
 * - This is a simple concurrent queue
 * - it employs "coarse_grained" locking
 *      - Only one thread can access the queue at any one time
 *      - In effect, the program becomes single-threaded
 * - Adding the condition variable improves this slightly
 *      - If the queue is empty and a thread is trying to pop()
 *      - Other threads can run, until the queue is no longer empty
 * - A lock-free solution would be more efficient
 *      - But more complex
 *      */


template<class T>
class Concurrent_Queue{
    std::mutex mut;
    std::queue<T> queue;
    int max{60};
    std::condition_variable cond_var;
    std::condition_variable not_full;
    std::condition_variable not_empty;

public:
    // Delete copy and move member functions

    Concurrent_Queue(const Concurrent_Queue &source) = delete;
    Concurrent_Queue &operator=(const Concurrent_Queue &source) = delete;
    Concurrent_Queue(Concurrent_Queue &&source) = delete;
    Concurrent_Queue &operator=(Concurrent_Queue &&source) =delete;

    // constructor
    Concurrent_Queue() = default;
    Concurrent_Queue(int max) : max{max} {};

    // Member functions
    // Push an element onto the queue

    void push( T value) {
        std::unique_lock<std::mutex> uniq_lck(mut);
        not_full.wait(uniq_lck, [this]{return queue.size() < max;});
        queue.push(value);
        cond_var.notify_one();
    }

    void pop(T &value) {
        std::unique_lock<std::mutex> uniq_lck(mut);
        not_empty.wait(uniq_lck, [this]{return !queue.empty();});
        value = queue.front();
        queue.pop();
        cond_var.notify_one();
    }

    class Concurrent_Queue_Empty : public std::runtime_error{
    public:
        Concurrent_Queue_Empty() : std::runtime_error("Queue is empty") {}
        Concurrent_Queue_Empty(const char *s) : std::runtime_error(s){}
    };

    class Concurrent_Queue_Full : public std::runtime_error{
    public:
        Concurrent_Queue_Full() : std::runtime_error("Queue is full") {}
        explicit Concurrent_Queue_Full(const char *s) : std::runtime_error(s){}
    };

};

// Shared queue object
Concurrent_Queue<std::string> conc_que;

// Waiting thread

void reader() {
    using namespace std::literals;
    std::this_thread::sleep_for(2s); // Pretend to be busy
    std::string sdata;

    // Pop some elements from the queue
    std::cout << "Reader calling pop....." << std::endl;
    for (int i = 0; i < 60; ++i) {
        conc_que.pop(sdata); // Pop the data off the queue
        std::cout << "Reader received data: " << sdata << std::endl;
    }
}

// Modifier thread
void writer() {
    // Push the data onto the queue
    for (int i = 0; i < 60; ++i) {
        std::string sdata = "Item" + std::to_string(i);
        conc_que.push(sdata);
    }

    std::cout << "Writer returned from push...." << std::endl;
}

int main() {
    std::cout << "Hello, World!" << std::endl;

    // Start the threads
    auto write_fut = std::async(std::launch::async, writer);
    auto read_fut = std::async(std::launch::async, reader);


    // Wait for them to complete
    try {
        read_fut.get();
    }
    catch(std::exception &e) {
        std::cout << "Exception caught: " << e.what() << std::endl;
    }
    try {
        write_fut.get();
    }
    catch(std::exception &e) {
        std::cout << "Exception caught: " << e.what() << std::endl;
    }

    return 0;
}

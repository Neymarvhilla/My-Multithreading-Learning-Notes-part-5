#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <memory>
// conflicting operations on STL containers are not safe
// They must be synchronized



/*
 * Data Structures and Concurrency
 *
 * - A data structure has multiple elements
 *      - Multiple threads may access these elements
 *      - These accesses can conflict
 *      - Locks or atomic operations may be needed
 *      */


/*
 * Modifying Operations
 * - May affect other parts of the object
 * - Linked list
 *      - Data stored in a chain of nodes
 *      - Adding or removing an element modifies the surrounding nodes
 * - Vector/string/dynamic array
 *      - Data stored in memory block
 *      - Adding or removing elements moves the following element in memory
 *      - Adding elements may cause the block to be reallocated
 * - if other threads are accessing those elements
 *      - Pointers and references may "dangle"
 *      - Iterators may become invalidated
 *      */

/*
 * Basic Thread Safety Guarantee
 *
 * - STL containers are "memory objects":
 * - Concurrent reads of the same object are safe
 *      - Many threads can read from object X
 * - A single write of an object is safe
 *      - One thread can write to object X
 *      - Provided no other threads access X concurrently
 * - Concurrent reads and writes of the same object are not safe
 *      - One thread writes to object X
 *      - Other threads must not access X during this operation
 *      */

/*
 * Coarse-grained Locking
 *
 * - Locks the entire object
 *      - Easy to do
 *      - Requires no change to the data structure
 *
 * - Sometimes the only option
 *      - A variable of built-in type
 *      - Types in the C++ Standard library
 *      - Types provided by other programmers, if we cannot modify them
 *
 * - In effect, all code that accesses the object will be single-threaded
 *      - Serial code
 *      */

/*
 * Fine-grained Locking
 *
 * - We can choose which parts of the object to lock
 *      - We only lock some of the elements
 *
 * - This is known as "fine-grained locking"
 *      - Allows concurrent access
 *      - Requires writing extra code
 *      - Requires careful design
 *      - Increases cost of creating an object(mutex initialization)
 *      */

/*
 * std::shared_ptr
 *
 * - std::shared_ptr was introduced in C++11
 *      - Different instances can share the same memory allocation
 *      - It uses reference counting
 *
 * - When a shared_ptr object is copied or assigned
 *      - There are no memory operations
 *      - Instead, the reference counter is incremented
 *      - When a copy is destroyed, the counter is decremented
 *      - When the last copy is destroyed, the counter is equal to zero
 *      - The allocated memory is then released.
 *      */

/*
 * std::shared_ptr Structure
 *
 * - std::shared_ptr has two private data members
 * - A pointer to the allocated memory
 * - A pointer to its "Control block"
 *      - The control block contains the reference counter*/

/*
 * std::shared_ptr
 *
 * - std::shared_ptr is defined in <memory>
 * - To create a shared_ptr object
 *
 *
 *          // Pass a pointer as the constructor argument
 *          std::shared_ptr<int>ptr1 (new int(42));
 *
 *          // Calling std::make_shared() is better
 *          auto ptr2 = std::make_shared<int>(42);
 * */

/*
 * Copying std::shared_ptr
 *
 * - Copy constructor
 *      std::shared_ptr<int>ptr3 = ptr2;
 *
 * - Before the copy
 *      - ptr2's shared reference counter has the value 1
 *
 * - After the copy
 *      - ptr2 and ptr3 share the same memory pointer and control block
 *      - The counter in the shared control block has the value 2
 *      */

/*
 * std::shared_ptr Operations
 *
 * - shared_ptr supports the same operations as unique_ptr
 *      - Including dereferencing
 *          auto shptr = make_shared<int>(42);
 *          std::cout << "shared_ptr's data is " << *shptr << "\n";
 *      - Plus copy and assignment
 *          auto shptr2 = shptr;
 *          std::cout << "Copied shared_ptr's data is " << *shptr2 << std::endl;
 *
 *          std::shared_ptr<int> ptr3;
 *          shptr3 = shptr;
 *          std::cout << "Assigned shared_ptr's data is" << *shptr3 << std::endl;
 *          */

/*
 * shared_ptr vs unique_ptr
 *
 * - Unique_ptr has the same overhead as using a traditional pointer
 * - shared_ptr has more overhead
 *      - Control block initialization
 *      - The reference counter is updated on every copy, assignment move operation or destructor call
 * - only use std::shared_ptr when necessary
 *      */

/*
 * Thread and std::shared_ptr
 *
 * - Two potential issues
 * - The reference counter
 *      - Modified by every copy, assignment, move operation or destructor call
 *      - Conflicting accesses by concurrent threads
 *
 * - The pointed-to-data
 *      - Threads could dereference std::shared_ptr concurrently
 *      - Conflicting accesses
 * */

/*
 * Threads and std::shared_ptr
 *
 * - The reference counter is an atomic type
 *      - This makes it safe to use in threaded programs
 *      - No extra code required when copying, moving or assigning
 *      - Adds extra overhead in single-threaded programs
 *      - Internal synchronization
 *
 *
 * - The pointed-to data is the responsibility of the programmer
 *      - Must be protected against data races
 *      - Concurrent accesses to the data must be synchronized
 *      - C++20 has std::atomic<std::shared_ptr>
 *      - External synchronization
 *      */


// Shared vector
std::vector<int> vec;

// Mutex to protect std::vector's data
std::mutex mut;


void func1()
{
    // Potentially conflicting access - must be protected
    std::lock_guard<std::mutex> lgd(mut);
    for (int i = 0; i < 100000; ++i)
        vec.push_back(i);
}

void func2()
{
    // Potentially conflicting access - must be protected
    std::lock_guard<std::mutex> lgd(mut);
    for (int i =0; i < 200000; ++i)
        vec.push_back(i);
}



// std::shared_ptr has an "atomic" reference counter
std::shared_ptr<int> shptr11 = std::make_shared<int>(42);


void func100()
{
    // Increments shared_p's reference counter safe
    std::shared_ptr<int> shptr22 = shptr11;
}

void func200()
{
    // Increments shared_p's reference counter - safe
    std::shared_ptr<int> shptr33 = shptr11;
}

// mutex to protect std::shared_ptr's data

std::mutex shared_mut;

void func111()
{
    // Potentially conflicting access - must be protected
    std::lock_guard<std::mutex> lgd(shared_mut);
    *shptr11 = 5;
}

void func222()
{
    // Potentially conflicting access - must be protected
    std::lock_guard<std::mutex> lgd(shared_mut);
    *shptr11 = 7;
}
int main() {


    std::thread thr1(func1);
    std::thread thr2(func2);

    thr1.join();
    thr2.join();

    std::cout << "shptr data: ";
    for (int i = 0; i < 200000; ++i)
        std::cout << vec[i] << ", ";
    std::cout << "Finished" << std::endl;


    // Pass a pointer as the constructor argument
    std::shared_ptr<int> ptr1(new int(42));

    // Calling std::make_shared() is better
    auto ptr2 = std::make_shared<int>(42);

    // Can be dereferenced
    std::cout << *ptr1 << std::endl;

    // Pointer arithmentic is not supported
    // ++ptr1;

    // Assignment, copying and moving are allowed
    ptr1 = ptr2;
    std::shared_ptr<int> ptr3(ptr2);
    std::shared_ptr<int> ptr4(std::move(ptr2));

    // Relaeses the allocated memory
    ptr1 = nullptr;




    return 0;
}

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

const int ITERATIONS = 1000000000; // 1 billion increments

// Function to increment a shared variable safely using a mutex
void safeIncrementWithMutex(int& sharedVar, std::mutex& mtx) {
    for (int i = 0; i < ITERATIONS; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        ++sharedVar;
    }
}

// Function to increment a shared variable safely using std::atomic
void safeIncrementWithAtomic(std::atomic<int>& sharedVar) {
    for (int i = 0; i < ITERATIONS; ++i) {
        sharedVar.fetch_add(1, std::memory_order_relaxed);
    }
}

// Function to increment a shared variable unsafely (without synchronization)
void unsafeIncrement(int& sharedVar) {
    for (int i = 0; i < ITERATIONS; ++i) {
        ++sharedVar;
    }
}

int main() {
    int unsafeVar = 0;
    std::mutex mtx;
    std::atomic<int> atomicVar(0);

    // Unsafe increment without synchronization
    auto startUnsafe = std::chrono::high_resolution_clock::now();
    std::thread t1(unsafeIncrement, std::ref(unsafeVar));
    std::thread t2(unsafeIncrement, std::ref(unsafeVar));
    t1.join();
    t2.join();
    auto endUnsafe = std::chrono::high_resolution_clock::now();
    std::cout << "Unsafe increment result: " << unsafeVar << std::endl;
    std::cout << "Unsafe increment time: " << std::chrono::duration_cast<std::chrono::milliseconds>(endUnsafe - startUnsafe).count() << " ms" << std::endl;

    // Reset unsafe variable
    unsafeVar = 0;

    // Safe increment with mutex
    auto startMutex = std::chrono::high_resolution_clock::now();
    std::thread t3(safeIncrementWithMutex, std::ref(unsafeVar), std::ref(mtx));
    std::thread t4(safeIncrementWithMutex, std::ref(unsafeVar), std::ref(mtx));
    t3.join();
    t4.join();
    auto endMutex = std::chrono::high_resolution_clock::now();
    std::cout << "Safe increment with mutex result: " << unsafeVar << std::endl;
    std::cout << "Safe increment with mutex time: " << std::chrono::duration_cast<std::chrono::milliseconds>(endMutex - startMutex).count() << " ms" << std::endl;

    // Safe increment with atomic
    auto startAtomic = std::chrono::high_resolution_clock::now();
    std::thread t5(safeIncrementWithAtomic, std::ref(atomicVar));
    std::thread t6(safeIncrementWithAtomic, std::ref(atomicVar));
    t5.join();
    t6.join();
    auto endAtomic = std::chrono::high_resolution_clock::now();
    std::cout << "Safe increment with atomic result: " << atomicVar << std::endl;
    std::cout << "Safe increment with atomic time: " << std::chrono::duration_cast<std::chrono::milliseconds>(endAtomic - startAtomic).count() << " ms" << std::endl;

    return 0;
}
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

const int NUM_INCREMENTS = 1000;

std::mutex mtx;
std::condition_variable cv;
bool turn = false; // false for t1's turn, true for t2's turn

void synchronizedIncrement(int& sharedVar, int& localVar, bool myTurn) {
    for (int i = 0; i < NUM_INCREMENTS; ++i) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [myTurn] { return turn == myTurn; });
        ++sharedVar;
        ++localVar;
        std::cout << "Thread " << std::this_thread::get_id() << " incremented. SharedVar = " << sharedVar << ", LocalVar = " << localVar << std::endl;
        turn = !turn;
        cv.notify_all();
    }
}

int main() {
    int sharedVariable = 0;
    int localVar1 = 0;
    int localVar2 = 0;

    std::thread t1(synchronizedIncrement, std::ref(sharedVariable), std::ref(localVar1), false);
    std::thread t2(synchronizedIncrement, std::ref(sharedVariable), std::ref(localVar2), true);

    t1.join();
    t2.join();

    std::cout << "Synchronized increment result: " << sharedVariable << std::endl;
    std::cout << "Final value of local variable 1: " << localVar1 << std::endl;
    std::cout << "Final value of local variable 2: " << localVar2 << std::endl;

    return 0;
}
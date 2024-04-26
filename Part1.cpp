#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <random>
#include <chrono>
#include <functional>
#include <queue>
#include <future>

using namespace std;

mutex print_mtx; // Mutex for synchronizing output

class ThreadPool {
public:
    explicit ThreadPool(size_t);
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
    -> future<typename result_of<F(Args...)>::type>;
    ~ThreadPool();
private:
    vector< thread > workers;
    queue< function<void()> > tasks;
    mutex queue_mutex;
    condition_variable condition;
    bool stop;
};

ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for(size_t i = 0;i<threads;++i)
        workers.emplace_back(
                [this] {
                    for(;;) {
                        function<void()> task;
                        {
                            unique_lock<mutex> lock(this->queue_mutex);
                            this->condition.wait(lock, [this]{ return this->stop || !this->tasks.empty(); });
                            if(this->stop && this->tasks.empty())
                                return;
                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                        }
                        task();
                    }
                }
        );
}

template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> future<typename result_of<F(Args...)>::type> {
    using return_type = typename result_of<F(Args...)>::type;
    auto task = make_shared< packaged_task<return_type()> >(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    future<return_type> res = task->get_future();
    {
        unique_lock<mutex> lock(queue_mutex);
        if(stop)
            throw runtime_error("enqueue on stopped ThreadPool");
        tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one();
    return res;
}

ThreadPool::~ThreadPool() {
    {
        unique_lock<mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(thread &worker: workers)
        worker.join();
}

void populateMatrix(vector<vector<int>>& matrix) {
    random_device rd;
    mt19937 generator(rd());
    uniform_int_distribution<> distribution(1, 10);

    for (auto& row : matrix) {
        for (auto& element : row) {
            element = distribution(generator);
        }
    }
}

void computeElement(const vector<vector<int>>& matA, const vector<vector<int>>& matB, vector<vector<int>>& matC, int i, int j, int m, string task) {
    if (m == 0) return;
    matC[i][j] += matA[i][m-1] * matB[m-1][j];
    computeElement(matA, matB, matC, i, j, m-1, task);
    if (task == "task1" && m == 1) {
        lock_guard<mutex> lock(print_mtx); // Locking mutex for synchronized output
        cout << "[" << i << "," << j << "]=" << matC[i][j] << " ";
    }
}

void parallelMultiply(const vector<vector<int>>& matA, const vector<vector<int>>& matB, vector<vector<int>>& matC, int m, string task, ThreadPool& pool) {
    vector<future<void>> futures; // Store the futures

    for (int i = 0; i < matC.size(); ++i) {
        for (int j = 0; j < matC[0].size(); ++j) {
            futures.push_back(pool.enqueue(computeElement, cref(matA), cref(matB), ref(matC), i, j, m, task));
        }
    }

    for (auto &fut : futures) { // Wait for all tasks to finish
        fut.get();
    }
}

void performanceTest(int n, int m, int k) {
    double fastestTime = numeric_limits<double>::max();
    int optimalThreadCount = 0;

    for (int threadCount = 1; threadCount <= n*k; ++threadCount) {
        vector<vector<int>> matA(n, vector<int>(m));
        vector<vector<int>> matB(m, vector<int>(k));
        vector<vector<int>> matC(n, vector<int>(k, 0));

        populateMatrix(matA);
        populateMatrix(matB);

        ThreadPool pool(threadCount); // Create a thread pool with the desired number of threads

        auto start = chrono::high_resolution_clock::now();
        parallelMultiply(matA, matB, matC, m, "task2", pool);
        auto finish = chrono::high_resolution_clock::now();

        chrono::duration<double> elapsed = finish - start;
        cout << "Threads: " << threadCount << " - Time taken: " << elapsed.count() << " seconds.\n";

        if (elapsed.count() < fastestTime) {
            fastestTime = elapsed.count();
            optimalThreadCount = threadCount;
        }
    }

    cout << "\nFastest execution time was " << fastestTime << " seconds with " << optimalThreadCount << " threads.\n";
}

int main() {
    int n = 11;
    int m = 9;
    int k = 8;

    cout << "Demonstrating parallelism with real-time results:\n";
    vector<vector<int>> matA(n, vector<int>(m));
    vector<vector<int>> matB(m, vector<int>(k));
    vector<vector<int>> matC(n, vector<int>(k, 0));
    populateMatrix(matA);
    populateMatrix(matB);
    ThreadPool pool(n*k);
    parallelMultiply(matA, matB, matC, m, "task1", pool);

    cout << "\n";

    cout << "\nTesting performance with varying number of threads:\n";
    performanceTest(n, m, k);

    return 0;
}
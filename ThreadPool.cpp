#include<vector>
#include<thread>
#include<functional> // to get the function variable
#include<condition_variable>
#include<mutex>
#include<queue>
#include<memory>
#include<utility>
#include<chrono>
#include<future>
#include<future>
#include<iostream>

using namespace std;


class ThreadPool{
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();

    template<class F, class... Args>
    auto ExecuteTask(F&& f, Args&&... args) -> future<decltype(f(args...))>;

private:
    vector<thread> threads;
    queue<function<void()>> tasks;

    mutex mtx;
    condition_variable cv;
    bool stop = false;

    };

ThreadPool::ThreadPool(size_t numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        threads.emplace_back([this]() {
            std::function<void()> task;

            while (true) {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this]() {
                    return stop || !tasks.empty();
                });

                if (stop && tasks.empty())
                    return;  // ✅ void return only

                task = std::move(tasks.front());
                tasks.pop();
                
                lock.unlock();  // optional: release early
                task();
            }
        });
    }
}


    ThreadPool::~ThreadPool(){
        {
            unique_lock<mutex> lock(mtx);
            stop = true;
        }// scoped

        cv.notify_all();

        for(thread& t : threads){
            if(t.joinable())
                t.join();
        }

    }

    template<class F, class... Args>
    auto ThreadPool::ExecuteTask(F&& f, Args&&... args) -> future<decltype(f(args...))>{
        using return_type = decltype(f(args...));

        auto task = make_shared<packaged_task<return_type()>>(
            bind(forward<F>(f), forward<Args>(args)...)
        );

        future<return_type> res = task->get_future();

        {
            unique_lock<mutex> lock(mtx);
            tasks.emplace([tasks]() {(*task)(); });
        }

        cv.notify_one();
        return res;
    
}

int Func(int x){
    return x*2;
}



int main(){
    ThreadPool pool(16);

    std::vector<std::future<int>> futures;

    // Simulate incoming requests
    for (int i = 1; i <= 10; ++i) {
        std::cout << "Submitting task " << i << std::endl;
        futures.push_back(pool.ExecuteTask(Func, i));

        std::this_thread::sleep_for(std::chrono::milliseconds(50));  // simulate delay between tasks
    }

    // Collect results
    for (int i = 0; i < futures.size(); ++i) {
        std::cout << "Result for task " << i+1 << ": " << futures[i].get() << std::endl;
    }

    std::cout << "All tasks completed.\n";

    return 0;

}
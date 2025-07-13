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
#include<iostream>
#include<atomic>

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
            bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        future<return_type> res = task->get_future();

        {
            unique_lock<mutex> lock(mtx);
            tasks.emplace([task]() {(*task)(); });
        }

        cv.notify_one();
        return res;
    
}

int Func(int x){
    return x*2;
}


int main() {
    ThreadPool pool(4);

    std::atomic<int> taskId{1};

    for (int i = 1; i <= 100; ++i) {
        int id = taskId++;
        pool.ExecuteTask([id]() {
            std::ostringstream log;
            log << "🟡 Thread " << std::this_thread::get_id()
                << " picked Task " << id << "\n";
            std::cout << log.str();

            std::this_thread::sleep_for(std::chrono::milliseconds(300)); // Simulate work

            log.str("");
            log << "✅ Thread " << std::this_thread::get_id()
                << " completed Task " << id << "\n";
            std::cout << log.str();
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Stagger tasks
    }

    std::this_thread::sleep_for(std::chrono::seconds(5)); // Allow time for all tasks to finish
    return 0;
}

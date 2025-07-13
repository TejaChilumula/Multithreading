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



// class for the Thread Pool, it should have the 
/* Constructor -to define the threads and to make the thread work

    Destructor - to destroy all the threads after finishing its tasks
*/
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

/*
    - So,   vector<thread> threads ---> stores the threads
    - so how to initialise the thread ?
        whenever we are adding the thread to the queue, you are defining the thread, 
        so, it automatically starts running.

        Each, thread will have all the
        while(true){ ..... } , 10 Threads ---> each of while{....} 

        Each Thread
            - waits on the cv, 
            -      then takes the function from the task<function<void()>>
            - and executs the task()
*/
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



    /*
        Template based function
         - Where you accepts any class or lambda or fun and Args...
               - return the future so you cant give the actual return type at the start so we do
               """ Trailing return Type"""
        1. We use the return_type of the F we get as arg
        2. We bind the function and args using bind()
            - We pack the task -- to get the future result using package_task()
            - we then  make_shared() ptr so that we can move the ptr not to copy
        
        Then ...
          *********IMPPPPP***********

           tasks.emplace([task]() {(*task)(); });

           [task] - this is outside of lambda, so we tell, we are using task
           () - no Args
            (*task) -> task is a shared_ptr so, use *
            () -> actually calling the function task ,, i.e. task() !
    */

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

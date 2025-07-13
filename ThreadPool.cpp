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

    ThreadPool::ThreadPool(size_t numThreads){
        for(size_t i=0;i<numThreads;++i){
            threads.emplace_back([this](){
                
                    function<void()> task;

                    while(true){
                        unique_lock<mutex> lock(mtx);
                        cv.wait(lock, [this](){
                            return stop || !tasks.empty();
                        });

                        if(stop && tasks.empty()) return false;

                        task = move(tasks.front());
                        tasks.pop();
                    }

                    task();
                
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
        using return_type = 
    }






int main(){


}
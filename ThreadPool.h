#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>

// the actual thread pool
struct ThreadPool {
   ThreadPool(std::size_t);
   template<class F>
   void enqueue(F f);
   ~ThreadPool();
private:
   // the io_service we are wrapping
   boost::asio::io_service service;
   using asio_worker = std::unique_ptr<boost::asio::io_service::work>;
   asio_worker working;
   // need to keep track of threads so we can join them
   std::vector<std::unique_ptr<boost::thread>> workers;
};


// add new work item to the pool
template<class F>
void ThreadPool::enqueue(F f) {
   service.post(f);
}



#endif // THREADPOOL_H

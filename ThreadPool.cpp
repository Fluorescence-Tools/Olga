#include "ThreadPool.h"
// the constructor just launches some amount of workers
ThreadPool::ThreadPool(size_t threads)
   :service()
   ,working(new asio_worker::element_type(service))
{
   for ( std::size_t i = 0; i < threads; ++i ) {
      workers.push_back(
	 std::unique_ptr<boost::thread>(
	    new boost::thread([this]
	    {
	       service.run();
	    })
	 )
      );
   }
}
// the destructor joins all threads
ThreadPool::~ThreadPool() {
   //service.stop();
   //for ( std::size_t i = 0; i < workers.size(); ++i )
   //   workers[i]->join();
   working.reset();
   service.run();
}

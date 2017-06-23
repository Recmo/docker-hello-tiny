#include "EventLoop.h"
#include "Timer.h"
#include "Listener.h"
#include "Socket.h"
#include "Response.h"
#include <iostream>
#include <thread>
#include <pthread.h>

#include <fcntl.h> // for O_RDONLY & friends
#include <unistd.h> // for open, lseek64

class TimeableDemo: public Timeable {
public:
  virtual void handle(EventLoop& el, Timer& timer) {
    std::cout <<
      "Thread " << std::this_thread::get_id() <<
      " current time " << timer.current_time_ns() << "\n";
  }
};

void thread(Listener* listener)
{
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);

  EventLoop loop;
  loop.add_handler(listener);
  TimeableDemo demo;
  loop.timer.add_handler(&demo);
  demo.set_timeout(loop.timer, loop.timer.current_time_ns() + 1e9 * (rand() % 10));
  loop.run();
}

int main(int argc, char*argv[])
{
  try {
    // Initialize responses
    Response::initialize(".", "public,max-age=86400");

    // Open listening socket
    Listener listener{"8080"};

    // Find ideal number of threads
    const uint num_threads = std::thread::hardware_concurrency();
    std::cerr << "Using " << num_threads << " threads.\n";
    if(num_threads == 0) {
      throw std::runtime_error{
        "Could not determine hardware concurrency."
      };
    }

    // Spawn threads
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for(uint i = 0; i < num_threads; ++i) {
      // TODO pthread_setaffinity_np
      threads.emplace_back([&](){
        try {
          thread(&listener);
        }
        catch(std::exception& e) {
          std::cerr << "Uncaught exception: " << e.what() << "\n"
            "Terminating!\n";
          std::exit(EXIT_FAILURE);
        }
      });
    }

    // Wait for completion
    for(std::thread& thread: threads) {
      thread.join();
    }
  }
  catch(std::exception& e) {
    std::cerr << "Uncaught exception: " << e.what() << "\n"
      "Terminating!\n";
    std::exit(EXIT_FAILURE);
  }
  return EXIT_SUCCESS;
}

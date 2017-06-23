#include "Timer.h"
#include <sys/timerfd.h>
#include <system_error>
#include <cerrno>
#include <sys/epoll.h> // for EPOLLIN
#include <unistd.h> // for read and close
#include <time.h> // for clock_gettime
#include "syscall.h"
#include "fail.h"

Timer::Timer()
: EventHandler(create_timer())
{
}

Timer::~Timer()
{
  syscall(SYS_close, descriptor);
}

void Timer::handle(EventLoop& el, uint32_t events)
{
  if(events != EPOLLIN) {
    fail(0, "Unexpected event from timerfd");
  }

  syscall(SYS_write, 1, "TIMEOUT\n", 8);

  // Flush the timerfd read buffer
  flush_timer();
}

uint64_t Timer::current_time_ns()
{
  timespec time;
  const int result = syscall(SYS_clock_gettime, CLOCK_BOOTTIME, &time);
  if(result < 0) {
    fail(-result, "Error reading clock");
  }

  // BOOT_CLOCK in uint64 nanoseconds wraps arround in:
  // 2⁶⁴ / (10⁹ · 3600 · 24 · 365.25) = 584 years
  // i.e. we will assume it does not wrap arround
  const uint64_t time_ns = time.tv_sec * 1000000000ULL + time.tv_nsec;
  return time_ns;
}

int Timer::create_timer()
{
  const int result = syscall(SYS_timerfd_create, CLOCK_BOOTTIME, TFD_NONBLOCK | TFD_CLOEXEC);
  if(result < 0) {
    fail(-result, "Error opening timer");
  }
  return result;
}

void Timer::flush_timer()
{
  // Read an uint64 from timerfd
  uint64_t expirations = 0;
  const int result = syscall(SYS_read, descriptor, &expirations, sizeof(expirations));
  if(result < 0) {
    fail(-result, "Error reading from timerfd");
  }
  if(result != 8) {
    fail(result, "Unexpected read from timerfd");
  }
  if(expirations != 1) {
    // TODO: This should not be fatal.
    fail(expirations, "Unexpected number of expirations from timerfd");
  }
}

void Timer::start(uint64_t interval_ns)
{
  const time_t s = interval_ns / 1000000000ULL;
  const long ns = interval_ns % 1000000000ULL;
  const itimerspec timeout{{s, ns}, {s, ns}};
  const int result = syscall(SYS_timerfd_settime, descriptor, 0, &timeout, nullptr);
  if(result < 0) {
    fail(-result, "Error setting timerfd");
  }
}

void Timer::stop()
{
  start(0);
}

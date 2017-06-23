#include "Signals.h"
#include "EventLoop.h"
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>
#include <cerrno>
#include "syscall.h"
#include "fail.h"

Signals::Signals()
: EventHandler(create())
{
}

Signals::~Signals()
{
  syscall(SYS_close, descriptor);
}

void Signals::handle(EventLoop &el, uint32_t events)
{
  if(events != EPOLLIN) {
    fail(events, "EPOLLIN");
  }

  // Handle one signal at a time
  signalfd_siginfo signals[1];
  const int result = syscall(SYS_read, descriptor, &signals, sizeof(signals));
  if(result < 0) {
    fail(events, "invalid read");
  }
  if(result == 0 || (result % sizeof(signalfd_siginfo)) != 0) {
    fail(result, "read Unexpected size");
  }
  const uint num_events = result / sizeof(signalfd_siginfo);

  for(uint i = 0; i < num_events; ++i) {
    const signalfd_siginfo& signal = signals[i];
    if(signal.ssi_signo == SIGINT) {
      syscall(SYS_write, 1, "INT\n", 4);
      el.stop();
    }
    if(signal.ssi_signo == SIGUSR1) {
      syscall(SYS_write, 1, "USR1\n", 5);
    }
  }
}

int Signals::create()
{
  const uint64_t signal_mask =
    (1UL << (SIGINT  - 1)) |
    (1UL << (SIGQUIT - 1)) |
    (1UL << (SIGUSR1 - 1));

  // Create a signalfd and register with epoll
  const int existing_fd = -1;
  const int flags = SFD_NONBLOCK | SFD_CLOEXEC;
  const int result = syscall(SYS_signalfd4, existing_fd, &signal_mask, sizeof(signal_mask), flags);
  if(result < 0) {
    fail(result, "signalfd");
  }
  const int fd = result;

  // Block other signal handlers
  sigset_t* old_set = nullptr;
  const int result2 = syscall(SYS_rt_sigprocmask, SIG_SETMASK, &signal_mask, old_set, sizeof(signal_mask));
  if(result2 < 0) {
    fail(result2, "procmask");
  }

  return fd;
}

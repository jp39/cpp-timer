// MIT License
//
// Copyright (c) 2020 jp39
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// SPDX-License-Identifier: MIT

#ifndef TIMER_H_
#define TIMER_H_

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <signal.h>
#include <time.h>
#endif

#include <chrono>
#include <functional>
#include <queue>
#include <vector>

class Timer {
public:
  using Handler = std::function<void()>;

  Timer(Handler handler) : handler_(handler) {
#ifdef _WIN32
    timer_ = CreateWaitableTimer(NULL, FALSE, NULL);
#else
    set_signal_handler();
    struct sigevent sev;

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = this;
    timer_create(CLOCK_MONOTONIC, &sev, &timer_);
#endif
  }

  ~Timer() {
#ifdef _WIN32
    CloseHandle(timer_);
#else
    timer_delete(&timer_);
#endif
  }

  template <class IRep, class IPeriod, class VRep, class VPeriod>
  void SetTime(std::chrono::duration<IRep, IPeriod> interval,
               std::chrono::duration<VRep, VPeriod> value) {
#ifdef _WIN32
    auto value_100ns = std::chrono::duration_cast<
        std::chrono::duration<LONGLONG, std::ratio<1, 10000000>>>(value);
    LARGE_INTEGER due_time;
    due_time.QuadPart = -value_100ns.count();
    auto period_ms =
        std::chrono::duration_cast<std::chrono::duration<LONG, std::milli>>(
            interval);

    SetWaitableTimer(timer_, &due_time, period_ms.count(), apc_routine, this,
                     FALSE);
#else
    struct itimerspec it;

    auto secs = std::chrono::duration_cast<std::chrono::seconds>(interval);
    auto nanosecs =
        std::chrono::duration_cast<std::chrono::nanoseconds>(interval) -
        std::chrono::duration_cast<std::chrono::nanoseconds>(secs);

    it.it_interval.tv_sec = secs.count();
    it.it_interval.tv_nsec = nanosecs.count();

    secs = std::chrono::duration_cast<std::chrono::seconds>(value);
    nanosecs = std::chrono::duration_cast<std::chrono::nanoseconds>(value) -
               std::chrono::duration_cast<std::chrono::nanoseconds>(secs);

    it.it_value.tv_sec = secs.count();
    it.it_value.tv_nsec = nanosecs.count();

    timer_settime(timer_, 0, &it, NULL);
#endif
  }

  static void Dispatch() {
#ifndef _WIN32
    sigset_t mask;

    sigprocmask(SIG_SETMASK, NULL, &mask);
    sigdelset(&mask, SIGRTMIN);
#endif

    while (1) {
#ifdef _WIN32
      SleepEx(INFINITE, TRUE);
#else
      sigsuspend(&mask);
#endif
      while (!timer_queue().empty()) {
        Timer *t = timer_queue().front();

        timer_queue().pop();
        t->handler_();
      }
    }
  }

private:
#ifdef _WIN32
  static void CALLBACK apc_routine(LPVOID arg, DWORD time_low,
                                   DWORD time_high) {
    Timer *timer = reinterpret_cast<Timer *>(arg);
    timer_queue().push(timer);
  }

  HANDLE timer_;
#else
  static void signal_handler(int sig, siginfo_t *si, void *uc) {
    Timer *timer = reinterpret_cast<Timer *>(si->si_value.sival_ptr);
    int overrun = timer_getoverrun(timer->timer_);
    timer_queue().push(timer);
    while (overrun--)
      timer_queue().push(timer);
  }

  static void set_signal_handler() {
    static bool signal_handler_set = false;

    if (signal_handler_set)
      return;

    struct sigaction sa;
    sigset_t mask;

    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = &signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGRTMIN, &sa, NULL);

    sigemptyset(&mask);
    sigaddset(&mask, SIGRTMIN);
    sigprocmask(SIG_SETMASK, &mask, NULL);

    signal_handler_set = true;
  }

  timer_t timer_;
#endif

  static std::queue<Timer *> &timer_queue() {
    static std::queue<Timer *> queue;

    return queue;
  }

  Handler handler_;
};

#endif // TIMER_H_

// vim: expandtab:sw=2:sts=2

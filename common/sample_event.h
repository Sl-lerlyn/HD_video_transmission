#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>

class noncopyable {
 protected:
  noncopyable() {}
  ~noncopyable() {}

 private:
  noncopyable(const noncopyable&) = delete;
  noncopyable& operator=(const noncopyable&) = delete;
};

class SampleEvent : public noncopyable {
 public:
  explicit SampleEvent(bool init = false) : signal_(init) {}
  ~SampleEvent() {}

  void Set();

  int Wait(int wait_ms = -1);

 private:
  std::condition_variable cv_;
  std::mutex lock_;
  std::atomic<bool> signal_;
};

#pragma once
#include "common/common_header.h"
#include "network_header.h"
#include <atomic>
#include <boost/asio.hpp>
#include <memory>
#include <thread>
#include <vector>

namespace Network {

struct Context {
  Context() {
    ioContext = std::make_shared<boost::asio::io_context>();
    work = std::make_shared<boost::asio::io_context::work>(*ioContext);
    thread = std::make_shared<std::thread>([this]() { ioContext->run(); });
  }

  void Stop() {
    ioContext->stop();
    if (thread && thread->joinable()) {
      thread->join();
    }
  }

  std::shared_ptr<boost::asio::io_context> ioContext;
  std::shared_ptr<boost::asio::io_context::work> work;
  std::shared_ptr<std::thread> thread;
};

//====================================================================================================
// Network::ContextPool
//====================================================================================================
class ContextPool {
public:
  explicit ContextPool(int poolCount);
  virtual ~ContextPool();

  void Run();
  void Stop();
  std::shared_ptr<boost::asio::io_context> GetContext();
  std::shared_ptr<boost::asio::io_context> GetPrivateContext();
  bool IsRun() const { return isRun; }

private:
  std::vector<std::shared_ptr<Context>> contextList;
  uint32_t poolCount;
  uint32_t privateIoContextIndex = 0;
  std::atomic<uint32_t> ioContextIndex{0};
  bool isRun = false;
};

} // namespace Network

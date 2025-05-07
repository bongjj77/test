#include "network_context_pool.h"
#include <stdexcept>

namespace Network {

//====================================================================================================
// Constructor
//====================================================================================================
ContextPool::ContextPool(int poolCount) : poolCount(poolCount > 0 ? poolCount : std::thread::hardware_concurrency()) {
  LOG_INFO("Network pool count %d", this->poolCount);
}

//====================================================================================================
// Destructor
//====================================================================================================
ContextPool::~ContextPool() { Stop(); }

//====================================================================================================
// Run
//====================================================================================================
void ContextPool::Run() {
  if (isRun) {
    return;
  }

  for (uint32_t index = 0; index < poolCount; ++index) {
    auto context = std::make_shared<Context>();
    contextList.push_back(context);
  }

  isRun = true;
}

//====================================================================================================
// Stop
//====================================================================================================
void ContextPool::Stop() {
  if (!isRun) {
    return;
  }

  for (auto &context : contextList) {
    context->Stop();
  }

  isRun = false;
}

//====================================================================================================
// GetContext
//====================================================================================================
std::shared_ptr<boost::asio::io_context> ContextPool::GetContext() {
  if (!isRun) {
    return nullptr;
  }

  if (ioContextIndex >= poolCount) {
    ioContextIndex = privateIoContextIndex;
  }

  return contextList[ioContextIndex++ % poolCount]->ioContext;
}

//====================================================================================================
// GetPrivateContext
//====================================================================================================
std::shared_ptr<boost::asio::io_context> ContextPool::GetPrivateContext() {
  if (!isRun) {
    return nullptr;
  }

  uint32_t index = (privateIoContextIndex < poolCount / 3) ? privateIoContextIndex++ : ioContextIndex++ % poolCount;

  return contextList[index]->ioContext;
}

} // namespace Network

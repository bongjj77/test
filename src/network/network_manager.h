#pragma once
#include "network_context_pool.h"
#include "network_header.h"
#include <memory>
#include <string>
#include <vector>

namespace Network {

class Manager {
public:
  Manager(int objectKey, const std::string &objectName, const std::shared_ptr<NetEvent> &netEvent);
  virtual ~Manager();

  virtual void PostRelease() = 0;
  virtual bool Remove(int indexKey) = 0;
  virtual void Remove(const std::vector<int> &indexKeys) = 0;
  virtual void RemoveAll() = 0;
  virtual uint32_t GetCount() const = 0;

  int GetObjectKey() const { return _objectKey; }
  const std::string &GetObjectName() const { return _objectName; }

protected:
  int _objectKey = -1;
  std::string _objectName;
  std::weak_ptr<NetEvent> _netEvent;
  std::shared_ptr<ContextPool> _servicePool = nullptr;
};

} // namespace Network

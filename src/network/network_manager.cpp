#include "network_manager.h"
#include "common/common_header.h"

namespace Network {

Manager::Manager(int objectKey, const std::string &objectName, const std::shared_ptr<NetEvent> &netEvent)
    : _objectKey(objectKey), _objectName(objectName), _netEvent(netEvent) {}

Manager::~Manager() {}

} // namespace Network

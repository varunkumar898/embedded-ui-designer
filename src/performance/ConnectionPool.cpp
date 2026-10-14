#include "ConnectionPool.h"
#include <algorithm>

namespace performance {

ConnectionPool::ConnectionPool(uint32_t maxConnections, 
                               std::chrono::seconds idleTimeout)
    : maxConnections(maxConnections), idleTimeout(idleTimeout) {}

std::shared_ptr<Connection> ConnectionPool::acquire(const std::string& deviceId,
                                                    const std::string& ipAddress,
                                                    uint16_t port) {
    auto now = std::chrono::system_clock::now();
    
    // Try to find an idle connection for this device/ip
    for (auto it = idleConnections.begin(); it != idleConnections.end(); ++it) {
        auto conn = *it;
        if (conn->deviceId == deviceId && conn->ipAddress == ipAddress && conn->port == port) {
            conn->isActive = true;
            conn->lastUsedAt = now;
            conn->requestCount++;
            activeConnections.push_back(conn);
            idleConnections.erase(it);
            return conn;
        }
    }
    
    // Create new connection if under limit
    if (activeConnections.size() + idleConnections.size() < maxConnections) {
        return createConnection(deviceId, ipAddress, port);
    }
    
    // Close an idle connection and reuse
    if (!idleConnections.empty()) {
        idleConnections.erase(idleConnections.begin());
        return createConnection(deviceId, ipAddress, port);
    }
    
    return nullptr;
}

void ConnectionPool::release(std::shared_ptr<Connection> conn) {
    auto it = std::find(activeConnections.begin(), activeConnections.end(), conn);
    if (it != activeConnections.end()) {
        conn->isActive = false;
        conn->lastUsedAt = std::chrono::system_clock::now();
        idleConnections.push_back(conn);
        activeConnections.erase(it);
    }
}

uint32_t ConnectionPool::getActiveConnectionCount() const {
    return static_cast<uint32_t>(activeConnections.size());
}

uint32_t ConnectionPool::getIdleConnectionCount() const {
    return static_cast<uint32_t>(idleConnections.size());
}

uint32_t ConnectionPool::closeIdleConnections() {
    auto now = std::chrono::system_clock::now();
    uint32_t closed = 0;
    
    auto it = idleConnections.begin();
    while (it != idleConnections.end()) {
        if ((now - (*it)->lastUsedAt) > idleTimeout) {
            it = idleConnections.erase(it);
            closed++;
        } else {
            ++it;
        }
    }
    
    return closed;
}

void ConnectionPool::clear() {
    activeConnections.clear();
    idleConnections.clear();
}

std::shared_ptr<Connection> ConnectionPool::createConnection(const std::string& deviceId,
                                                             const std::string& ipAddress,
                                                             uint16_t port) {
    auto conn = std::make_shared<Connection>();
    conn->deviceId = deviceId;
    conn->ipAddress = ipAddress;
    conn->port = port;
    conn->createdAt = std::chrono::system_clock::now();
    conn->lastUsedAt = conn->createdAt;
    conn->isActive = true;
    conn->requestCount = 1;
    
    activeConnections.push_back(conn);
    return conn;
}

} // namespace performance

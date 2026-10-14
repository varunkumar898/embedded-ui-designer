#pragma once
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <chrono>

namespace performance {

struct Connection {
    std::string deviceId;
    std::string ipAddress;
    uint16_t port{8080};
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point lastUsedAt;
    bool isActive{true};
    uint32_t requestCount{0};
};

class ConnectionPool {
public:
    ConnectionPool(uint32_t maxConnections = 100, 
                   std::chrono::seconds idleTimeout = std::chrono::seconds(300));
    
    // Acquire or create connection
    std::shared_ptr<Connection> acquire(const std::string& deviceId, 
                                        const std::string& ipAddress,
                                        uint16_t port = 8080);
    
    // Release connection back to pool
    void release(std::shared_ptr<Connection> conn);
    
    // Get pool statistics
    uint32_t getActiveConnectionCount() const;
    uint32_t getIdleConnectionCount() const;
    
    // Close idle connections
    uint32_t closeIdleConnections();
    
    // Clear all connections
    void clear();
    
private:
    std::vector<std::shared_ptr<Connection>> activeConnections;
    std::vector<std::shared_ptr<Connection>> idleConnections;
    
    uint32_t maxConnections;
    std::chrono::seconds idleTimeout;
    
    std::shared_ptr<Connection> createConnection(const std::string& deviceId,
                                                 const std::string& ipAddress,
                                                 uint16_t port);
};

} // namespace performance

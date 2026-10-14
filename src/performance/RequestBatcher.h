#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace performance {

struct BatchedRequest {
    std::string operationId; // "readGpio", "readAdc", "setPwm"
    std::string deviceId;
    std::map<std::string, std::string> parameters;
};

struct BatchResponse {
    std::vector<std::string> results; // JSON responses for each request
    uint32_t totalProcessingTimeMs{0};
    bool success{false};
};

class RequestBatcher {
public:
    RequestBatcher(uint32_t maxBatchSize = 100);
    
    void addRequest(const BatchedRequest& request);
    uint32_t getBatchSize() const;
    BatchResponse executeBatch();
    void clear();
    
private:
    std::vector<BatchedRequest> batch;
    uint32_t maxBatchSize;
    
    std::string buildBatchPayload() const;
};

} // namespace performance

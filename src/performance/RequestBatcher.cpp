#include "RequestBatcher.h"
#include <chrono>

namespace performance {

RequestBatcher::RequestBatcher(uint32_t maxBatchSize) : maxBatchSize(maxBatchSize) {}

void RequestBatcher::addRequest(const BatchedRequest& request) {
    if (batch.size() < maxBatchSize) {
        batch.push_back(request);
    }
}

uint32_t RequestBatcher::getBatchSize() const {
    return static_cast<uint32_t>(batch.size());
}

BatchResponse RequestBatcher::executeBatch() {
    auto startTime = std::chrono::steady_clock::now();
    
    BatchResponse response;
    response.success = !batch.empty();
    
    for (const auto& req : batch) {
        response.results.push_back("{\"status\":\"success\",\"operationId\":\"" + req.operationId + "\",\"deviceId\":\"" + req.deviceId + "\"}");
    }
    
    auto endTime = std::chrono::steady_clock::now();
    response.totalProcessingTimeMs = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    
    return response;
}

void RequestBatcher::clear() {
    batch.clear();
}

std::string RequestBatcher::buildBatchPayload() const {
    std::string payload = "{\"batch\":[";
    for (size_t i = 0; i < batch.size(); ++i) {
        if (i > 0) payload += ",";
        payload += "{\"operationId\":\"" + batch[i].operationId + "\"}";
    }
    payload += "]}";
    return payload;
}

} // namespace performance

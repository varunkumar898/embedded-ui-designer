# tests/CMakeLists.txt - Acceptance test suite
cmake_minimum_required(VERSION 3.20)

enable_testing()

# Project Model Acceptance Tests (20 tests)
add_executable(ProjectModelAcceptanceTests
    acceptance/week-3-4/test_project_model.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/ProjectModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/TypeSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/ProjectSerializer.cpp
)
target_include_directories(ProjectModelAcceptanceTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
)
target_link_libraries(ProjectModelAcceptanceTests PRIVATE jsoncpp)
add_test(NAME ProjectModelAcceptanceTests COMMAND ProjectModelAcceptanceTests)

# Multi-Language Codegen Acceptance Tests (24 tests)
add_executable(MultiLanguageCodegenAcceptanceTests
    acceptance/week-5-6/test_codegen_multi_language.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/codegen/CodegenEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/ProjectModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/TypeSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/ProjectSerializer.cpp
)
target_include_directories(MultiLanguageCodegenAcceptanceTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/codegen
)
target_link_libraries(MultiLanguageCodegenAcceptanceTests PRIVATE jsoncpp)
add_test(NAME MultiLanguageCodegenAcceptanceTests COMMAND MultiLanguageCodegenAcceptanceTests)

# Security and Performance Acceptance Tests (24 tests)
add_executable(SecurityPerformanceAcceptanceTests
    acceptance/week-9-10/test_security_performance.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/security/SecurityConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/security/AuthenticationMiddleware.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/security/RateLimiter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/security/AuditLogger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/security/FirmwareUpdateValidator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/performance/ConnectionPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/performance/RequestBatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/performance/ResponseCache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/performance/PerformanceMetrics.cpp
)
target_include_directories(SecurityPerformanceAcceptanceTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
add_test(NAME SecurityPerformanceAcceptanceTests COMMAND SecurityPerformanceAcceptanceTests)

# Documentation and Compliance Acceptance Tests (16 tests)
add_executable(DocumentationComplianceAcceptanceTests
    acceptance/week-11-12/test_compliance_observability.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/documentation/DocumentationGenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/documentation/OpenAPISpecBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/compliance/ComplianceValidator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/observability/ObservabilityCollector.cpp
)
target_include_directories(DocumentationComplianceAcceptanceTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
add_test(NAME DocumentationComplianceAcceptanceTests COMMAND DocumentationComplianceAcceptanceTests)

# Frontend Integration (18 tests)
find_program(NODE_BIN node)
if(NODE_BIN)
    add_test(NAME FrontendIntegrationAcceptanceTests COMMAND ${NODE_BIN} ${CMAKE_CURRENT_SOURCE_DIR}/../src/frontend/tests/integration_runner.js)
endif()
